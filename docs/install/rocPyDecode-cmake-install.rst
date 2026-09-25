CMake installation
==================

Prepare the SDK, development dependencies, and media described in
:doc:`rocPyDecode-prerequisites` before configuring.

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
assets present, the core suite contains **seven tests**: binding types, raw H.264,
raw H.265, batched JPEG decoding, video/JPEG regression checks, and the configure
regression suite. The configure suite also runs when video is skipped or
disabled, provided a Python 3 interpreter is available. Set
``-DBUILD_TESTING=OFF`` to omit test registration.

When PyAV, NumPy and the H.264 MP4 fixture are available, CTest also
registers the PyAV pixel/packet regression, debug API smoke test and CPU API
smoke test. The debug test is reported as skipped in builds that define
``NDEBUG``, including Release and RelWithDebInfo; it must run in Debug builds.

Media-dependent tests are registered during configuration only when their
assets are present. A passing run with fewer tests does not establish full
runtime coverage. Install the missing media and rerun CMake before CTest.
Check that each raw-video test reports a positive decoded-frame count; zero
frames is a failure.
Check that the JPEG test processes images and reports zero bad files.

Use the installed bindings
--------------------------

Keep the selected SDK's runtime library path from the setup above. For the
local install prefix and Python 3.12 used here:

.. code-block:: shell

   export PYTHONPATH="$PWD/install/lib${PYTHONPATH:+:$PYTHONPATH}"
   python3.12 -c 'import rocpydecode, rocpyjpegdecode; import pyRocVideoDecode.decoder, pyRocJpegDecode.decoder; print("Installed bindings imported successfully")'


If you change ``CMAKE_INSTALL_LIBDIR`` or the Python version, adjust these paths
and the interpreter accordingly.
