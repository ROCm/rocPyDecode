# Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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
 
# rocDecVideoSurfaceFormat
from rocPyDecode.decTypes import rocDecVideoSurfaceFormat_NV12         
from rocPyDecode.decTypes import rocDecVideoSurfaceFormat_P016          
from rocPyDecode.decTypes import rocDecVideoSurfaceFormat_YUV444       
from rocPyDecode.decTypes import rocDecVideoSurfaceFormat_YUV444_16Bit 

# Surface Memory Types
from rocPyDecode.decTypes import OUT_SURFACE_MEM_DEV_INTERNAL
from rocPyDecode.decTypes import OUT_SURFACE_MEM_DEV_COPIED
from rocPyDecode.decTypes import OUT_SURFACE_MEM_HOST_COPIED

# Video Packet Flags
from rocPyDecode.decTypes import ROCDEC_PKT_ENDOFSTREAM
from rocPyDecode.decTypes import ROCDEC_PKT_TIMESTAMP
from rocPyDecode.decTypes import ROCDEC_PKT_DISCONTINUITY
from rocPyDecode.decTypes import ROCDEC_PKT_ENDOFPICTURE
from rocPyDecode.decTypes import ROCDEC_PKT_NOTIFY_EOS

# Video Codecs
from rocPyDecode.decTypes import rocDecVideoCodec_MPEG1
from rocPyDecode.decTypes import rocDecVideoCodec_MPEG2
from rocPyDecode.decTypes import rocDecVideoCodec_MPEG4
from rocPyDecode.decTypes import rocDecVideoCodec_AVC
from rocPyDecode.decTypes import rocDecVideoCodec_HEVC
from rocPyDecode.decTypes import rocDecVideoCodec_AV1
from rocPyDecode.decTypes import rocDecVideoCodec_VP8
from rocPyDecode.decTypes import rocDecVideoCodec_VP9
from rocPyDecode.decTypes import rocDecVideoCodec_JPEG
from rocPyDecode.decTypes import rocDecVideoCodec_NumCodecs
from rocPyDecode.decTypes import rocDecVideoCodec_YUV420
from rocPyDecode.decTypes import rocDecVideoCodec_YV12
from rocPyDecode.decTypes import rocDecVideoCodec_NV12
from rocPyDecode.decTypes import rocDecVideoCodec_YUYV
from rocPyDecode.decTypes import rocDecVideoCodec_UYVY

_known_types = {

    rocDecVideoSurfaceFormat_NV12: ("rocDecVideoSurfaceFormat_NV12", rocDecVideoSurfaceFormat_NV12),
    rocDecVideoSurfaceFormat_P016: ("rocDecVideoSurfaceFormat_P016", rocDecVideoSurfaceFormat_P016),
    rocDecVideoSurfaceFormat_YUV444: ("rocDecVideoSurfaceFormat_YUV444", rocDecVideoSurfaceFormat_YUV444),
    rocDecVideoSurfaceFormat_YUV444_16Bit: ("rocDecVideoSurfaceFormat_YUV444_16Bit", rocDecVideoSurfaceFormat_YUV444_16Bit),

    OUT_SURFACE_MEM_DEV_INTERNAL: ("OUT_SURFACE_MEM_DEV_INTERNAL", OUT_SURFACE_MEM_DEV_INTERNAL),
    OUT_SURFACE_MEM_DEV_COPIED: ("OUT_SURFACE_MEM_DEV_COPIED", OUT_SURFACE_MEM_DEV_COPIED),
    OUT_SURFACE_MEM_HOST_COPIED: ("OUT_SURFACE_MEM_HOST_COPIED", OUT_SURFACE_MEM_HOST_COPIED),
    
    ROCDEC_PKT_ENDOFSTREAM: ("ROCDEC_PKT_ENDOFSTREAM", ROCDEC_PKT_ENDOFSTREAM),
    ROCDEC_PKT_TIMESTAMP: ("ROCDEC_PKT_TIMESTAMP", ROCDEC_PKT_TIMESTAMP),
    ROCDEC_PKT_DISCONTINUITY: ("ROCDEC_PKT_DISCONTINUITY", ROCDEC_PKT_DISCONTINUITY),
    ROCDEC_PKT_ENDOFPICTURE: ("ROCDEC_PKT_ENDOFPICTURE", ROCDEC_PKT_ENDOFPICTURE),
    ROCDEC_PKT_NOTIFY_EOS: ("ROCDEC_PKT_NOTIFY_EOS", ROCDEC_PKT_NOTIFY_EOS),

    rocDecVideoCodec_MPEG1: ("rocDecVideoCodec_MPEG1",rocDecVideoCodec_MPEG1),  
    rocDecVideoCodec_MPEG2: ("rocDecVideoCodec_MPEG2",rocDecVideoCodec_MPEG2),
    rocDecVideoCodec_MPEG4: ("rocDecVideoCodec_MPEG4",rocDecVideoCodec_MPEG4),
    rocDecVideoCodec_AVC: ("rocDecVideoCodec_AVC",rocDecVideoCodec_AVC),
    rocDecVideoCodec_HEVC: ("rocDecVideoCodec_HEVC",rocDecVideoCodec_HEVC),
    rocDecVideoCodec_AV1: ("rocDecVideoCodec_AV1",rocDecVideoCodec_AV1),
    rocDecVideoCodec_VP8: ("rocDecVideoCodec_VP8",rocDecVideoCodec_VP8),
    rocDecVideoCodec_VP9: ("rocDecVideoCodec_VP9",rocDecVideoCodec_VP9),
    rocDecVideoCodec_JPEG: ("rocDecVideoCodec_JPEG",rocDecVideoCodec_JPEG),
    rocDecVideoCodec_NumCodecs: ("rocDecVideoCodec_NumCodecs",rocDecVideoCodec_NumCodecs),
    rocDecVideoCodec_YUV420: ("rocDecVideoCodec_YUV420",rocDecVideoCodec_YUV420),
    rocDecVideoCodec_YV12: ("rocDecVideoCodec_YV12",rocDecVideoCodec_YV12),
    rocDecVideoCodec_NV12: ("rocDecVideoCodec_NV12",rocDecVideoCodec_NV12),
    rocDecVideoCodec_YUYV: ("rocDecVideoCodec_YUYV",rocDecVideoCodec_YUYV),
    rocDecVideoCodec_UYVY: ("rocDecVideoCodec_UYVY",rocDecVideoCodec_UYVY),
}

def data_type_function(dtype):
    if dtype in _known_types:
        ret = _known_types[dtype][0]
        return ret
    else:
        raise RuntimeError(str(dtype) + " does not correspond to a known type.")