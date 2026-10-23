Build and install rocPyDecode from source
=========================================

Prepare the SDK, development dependencies, and media described in
:doc:`rocPyDecode-prerequisites` before configuring.

GPU targets
-----------

Building and installing do not require a GPU on the build machine. By default,
the video component uses the 25 GPU targets listed in ``videoDecode/CMakeLists.txt``.
The selected targets are printed during configuration. The default build
requires a compiler supporting all 25 targets. The minimum ROCm 7.0 / AMD
Clang 18 prerequisites do not guarantee support for the full default list.
The full list has been verified with AMD Clang 24 from a ROCm development SDK.
CMake compiles a small kernel for each selected target and stops configuration
if any check fails, listing the failed targets. It never removes targets
automatically.

With an older toolchain, explicitly select targets supported by its compiler
and matching the GPUs where the bindings will run. For example, if the compiler
supports all three targets below:

.. code-block:: shell

   cmake -S . -B build -DGPU_TARGETS="gfx90a;gfx942;gfx1100"

An explicit ``GPU_TARGETS`` list replaces the defaults and is retained in the
CMake cache. These settings also apply to standalone video builds.
The JPEG binding links the HIP host runtime and does not compile device kernels, so
it does not require GPU target selection. Its GPU coverage comes from the
installed rocJPEG SDK.
GPU decoding and runtime tests still require compatible
hardware, drivers, and SDK libraries; compiling a target does not add decoding
capabilities absent from the GPU or SDK.

The 25-target default applies to fresh builds without an explicit target selection.
Existing build directories retain their cached targets, including targets
previously detected by HIP. To change them, pass ``-DGPU_TARGETS=...`` explicitly.
To use the current defaults, configure a new build directory without
``GPU_TARGETS`` or the legacy ``AMDGPU_TARGETS`` override.

Build, install, and test
------------------------

Obtain the source and enter the repository:

.. code-block:: shell

   git clone https://github.com/ROCm/rocPyDecode.git
   cd rocPyDecode

After preparing :doc:`rocPyDecode-prerequisites`, run:

.. code-block:: shell

   cmake -S . -B build \
       -DCMAKE_INSTALL_PREFIX="$PWD/install" \
       -DPYTHON_VERSION_SUGGESTED=3.12
   cmake --build build --parallel
   cmake --install build
   # Requires a supported GPU and test media. Skip on build-only machines.
   ctest --test-dir build --output-on-failure -V --no-tests=error


The local install prefix does not require ``sudo``. Replace ``build`` and ``install``
consistently if you need separate host, container, or SDK-specific builds.
CTest uses the build-tree bindings; ``-V`` also displays successful tests' output.

Both components are enabled by default. Set ``-DBUILD_VIDEO_DECODE=OFF`` or
``-DBUILD_JPEG_DECODE=OFF`` to omit a component and its dependencies. For a
standalone build, run the same commands directly inside ``videoDecode`` or
``jpegDecode``.

For a root build with both components available, testing enabled, and all test
assets present, the suite covers binding types, video demuxing and GPU decoding,
batched JPEG decoding, video/JPEG regression checks, JPEG input conversion,
and configuration regressions for missing SDK utilities and GPU target selection.
The configure suite also runs when video is skipped or disabled, provided a
Python 3 interpreter is available. Set
``-DBUILD_TESTING=OFF`` to omit test registration.

CTest always registers the Python API smoke test and PyAV demuxing/GPU pixel
regression for the video component. They check their optional dependencies and
MP4 fixture at runtime and report missing prerequisites as skipped tests.
Install the missing packages or MP4 fixture and rerun CTest; no reconfiguration
is needed. The Python API smoke test runs in Debug and Release builds.
Builds with ``BUILD_TESTING=ON`` also run native buffer, DLPack, and RGB
allocation checks in both build types.

JPEG tests are registered during configuration only when their assets are
present. Install missing JPEG media and rerun CMake before CTest.
A passing run with omitted or skipped tests does not establish full runtime coverage.
Check that the video API test reports a positive decoded-frame count; zero
frames is a failure.
Check that the JPEG test processes images and reports zero bad files.

Test the installed files using :ref:`installed-package-tests`.
To use the bindings in an application, follow :ref:`installed-python-bindings`.

Standalone component builds
---------------------------

The same configure, build, and install commands work from ``videoDecode`` or
``jpegDecode`` without the other component. Install only that component's SDK
requirements. Its local ``third_party`` directory supplies its bundled headers.

For build-tree imports in a standalone Python 3.12 build, add
``build/rocpydecode_3_12/lib`` and ``build/rocpydecode_3_12`` to ``PYTHONPATH`` for
video, or ``build/rocpyjpegdecode_3_12/lib`` and ``build/rocpyjpegdecode_3_12`` for
JPEG. In a combined build these paths are under ``build/videoDecode`` and
``build/jpegDecode``, respectively. Adjust the version suffix for your Python.
CTest sets the appropriate paths automatically.

Missing rocDecode utility sources
---------------------------------

Video builds use utility sources under ``$ROCM_PATH/share/rocdecode/utils``.
To use compatible external sources, configure with
``-DROCPYVIDEO_UTILS_DIR=/path/to/rocDecode/utils``. Sources remain external and
must match the installed rocDecode headers and libraries.

If required files are absent, CMake lists them and skips video targets, tests,
and installation. An explicit override does not fall back to the SDK directory.
JPEG still builds when enabled. If no component remains, configuration warns
that nothing will be built. Check the configuration output before treating a
successful command as a successful video build.

The build option is retained in the cache. Correct the utility path and
reconfigure to restore video support. ``BUILD_VIDEO_DECODE=OFF`` instead skips
video configuration explicitly.
