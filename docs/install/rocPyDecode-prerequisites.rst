.. meta::
  :description: rocPyDecode prerequisites
  :keywords: prerequisites, rocPyDecode, rocDecode, rocJPEG, ROCm

Prerequisites
=============

Both components require ROCm, AMD Clang, CMake 3.15 or newer, C++17, Python
development files, pybind11, and DLPack headers.

``videoDecode`` additionally requires rocDecode development files and its
utility sources. FFmpeg development libraries and the rocDecode host library
enable the optional demuxer and CPU backend.

``jpegDecode`` requires rocJPEG development files. Component runtime tests are
registered only when the corresponding rocDecode videos or rocJPEG images are
installed.
