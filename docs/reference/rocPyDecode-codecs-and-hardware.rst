.. meta::
  :description: rocPyDecode codec and GPU support
  :keywords: rocPyDecode, rocDecode, GPU, codec

rocPyDecode supported codecs and hardware
=========================================

The PyAV demuxer accepts H.264 (AVC), H.265 (HEVC), VP9, and AV1. Actual decoding
support, bit depths, and maximum dimensions depend on the GPU and rocDecode SDK.
Consult the `rocDecode codec and hardware tables
<https://rocm.docs.amd.com/projects/rocDecode/en/latest/reference/rocDecode-formats-and-architectures.html>`_
for the relevant SDK release.

Use ``decoder.IsCodecSupported(device_id, codec_id, bit_depth)`` to check the
selected device, as shown in :doc:`../how-to/using-rocPydecode`.
Compiling for a GPU target does not add hardware decoding capabilities. See
:doc:`../install/rocPyDecode-cmake-install` for build target selection.
