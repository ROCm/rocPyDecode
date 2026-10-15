CMake installation
==================

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

Both components are enabled by default. Set ``-DBUILD_VIDEO_DECODE=OFF`` or
``-DBUILD_JPEG_DECODE=OFF`` to omit a component and its dependencies. For a
standalone build, run the same commands directly inside ``videoDecode`` or
``jpegDecode``.

For a root build with both components available, testing enabled, and all test
assets present, the core suite covers binding types, raw H.264,
raw H.265, batched JPEG decoding, video/JPEG regression checks, JPEG input conversion,
and configuration regressions for missing SDK utilities and GPU target selection.
The configure suite also runs when video is skipped or disabled, provided a
Python 3 interpreter is available. Set
``-DBUILD_TESTING=OFF`` to omit test registration.

When the optional CPU backend and H.264 MP4 fixture are available, CTest also
registers the debug API smoke test and, when NumPy is available, the CPU API
smoke test. The debug test is reported as skipped in builds that define
``NDEBUG``, including Release and RelWithDebInfo; it must run in Debug builds.

Media-dependent tests are registered during configuration only when their
assets are present. A passing run with fewer tests does not establish full
runtime coverage. Install the missing media and rerun CMake before CTest.
Check that each raw-video test reports a positive decoded-frame count; zero
frames is a failure.
Check that the JPEG test processes images and reports zero bad files.

Test an installed package
-------------------------

Packages built with ``BUILD_TESTING=ON`` (the default) install a CTest setup for
each component. No source checkout, compiler, or Python development headers
are needed to run these installed tests. Use CMake 3.20 or newer, a Python
interpreter matching the installed extension, compatible ROCm runtime libraries,
a supported GPU, and the SDK test media described above.

Set ``prefix`` to the installed or extracted package directory. CMake discovers
Python from the active environment and uses ``ROCM_PATH`` from the environment
when set, otherwise ``/opt/rocm``. Python must match the installed extension.

.. code-block:: shell

   prefix=/path/to/installed/package
   cmake -S "$prefix/share/rocpydecode/tests" -B video-tests
   ctest --test-dir video-tests -V --no-tests=error

   cmake -S "$prefix/share/rocpyjpegdecode/tests" -B jpeg-tests
   ctest --test-dir jpeg-tests -V --no-tests=error


Run only the commands for installed components. From a configured test build
directory, you can also run ``ctest -V``. CMake sets the test Python path to the
selected package's bindings. Adjust ``share`` if the package uses a custom data
directory; the test setup retains the relative library/data layout when moved.

Optional settings for non-default installations:

- Add ``-DPython3_EXECUTABLE=/path/to/python`` to the CMake command to select a
  different interpreter, or activate its virtual environment before configuring.
- Add ``-DROCM_PATH=/path/to/rocm`` to select a different SDK and its test media.
- If importing the bindings reports a missing shared library, make that library
  available through the runtime loader configuration. For a custom SDK, this
  may require ``LD_LIBRARY_PATH`` pointing to its library directories. CMake's
  ``ROCM_PATH`` setting selects test media; it does not change the runtime loader's
  search paths. No additional library-path export is needed when the installed
  runtime libraries are already discoverable.

Installed configuration fails if the bindings cannot be imported, either required
raw video is missing, or no JPEG ``*.jpg`` fixtures are found. To use media outside
the SDK prefix, set ``ROCPYVIDEO_MEDIA_DIR`` or ``ROCPYJPEG_MEDIA_DIR`` when configuring.
Use a fresh test build directory after moving a package or changing Python or SDKs.
The source-configuration regression suite remains a source-build test.

Use the installed bindings
--------------------------

Keep the selected SDK's runtime library path from the setup above. For the
local install prefix and Python 3.12 used here:

.. code-block:: shell

   export PYTHONPATH="$PWD/install/lib${PYTHONPATH:+:$PYTHONPATH}"
   python3.12 -c 'import rocpydecode, rocpyjpegdecode; import pyRocVideoDecode.decoder, pyRocJpegDecode.decoder; print("Installed bindings imported successfully")'


If you change ``CMAKE_INSTALL_LIBDIR`` or the Python version, adjust these paths
and the interpreter accordingly.
