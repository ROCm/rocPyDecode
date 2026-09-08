[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

<p align="center"><img width="70%" src="docs/data/AMD_rocPyDecode_Logo.png" alt="AMD rocPyDecode Logo" /></p>

# rocPyDecode

rocPyDecode provides Python bindings for AMD's rocDecode and rocJPEG C/C++
libraries. The repository contains two components:

- [`videoDecode`](videoDecode/README.md): rocDecode video bindings,
  `pyRocVideoDecode`, samples, and tests.
- [`jpegDecode`](jpegDecode/README.md): rocJPEG image bindings,
  `pyRocJpegDecode`, samples, and tests.

Each component supports standalone configuration, build, installation, and
testing. The top-level build supports building both components together.

## Prerequisites

- ROCm and AMD Clang
- CMake 3.15 or newer and a C++17 toolchain
- Python development files and pybind11
- DLPack development headers
- rocDecode development and utility sources for `videoDecode`
- rocJPEG development files for `jpegDecode`
- FFmpeg development files and the rocDecode host library for optional video
  demuxer and CPU-backend support
- rocDecode/rocJPEG test media for the corresponding runtime CTests

Set `ROCM_PATH` if ROCm is not installed at `/opt/rocm`.

## Combined build

```bash
cmake -S . -B build \
    -DCMAKE_INSTALL_PREFIX="$PWD/install" \
    -DPYTHON_VERSION_SUGGESTED=3.12
cmake --build build --parallel
cmake --install build
ctest --test-dir build --output-on-failure
```

Both components are enabled by default. Use `-DBUILD_VIDEO_DECODE=OFF` or
`-DBUILD_JPEG_DECODE=OFF` to build only one component from the repository root.

See each child README and its local `docs` directory for standalone guidance:

- [`videoDecode/docs`](videoDecode/docs/index.rst)
- [`jpegDecode/docs`](jpegDecode/docs/index.rst)

The top-level documentation starts at [`docs/index.rst`](docs/index.rst). The
published documentation is available at
[rocPyDecode](https://rocm.docs.amd.com/projects/rocPyDecode/en/latest/).
