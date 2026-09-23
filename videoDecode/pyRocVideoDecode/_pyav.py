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

"""Shared PyAV runtime loading and codec translation."""
import rocpydecode as native

_NAMES = {"mpeg1video": "MPEG1", "mpeg2video": "MPEG2", "mpeg4": "MPEG4",
          "h264": "AVC", "hevc": "HEVC", "mjpeg": "JPEG", "vp8": "VP8",
          "vp9": "VP9", "av1": "AV1"}
_ALIASES = {"h265": "hevc", "mpeg1": "mpeg1video", "mpeg2": "mpeg2video"}
# Stable numeric codec IDs exposed by demuxer.GetCodecId().
_CODEC_IDS = {1: "mpeg1video", 2: "mpeg2video", 7: "mjpeg", 12: "mpeg4",
              27: "h264", 139: "vp8", 167: "vp9", 173: "hevc", 225: "av1"}

def require_av():
    try:
        import av
    except ImportError as error:
        raise ImportError("Demuxing and CPU decoding require PyAV: "
                          "python -m pip install 'av>=18.1,<19'") from error
    if tuple(map(int, av.__version__.split(".")[:2])) < (18, 1):
        raise ImportError("rocPyDecode requires PyAV 18.1 or newer")
    return av

def codec_name(value):
    if isinstance(value, native.decTypes.rocDecVideoCodec):
        for name, suffix in _NAMES.items():
            if value == getattr(native.decTypes.rocDecVideoCodec, "rocDecVideoCodec_" + suffix):
                return name
    elif isinstance(value, str):
        name = _ALIASES.get(value.lower(), value.lower())
        if name in _NAMES:
            return name
    elif isinstance(value, int):
        if value in _CODEC_IDS:
            return _CODEC_IDS[value]
    raise ValueError(f"Unsupported video codec: {value!r}")

def codec_id(value):
    name = codec_name(value)
    return getattr(native.decTypes.rocDecVideoCodec, "rocDecVideoCodec_" + _NAMES[name])
