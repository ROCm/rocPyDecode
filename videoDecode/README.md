# rocPyVideoDecode

Python bindings and samples for the ROCm rocDecode library.

## Prerequisites

Use Linux and an AMD GPU supported by the selected ROCm release and the
underlying decoder library. Install a complete ROCm 7.0 or newer SDK with
AMD Clang 18 or newer and C++17 support. See the
[ROCm installation guide](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/) for OS, GPU, driver,
and repository setup. The Ubuntu package examples below target Ubuntu 24.04
and Python 3.12; use packages appropriate to your OS and selected ROCm release.

Required build dependencies:

- CMake 3.20 or newer for the commands below, including `ctest --test-dir`.
  The CMake project itself accepts CMake 3.15 or newer.
- Python 3.9 or newer and matching development headers/libraries. The examples
  select Python 3.12; change `PYTHON_VERSION_SUGGESTED` to your installed version.
- pybind11 3.1.0 and DLPack 1.3 headers; bundled in the repository.
- rocDecode **1.0.0 or newer**, including development files, CMake package
  configuration, and utility sources under `share/rocdecode/utils`.

Install the build tools after configuring the appropriate ROCm repositories:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config \
    python3.12-dev
sudo apt-get install -y rocdecode-dev rocdecode-test
```

The `-dev` packages supply build dependencies; the `-test` packages request
runtime test assets. Packaging varies between SDK distributions: verify the
media paths below even after installing these packages. For an SDK archive,
install its complete development and test assets into the same SDK prefix.
A particular nightly SDK or container is not required.

Check the candidate versions with `apt-cache policy` before installing. An old
ROCm repository can offer decoder packages below the required version; merely
installing those packages will not satisfy CMake. Select a compatible complete
SDK instead of mixing decoder libraries from one release with another SDK.
Run dependency setup in the environment where you will build and test:
packages installed inside a container do not provision the bare-metal host.

### Optional video features

FFmpeg development libraries enable demuxing. The matching rocDecode host
library and its utility sources additionally enable the CPU backend:

```bash
sudo apt-get install -y libavcodec-dev libavformat-dev libavutil-dev \
    libswscale-dev rocdecode-host
```

These paths are omitted when their complete dependencies are unavailable.
The default raw-video tests do not require them.

NumPy, ROCm-compatible PyTorch, and hip-python are needed only by samples that
use them, not by the native build or default CTest suite. Install those optional
packages according to the selected sample and SDK version.

## Bundled build dependencies

Each component includes pybind11 3.1.0 and DLPack 1.3 in its own `third_party/`.
CMake uses these committed copies without downloading dependencies or searching
system installations. No separate pybind11 or DLPack installation is required;
the ROCm SDK, decoder libraries, and Python development files are still required.

The videoDecode and jpegDecode directories each contain a complete third_party
copy. Either component can be copied and built independently. Combined builds
initialize the identical dependency targets once.
A parent project can add rocPyDecode with add_subdirectory; it must add it before
creating conflicting pybind11 or DLPack targets. The build reports such conflicts
instead of silently substituting another dependency version.

Dependency licenses are included under third_party and installed with the
project documentation.

## Select the SDK and check test assets

Run from the project directory. Set `ROCM_PATH` to the complete SDK prefix;
`/opt/rocm` is the default. Activate your intended Python environment first.

```bash
export ROCM_PATH=/opt/rocm
export PATH="$ROCM_PATH/bin:$ROCM_PATH/lib/llvm/bin:$PATH"
export CMAKE_PREFIX_PATH="$ROCM_PATH${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
export LD_LIBRARY_PATH="$ROCM_PATH/lib:$ROCM_PATH/lib/rocm_sysdeps/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

"$ROCM_PATH/bin/rocminfo"
test -d "$ROCM_PATH/share/rocdecode/utils/rocvideodecode"
ls "$ROCM_PATH/share/rocdecode/video/AMD_driving_virtual_20-H264.264"
ls "$ROCM_PATH/share/rocdecode/video/AMD_driving_virtual_20-H265.265"
```

The `rocm_sysdeps/lib` directory is used by SDK distributions that bundle
runtime dependencies; it may be absent in a system-package installation.
Ensure the user can access the GPU devices. In a container, the host driver
and GPU device access must also be available to that container.

A missing decoder CMake package requires the corresponding development
package or a corrected SDK prefix. Changing `CMAKE_PREFIX_PATH` cannot supply
an absent or incompatible library. After changing SDKs, Python environments,
or moving between a container and the host, use a new build directory to avoid
reusing cached compiler and dependency paths.

## Build, install, and test

Run these commands from the project directory after preparing the prerequisites:

```bash
cmake -S . -B build \
    -DCMAKE_INSTALL_PREFIX="$PWD/install" \
    -DPYTHON_VERSION_SUGGESTED=3.12
cmake --build build --parallel
cmake --install build
ctest --test-dir build --output-on-failure -V
```

The local install prefix does not require `sudo`. Replace `build` and `install`
consistently if you need separate host, container, or SDK-specific builds.
CTest uses the build-tree bindings; `-V` also displays successful tests' output.

With both raw videos available, expect **four tests**: binding types, raw
H.264 decoding, raw H.265 decoding, and frame-limit/error regressions.

Media-dependent tests are registered during configuration only when their
assets are present. A passing run with fewer tests does not establish full
runtime coverage. Install the missing media and rerun CMake before CTest.
Check that each raw-video test reports a positive decoded-frame count; zero
frames is a failure.

## Use the installed bindings

Keep the selected SDK's runtime library path from the setup above. For the
local install prefix and Python 3.12 used here:

```bash
export PYTHONPATH="$PWD/install/lib${PYTHONPATH:+:$PYTHONPATH}"
python3.12 -c 'import rocpydecode; import pyRocVideoDecode.decoder; print("Installed bindings imported successfully")'
```

If you change `CMAKE_INSTALL_LIBDIR` or the Python version, adjust these paths
and the interpreter accordingly.

For build-tree imports, add `build/rocpydecode_3_12/lib` and
`build/rocpydecode_3_12` to `PYTHONPATH`.

See the [component documentation](docs/index.rst) for samples and API details.
