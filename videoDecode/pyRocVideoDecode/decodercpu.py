# Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

"""PyAV software decoding with rocPyDecode surface and tensor outputs."""
from collections import deque
from fractions import Fraction
import ctypes
import threading
import rocpydecode as rocpydec
import rocpydecode.decTypes as dectypes
from ._pyav import codec_name, require_av
from .decoder import (GetOutputFormat, GetRocDecCodecID, GetRectangle, GetDim,
                      GetOutputSurfaceInfo, GetRocPyDecPacket)


class decodercpu:
    def __init__(self, codec, device_id=0, mem_type=dectypes.OUT_SURFACE_MEM_HOST_COPIED,
                 b_force_zero_latency=False, crop_rect=None, max_width=0,
                 max_height=0, clk_rate=1000):
        if mem_type not in (1, 2):
            raise ValueError("CPU decoding supports host-copied or device-copied output")
        if device_id < 0 or max_width < 0 or max_height < 0 or clk_rate <= 0:
            raise ValueError("Invalid device, dimensions or clock rate")
        self._crop = GetRectangle(crop_rect)
        if crop_rect is not None and any(crop_rect) and (min(crop_rect) < 0 or crop_rect[2] <= crop_rect[0] or crop_rect[3] <= crop_rect[1]):
            raise ValueError("Invalid crop rectangle")
        self._av = require_av()
        name = codec_name(codec)
        # The decoder named "av1" can be hardware-only. Prefer the software
        # decoder shipped by PyAV's wheels, as PyAV container decoding does.
        if name == "av1":
            name = next((candidate for candidate in ("libdav1d", "libaom-av1")
                         if candidate in self._av.codecs_available), name)
        self._codec = self._av.CodecContext.create(name, "r")
        # Slice threading does not introduce frame-threading delay.
        self._codec.thread_type = "SLICE"
        self._device, self._memory, self._clock = device_id, mem_type, clk_rate
        self._frames = deque()
        self._surface = None
        self._last_frame = None
        self._time_base = Fraction(1, clk_rate)
        self._eos = False
        self._flushed = 0
        self._overhead = {}
        self._lock = threading.RLock()

    @property
    def viddec(self):
        return self

    def DecodeFrame(self, packet):
        with self._lock:
            size = packet.bitstream_size
            if size < 0 or size > 2**31 - 1:
                raise ValueError("CPU decode packet size is outside the supported integer range")
            if size and not packet.bitstream_adrs:
                raise ValueError("Non-empty CPU packet requires a bitstream address")
            if size == 0:
                if self._eos:
                    return 0
                self._eos = True
                packet.pkt_flags |= int(dectypes.ROCDEC_PKT_ENDOFSTREAM)
                frames = self._codec.decode(None) if self._codec.is_open else []
                self._flushed = len(frames)
            else:
                if self._eos:
                    raise ValueError("CPU decoder has reached end of stream; create a new decoder")
                original = getattr(packet, "_av_packet", None)
                has_timestamp = bool(packet.pkt_flags & int(dectypes.ROCDEC_PKT_TIMESTAMP))
                original_matches = original is not None and original.size == size and original.buffer_ptr == packet.bitstream_adrs
                if original_matches:
                    # Keep the original time base only while the public timestamp
                    # is unchanged. Caller edits must take effect on CPU output.
                    original_matches = (
                        not has_timestamp and original.pts is None
                    ) or (
                        has_timestamp and original.pts is not None and original.time_base is not None
                        and packet.frame_pts == int(original.pts * original.time_base * 1000)
                    )
                if original_matches:
                    encoded = original
                else:
                    encoded = self._av.Packet(ctypes.string_at(packet.bitstream_adrs, size))
                    if has_timestamp:
                        encoded.pts = packet.frame_pts
                        encoded.time_base = Fraction(1, self._clock)
                if encoded.time_base is not None:
                    self._time_base = encoded.time_base
                frames = self._codec.decode(encoded)
            for frame in frames:
                if frame.time_base is None:
                    frame.time_base = self._time_base
            self._frames.extend(frames)
            if frames:
                self._last_frame = frames[-1]
            return len(frames)

    def _prepare_surface(self, frame):
        depth = max(c.bits for c in frame.format.components)
        formats = {
            "yuv420p": "YUV420", "yuvj420p": "YUV420", "yuv422p": "YUV422",
            "yuvj422p": "YUV422", "yuv444p": "YUV444", "yuvj444p": "YUV444",
        }
        suffix = formats.get(frame.format.name)
        if suffix is None and frame.format.name.startswith(("yuv420p", "yuv422p", "yuv444p")) and frame.format.name.endswith("le"):
            suffix = "YUV" + frame.format.name[3:6] + "_16Bit"
        if suffix is None:
            raise ValueError(f"Unsupported CPU output format: {frame.format.name}")
        surface_format = getattr(dectypes.rocDecVideoSurfaceFormat, "rocDecVideoSurfaceFormat_" + suffix)
        self._surface = rocpydec._CpuSurface(
            frame.planes, [p.line_size for p in frame.planes], frame.width, frame.height,
            depth, surface_format, self._device, self._memory, self._crop)

    def _take_frame(self):
        if not self._frames:
            return None
        frame = self._frames[0]
        self._prepare_surface(frame)
        self._frames.popleft()
        self._last_frame = frame
        return int(frame.pts * frame.time_base * self._clock) if frame.pts is not None and frame.time_base else 0

    def GetFrameYuv(self, packet, separate_planes=False):
        with self._lock:
            pts = self._take_frame()
            if pts is None:
                return -1
            packet.frame_pts = pts
            self._surface.Yuv(packet, separate_planes)
            return pts

    def GetFrameRgb(self, packet, rgb_format):
        if rgb_format not in range(1, 9):
            raise ValueError("RGB format must be in the range 1 through 8")
        with self._lock:
            pts = self._take_frame()
            if pts is None:
                return -1
            packet.frame_pts = pts
            self._surface.Rgb(packet, rgb_format)
            return pts

    def GetGpuInfo(self):
        return rocpydec._cpu_device_info(self._device)

    GetDeviceinfo = GetGpuInfo

    def _ensure_surface(self):
        with self._lock:
            if self._surface is None and self._frames:
                self._prepare_surface(self._frames[0])

    def GetWidth(self):
        self._ensure_surface()
        return self._surface.width if self._surface else (self._last_frame.width if self._last_frame else 0)

    def GetHeight(self):
        self._ensure_surface()
        return self._surface.height if self._surface else (self._last_frame.height if self._last_frame else 0)

    def GetStride(self):
        self._ensure_surface()
        return self._surface.pitch if self._surface else 0

    def GetFrameSize(self):
        self._ensure_surface()
        return self._surface.size if self._surface else 0

    def GetBitDepth(self):
        return max(c.bits for c in self._last_frame.format.components) if self._last_frame else 0

    def GetOutputSurfaceInfo(self):
        self._ensure_surface()
        return self._surface.info_address if self._surface else 0

    def GetResizedOutputSurfaceInfo(self):
        return self._surface.resized_info_address if self._surface else 0

    def ResizeFrame(self, packet, resize_dim, surface_info):
        if not self._surface:
            return 0
        dim = resize_dim if isinstance(resize_dim, rocpydec.Dim) else GetDim(resize_dim)
        return self._surface.Resize(packet, dim, surface_info)

    def SaveFrameToFile(self, output_file_path, frame_adrs, surface_info=0,
                        output_format=dectypes.OutputFormatEnum.native):
        if not self._surface:
            raise ValueError("No CPU frame has been retrieved")
        self._surface.Save(output_file_path, frame_adrs, surface_info, GetOutputFormat(output_format))

    def ReleaseFrame(self, packet):
        # Exported DLPack buffers retain their allocations independently.
        return True

    def GetNumOfFlushedFrames(self):
        return self._flushed

    def AddDecoderSessionOverHead(self, session_id, duration):
        with self._lock:
            self._overhead[session_id] = self._overhead.get(session_id, 0.0) + duration

    def GetDecoderSessionOverHead(self, session_id):
        with self._lock:
            return self._overhead.get(session_id, 0.0)

    def IsCodecSupported(self, device_id, codec_id, bit_depth):
        try:
            self._av.Codec(codec_name(codec_id), "r")
            return bit_depth in (8, 10, 12, 16)
        except ValueError:
            return False


class PyRocVideoDecoderCpu(decodercpu):
    """CPU decoder entry point with device and memory type as leading arguments."""
    def __init__(self, device_id=0, out_mem_type=2, codec=dectypes.rocDecVideoCodec_HEVC,
                 force_zero_latency=False, p_crop_rect=None, max_width=0, max_height=0,
                 clk_rate=1000):
        crop = None if p_crop_rect is None else (p_crop_rect.left, p_crop_rect.top,
                                                p_crop_rect.right, p_crop_rect.bottom)
        if crop == (0, 0, 0, 0):
            crop = None
        super().__init__(codec, device_id, out_mem_type, force_zero_latency, crop,
                         max_width, max_height, clk_rate)
