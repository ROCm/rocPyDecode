.. meta::
  :description: rocPyDecode surface memory locations and lifetimes
  :keywords: rocPyDecode, decoded frames, device memory, host memory

rocPyDecode surface data memory locations
=========================================

The video decoder's ``mem_type`` argument selects how decoded YUV surfaces
are exposed:

.. list-table:: Output surface memory types
   :header-rows: 1
   :widths: 10 35 55

   * - Value
     - Enum
     - Location
   * - 0
     - ``OUT_SURFACE_MEM_DEV_INTERNAL``
     - Borrowed internal decoder device surfaces; the Python decoder default.
   * - 1
     - ``OUT_SURFACE_MEM_DEV_COPIED``
     - Output copied to device memory.
   * - 2
     - ``OUT_SURFACE_MEM_HOST_COPIED``
     - Output copied to host memory for CPU access.
   * - 3
     - ``OUT_SURFACE_MEM_NOT_MAPPED``
     - Decode without mapping output pixels, for decode-only measurements.

GPU decoding is used even when the output is copied to host memory.
Internal surfaces belong to the decoder: retrieve and process each frame before
calling ``ReleaseFrame(packet)``, and do not access borrowed storage afterward.
This mode does not imply that every conversion or framework export is zero-copy.
See the :doc:`decoder API <../reference/decoderClass>` for RGB conversion,
resizing, and buffer lifetime requirements.
