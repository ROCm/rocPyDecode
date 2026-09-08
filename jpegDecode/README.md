# rocPyJpegDecode

Python bindings and samples for the ROCm rocJPEG library.

Component-owned documentation is available in [`docs`](docs/index.rst).

## Build and test

```bash
cmake -S . -B build -DPYTHON_VERSION_SUGGESTED=3.12
cmake --build build --parallel
cmake --install build --prefix "$PWD/install"
ctest --test-dir build --output-on-failure
```

The build requires ROCm, rocJPEG development files, DLPack, pybind11, and
Python development files. Runtime tests are registered when the rocJPEG test
images are available under `${ROCM_PATH}/share/rocjpeg/images`.

The default CTest suite uses the batched decoder and does not require PyTorch.
Torch-dependent and performance samples remain available for explicit use once
their optional dependencies are installed.

Set `ROCM_PATH` when ROCm is not installed at `/opt/rocm`. Build-tree Python
imports use `build/rocpyjpegdecode_<major>_<minor>/lib` for the native module
and `build/rocpyjpegdecode_<major>_<minor>` for `pyRocJpegDecode`.
