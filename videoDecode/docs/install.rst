Install rocPyVideoDecode
========================

The component requires ROCm, rocDecode development and utility sources,
DLPack, pybind11, Python development files, CMake 3.15 or newer, and a C++17
compiler. FFmpeg development libraries and the rocDecode host library enable
the optional demuxer and CPU-backend paths.

Configure, build, install, and test the component from ``videoDecode``:

.. code-block:: shell

   cmake -S . -B build \
       -DCMAKE_INSTALL_PREFIX="$PWD/install" \
       -DPYTHON_VERSION_SUGGESTED=3.12
   cmake --build build --parallel
   cmake --install build
   ctest --test-dir build --output-on-failure

Set ``ROCM_PATH`` when ROCm is installed somewhere other than ``/opt/rocm``.
