.. meta::
  :description: Build rocPyDecode with CMake
  :keywords: CMake, install, rocPyDecode, rocPyJpegDecode, ROCm

CMake installation
==================

Configure, build, install, and test both components from the repository root:

.. code-block:: shell

  cmake -S . -B build \
      -DCMAKE_INSTALL_PREFIX="$PWD/install" \
      -DPYTHON_VERSION_SUGGESTED=3.12
  cmake --build build --parallel
  cmake --install build
  ctest --test-dir build --output-on-failure

``BUILD_VIDEO_DECODE`` and ``BUILD_JPEG_DECODE`` are enabled by default. Set
either option to ``OFF`` to omit that component. The same command sequence can
be run from ``videoDecode`` or ``jpegDecode`` for a standalone component build.

Set ``ROCM_PATH`` when ROCm is installed somewhere other than ``/opt/rocm``.
