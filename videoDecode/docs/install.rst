Install rocPyVideoDecode
========================

Prerequisites
-------------

Use Linux. GPU decoding and runtime tests require an AMD GPU supported by the
selected ROCm release and decoder library; building and installing do not require
a GPU. Install a complete ROCm 7.0 or newer SDK with
AMD Clang 18 or newer and C++17 support. See the
`ROCm installation guide <https://rocm.docs.amd.com/projects/install-on-linux/en/latest/>`_ for OS, GPU, driver,
and repository setup. The Ubuntu package examples below target Ubuntu 24.04
and Python 3.12; use packages appropriate to your OS and selected ROCm release.

The minimum ROCm/compiler versions above apply only when the compiler supports
the selected GPU targets; they do not guarantee support for all 25 defaults.
The default build requires a compiler that supports the entire default list.
With an older compiler, pass an explicit ``GPU_TARGETS`` list to CMake,
for example ``-DGPU_TARGETS=gfx1100`` for a compiler supporting gfx1100.
Choose targets for the GPUs where the bindings will run.

Required build dependencies:

- CMake 3.20 or newer for the commands below, including ``ctest --test-dir``.
  The CMake project itself requires CMake 3.18 or newer for Python module discovery.
- Python 3.9 or newer and matching development headers. The examples
  select Python 3.12; change ``PYTHON_VERSION_SUGGESTED`` to your installed version.
- pybind11 3.1.0 and DLPack 1.3 headers; bundled in the repository.
- rocDecode **1.0.0 or newer**, including development files, CMake package
  configuration, and utility sources under ``share/rocdecode/utils``.

Install the build tools after configuring the appropriate ROCm repositories:

.. code-block:: shell

   sudo apt-get update
   sudo apt-get install -y build-essential cmake pkg-config \
       python3.12-dev
   sudo apt-get install -y rocdecode-dev rocdecode-test


The ``-dev`` packages supply build dependencies; the ``-test`` packages request
runtime test assets. Packaging varies between SDK distributions: verify the
media paths below even after installing these packages. For an SDK archive,
install its complete development and test assets into the same SDK prefix.
A particular nightly SDK or container is not required.

Check the candidate versions with ``apt-cache policy`` before installing. An old
ROCm repository can offer decoder packages below the required version; merely
installing those packages will not satisfy CMake. Select a compatible complete
SDK instead of mixing decoder libraries from one release with another SDK.
Run dependency setup in the environment where you will build and test:
packages installed inside a container do not provision the bare-metal host.

Optional video features
~~~~~~~~~~~~~~~~~~~~~~~

FFmpeg development libraries enable demuxing. The matching rocDecode host
library and its utility sources additionally enable the CPU backend:

.. code-block:: shell

   sudo apt-get install -y libavcodec-dev libavformat-dev libavutil-dev \
       libswscale-dev


The CPU backend additionally requires a matching rocDecode host library and its
FFmpeg decoder utility sources under the selected SDK prefix. Some SDK
repositories do not provide a separate rocdecode-host package. Check
``apt-cache policy rocdecode-host`` before installing it; if unavailable,
use a complete matching SDK that supplies these files to enable CPU decoding.
Do not mix host libraries from another ROCm release.

GPU decoding and the default CTests do not require the CPU backend.

These paths are omitted when their complete dependencies are unavailable.
The default raw-video tests do not require them.

NumPy, ROCm-compatible PyTorch, and hip-python are needed only by samples that
use them, not by the native build or default CTest suite. Install those optional
packages according to the selected sample and SDK version.

Bundled build dependencies
--------------------------

Each component includes pybind11 3.1.0 and DLPack 1.3 in its own ``third_party/``.
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

Select the SDK and check test assets
------------------------------------

Run from the project directory. Set ``ROCM_PATH`` to the complete SDK prefix;
``/opt/rocm`` is the default. Activate your intended Python environment first.

.. code-block:: shell

   export ROCM_PATH=/opt/rocm
   export PATH="$ROCM_PATH/bin:$ROCM_PATH/lib/llvm/bin:$PATH"
   export CMAKE_PREFIX_PATH="$ROCM_PATH${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
   export LD_LIBRARY_PATH="$ROCM_PATH/lib:$ROCM_PATH/lib/rocm_sysdeps/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

   # Runtime GPU check; skip on build-only machines.
   "$ROCM_PATH/bin/rocminfo"
   test -d "$ROCM_PATH/share/rocdecode/utils/rocvideodecode"
   ls "$ROCM_PATH/share/rocdecode/video/AMD_driving_virtual_20-H264.264"
   ls "$ROCM_PATH/share/rocdecode/video/AMD_driving_virtual_20-H265.265"


The ``rocm_sysdeps/lib`` directory is used by SDK distributions that bundle
runtime dependencies; it may be absent in a system-package installation.
For GPU decoding and runtime tests, ensure the user can access the GPU devices.
In a runtime container, the host driver and GPU device access must also be
available to that container.

A missing decoder CMake package requires the corresponding development
package or a corrected SDK prefix. Changing ``CMAKE_PREFIX_PATH`` cannot supply
an absent or incompatible library. After changing SDKs, Python environments,
or moving between a container and the host, use a new build directory to avoid
reusing cached compiler and dependency paths.

GPU targets
-----------

By default, this component compiles for all 25 targets in
its ``CMakeLists.txt``, without detecting local GPUs.
Pass ``-DGPU_TARGETS="gfx90a;gfx942;gfx1100"`` to select a subset. The compiler
must support the selected targets; an older compiler may require a smaller list.
The selected targets are printed during configuration and retained in the CMake
cache. GPU decoding also requires a compatible installed rocDecode SDK.

The 25-target default applies to fresh builds without an explicit target selection.
Existing build directories retain their cached targets, including targets
previously detected by HIP. To change them, pass ``-DGPU_TARGETS=...`` explicitly.
To use the current defaults, configure a new build directory without
``GPU_TARGETS`` or the legacy ``AMDGPU_TARGETS`` override.

Build, install, and test
------------------------

Run these commands from the project directory after preparing the prerequisites:

.. code-block:: shell

   cmake -S . -B build \
       -DCMAKE_INSTALL_PREFIX="$PWD/install" \
       -DPYTHON_VERSION_SUGGESTED=3.12
   cmake --build build --parallel
   cmake --install build
   ctest --test-dir build --output-on-failure -V


The local install prefix does not require ``sudo``. Replace ``build`` and ``install``
consistently if you need separate host, container, or SDK-specific builds.
CTest uses the build-tree bindings; ``-V`` also displays successful tests' output.

With both raw videos available, tests cover binding types, raw
H.264 decoding, raw H.265 decoding, and frame-limit/error regressions.

Media-dependent tests are registered during configuration only when their
assets are present. A passing run with fewer tests does not establish full
runtime coverage. Install the missing media and rerun CMake before CTest.
Check that each raw-video test reports a positive decoded-frame count; zero
frames is a failure.

Use the installed bindings
--------------------------

Keep the selected SDK's runtime library path from the setup above. For the
local install prefix and Python 3.12 used here:

.. code-block:: shell

   export PYTHONPATH="$PWD/install/lib${PYTHONPATH:+:$PYTHONPATH}"
   python3.12 -c 'import rocpydecode; import pyRocVideoDecode.decoder; print("Installed bindings imported successfully")'


If you change ``CMAKE_INSTALL_LIBDIR`` or the Python version, adjust these paths
and the interpreter accordingly.

For build-tree imports, add ``build/rocpydecode_3_12/lib`` and
``build/rocpydecode_3_12`` to ``PYTHONPATH``.
