# rocPyVideoDecode

Python bindings and samples for the ROCm rocDecode library.

Component-owned documentation is available in [`docs`](docs/index.rst).

## Build and test

```bash
cmake -S . -B build -DPYTHON_VERSION_SUGGESTED=3.12
cmake --build build --parallel
cmake --install build --prefix "$PWD/install"
ctest --test-dir build --output-on-failure
```

The build requires ROCm, rocDecode development and utility sources, DLPack,
pybind11, and Python development files. FFmpeg and the rocDecode host library
enable the demuxer and CPU-backend paths; those paths are omitted when their
complete dependency set is unavailable.

Set `ROCM_PATH` when ROCm is not installed at `/opt/rocm`. Build-tree Python
imports use `build/rocpydecode_<major>_<minor>/lib` for the native module and
`build/rocpydecode_<major>_<minor>` for `pyRocVideoDecode`.

The default CTest suite validates the binding types and direct raw H.264/H.265
GPU decoding. FFmpeg-, host-library-, PyTorch-, hip-python-, and VAAPI-sensitive
samples remain available for explicit use after their optional dependencies
are installed, but are not registered as default CTests.
