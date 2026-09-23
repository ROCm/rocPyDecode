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

"""Container demuxing through PyAV; packets remain compressed for rocDecode."""
from collections import deque
from fractions import Fraction
from pathlib import Path
import io
import operator
import threading
import rocpydecode as rocpydec
from ._pyav import require_av


class stream_provider(io.BytesIO):
    """A seekable in-memory input for container demuxing."""
    def __init__(self, input_file_path):
        super().__init__(Path(input_file_path).read_bytes())

    def GetFileStreamProvider(self):
        return self

    def GetBufferSize(self):
        return max(0, self.getbuffer().nbytes - self.tell())

    def GetData(self, buffer, n_buf):
        n_buf = operator.index(n_buf)
        if n_buf <= 0:
            return 0
        target = memoryview(buffer).cast("B")
        if target.readonly:
            raise TypeError("GetData requires a writable destination buffer")
        data = self.read(min(n_buf, target.nbytes))
        target[:len(data)] = data
        return len(data)


class demuxer:
    def __init__(self, name):
        self._lock = threading.RLock()
        self._closed = False
        av = require_av()
        self._container = av.open(str(name) if isinstance(name, Path) else name)
        try:
            if not self._container.streams.video:
                raise ValueError("Input has no video stream")
            self._stream = self._container.streams.video[0]
            self._filter = None
            codec = self._stream.codec_context.name
            if codec in ("h264", "hevc"):
                self._filter = av.BitStreamFilterContext(codec + "_mp4toannexb", self._stream)
            elif codec == "mpeg4":
                self._filter = av.BitStreamFilterContext("dump_extra", self._stream)
            self._packets = iter(self._container.demux(self._stream))
            self._pending = deque()
            self._drained = False
        except Exception:
            self._container.close()
            raise

    @property
    def vidmux(self):
        return self

    def _check_open(self):
        if self._closed:
            raise ValueError("Demuxer is closed")

    def GetCodecId(self):
        with self._lock:
            self._check_open()
            return self._stream.codec_context.codec.id

    def GetBitDepth(self):
        with self._lock:
            self._check_open()
            fmt = self._stream.codec_context.format
            if fmt is None:
                raise ValueError("Input does not report a pixel format/bit depth")
            return max(component.bits for component in fmt.components)

    def _next_packet(self):
        while not self._pending:
            packet = next(self._packets, None)
            if packet is None:
                if self._filter and not self._drained:
                    self._drained = True
                    self._pending.extend(self._filter.filter(None))
                    if self._pending:
                        break
                return None
            if not packet.size:  # PyAV's synthetic decoder-flush packets.
                continue
            self._pending.extend(self._filter.filter(packet) if self._filter else [packet])
        return self._pending.popleft()

    def _wrap(self, packet):
        if packet is None:
            result = rocpydec.PyPacketData()
            result.end_of_stream = True
            result.pkt_flags = int(rocpydec.decTypes.ROCDEC_PKT_ENDOFSTREAM)
            return result
        time_base = packet.time_base or self._stream.time_base
        pts = int(packet.pts * time_base * 1000) if packet.pts is not None else 0
        result = rocpydec.GetRocPyDecPacket(pts, packet.size, packet)
        if packet.pts is None:
            result.pkt_flags = 0
        # Retain original timestamp units for PyAV CPU decoding; the public field
        # preserves rocPyDecode's millisecond clock for the GPU decoder.
        result._av_packet = packet
        return result

    def DemuxFrame(self):
        with self._lock:
            self._check_open()
            return self._wrap(self._next_packet())

    def SeekFrame(self, frame_number, seek_mode, seek_criteria):
        with self._lock:
            self._check_open()
            value = operator.index(frame_number)
            if value < 0 or seek_mode not in (0, 1) or seek_criteria not in (0, 1):
                raise ValueError("Invalid seek value, mode or criteria")
            stream = self._stream
            if seek_criteria == 0:
                if not stream.average_rate or stream.average_rate != stream.base_rate:
                    raise ValueError("Frame-number seeking requires a constant known frame rate; use timestamps")
                seconds = Fraction(value, 1) / stream.average_rate
            else:
                seconds = Fraction(value, 1)  # Timestamp criterion uses seconds.
            target = int(seconds / stream.time_base) + (stream.start_time or 0)
            self._container.seek(target, stream=stream, backward=True, any_frame=seek_mode == 0)
            if self._filter:
                self._filter.flush()
            self._pending.clear()
            self._drained = False
            self._packets = iter(self._container.demux(stream))
            packet = self._next_packet()
            # Exact mode selects packets by DTS. Inter-predicted packets still
            # require their reference frames for decoding.
            if seek_mode == 0:
                while packet is not None:
                    timestamp = packet.dts if packet.dts is not None else packet.pts
                    if timestamp is None or timestamp >= target:
                        break
                    packet = self._next_packet()
            return self._wrap(packet)

    def close(self):
        with self._lock:
            if not self._closed:
                self._closed = True
                # Dispose of the suspended iterator before freeing its container.
                self._packets.close()
                self._pending.clear()
                self._filter = None
                self._container.close()

    def __enter__(self):
        with self._lock:
            self._check_open()
            return self

    def __exit__(self, *args):
        self.close()
