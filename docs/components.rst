Video and JPEG components
=========================

``videoDecode`` provides GPU video decoding through rocDecode and container
demuxing through PyAV. ``jpegDecode`` provides GPU JPEG decoding through rocJPEG.
The native bindings and JPEG decoding do not require PyAV.

Both components support independent builds. The repository root builds both by
default; see :doc:`install/rocPyDecode-cmake-install` for component selection.

* :doc:`Video API <reference/rocPyDecode>`
* :doc:`JPEG API <reference/rocPyJPEGDecode-api>`
* :doc:`Video memory types <conceptual/rocPyDecode-mem-types>`
* :doc:`Supported video codecs and hardware <reference/rocPyDecode-codecs-and-hardware>`
* :doc:`Using the video API <how-to/using-rocPydecode>`
* :doc:`Samples and tests <tutorials/rocPyDecode-samples>`
