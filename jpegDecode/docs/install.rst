Install rocPyJpegDecode
=======================

The component requires ROCm, rocJPEG development files, DLPack, pybind11,
Python development files, CMake 3.15 or newer, and a C++17 compiler.

Configure, build, install, and test the component from ``jpegDecode``:

.. code-block:: shell

   cmake -S . -B build \
       -DCMAKE_INSTALL_PREFIX="$PWD/install" \
       -DPYTHON_VERSION_SUGGESTED=3.12
   cmake --build build --parallel
   cmake --install build
   ctest --test-dir build --output-on-failure

Set ``ROCM_PATH`` when ROCm is installed somewhere other than ``/opt/rocm``.
Runtime tests are registered when rocJPEG test images are available under
``${ROCM_PATH}/share/rocjpeg/images``.
