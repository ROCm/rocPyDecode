.. meta::
  :description: Install rocPyDecode and validate installed Python bindings
  :keywords: install, rocPyDecode, rocPyJpegDecode, CTest, ROCm

Install rocPyDecode
===================

rocPyDecode provides Python bindings for rocDecode video decoding and rocJPEG
image decoding on Linux. Before you begin, check the
:doc:`rocPyDecode-prerequisites` for compatible hardware, SDK libraries, and
Python versions. Building does not require a GPU; decoding and runtime tests do.

For a source installation or custom GPU targets, see
:doc:`rocPyDecode-cmake-install`. The root build installs both components by
default; either component can also be built independently.

Use a binary installation
-------------------------

If your distribution supplies rocPyDecode binaries, follow that distribution's
installation instructions and install its matching test files to use CTest.
Use a Python interpreter matching the native extensions' version and ABI.
For example, a ``cpython-312`` extension requires CPython 3.12.
The rocDecode and rocJPEG SDK packages supply the underlying libraries;
they do not by themselves install these Python bindings.

Set ``prefix`` to the directory containing the installed ``lib`` and ``share``
subdirectories. Set ``ROCM_PATH`` to the matching SDK prefix. These locations
can differ, including when binaries are extracted into a local directory.

.. code-block:: shell

   prefix=/path/to/installed/package
   export ROCM_PATH=/path/to/rocm

Use the package's configured library paths. If an import reports a missing
shared library, follow :ref:`sdk-environment` to configure a custom SDK.

.. _installed-python-bindings:

Use the installed bindings
--------------------------

For the default installation layout, expose the installed Python modules to
the interpreter in your active environment:

.. code-block:: shell

   export PYTHONPATH="$prefix/lib${PYTHONPATH:+:$PYTHONPATH}"
   python -c 'import rocpydecode; import pyRocVideoDecode.decoder; print("Video bindings imported")'
   python -c 'import rocpyjpegdecode; import pyRocJpegDecode.decoder; print("JPEG bindings imported")'

Run only the import check for components you installed. Adjust ``lib`` if the
package uses a different library directory. For a source installation with
``-DCMAKE_INSTALL_PREFIX="$PWD/install"``, set ``prefix="$PWD/install"`` while
in the repository root.

Importing the bindings does not validate GPU decoding. Install the
:ref:`optional-python-packages` for container demuxing and pixel tests, then
run the installed tests below.

.. _installed-package-tests:

Test an installed package
-------------------------

Packages built with ``BUILD_TESTING=ON`` (the default) install a CTest setup for
each component. No source checkout, compiler, or Python development headers
are needed to run these installed tests. Use CMake 3.20 or newer, a Python
interpreter matching the installed extension, compatible ROCm runtime libraries,
a supported GPU, and the :ref:`SDK test media <sdk-test-media>`.

Set ``prefix`` to the installed or extracted package directory. CMake discovers
Python from the active environment and uses ``ROCM_PATH`` from the environment
when set, otherwise ``/opt/rocm``. Python must match the installed extension.

.. code-block:: shell

   prefix=/path/to/installed/package
   cmake -S "$prefix/share/rocpydecode/tests" -B video-tests
   ctest --test-dir video-tests --output-on-failure -V --no-tests=error

   cmake -S "$prefix/share/rocpyjpegdecode/tests" -B jpeg-tests
   ctest --test-dir jpeg-tests --output-on-failure -V --no-tests=error


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

Installed configuration fails if the bindings cannot be imported or no JPEG
``*.jpg`` fixtures are found. Video API and pixel tests report a CTest skip
when PyAV or the H.264 MP4 fixture is unavailable; the pixel test also needs NumPy.
Skipped tests do not establish decoding correctness. To use media outside the SDK
prefix, set ``ROCPYVIDEO_MEDIA_DIR`` or ``ROCPYJPEG_MEDIA_DIR`` when configuring.
Use a fresh test build directory after moving a package or changing Python or SDKs.
The source-configuration regression suite remains a source-build test.


Check the results
~~~~~~~~~~~~~~~~~

A complete run executes three video tests and three JPEG tests, with no skips.
The video tests cover binding types, the Python demux/decode API, and GPU pixel
correctness. The JPEG tests cover batched decoding, output-layout/error cases,
and input conversion. No PyTorch installation is required for these suites.

.. list-table:: Interpreting CTest results
   :header-rows: 1
   :widths: 25 75

   * - Result
     - Meaning
   * - Passed
     - The registered test completed successfully.
   * - Skipped
     - A runtime prerequisite is missing. Install it and rerun; decoding has
       not been validated by that test.
   * - Failed
     - Inspect the test output. Resolve the reported error before claiming
       validation passed.
   * - No tests found
     - Check the selected test directory and package contents. The commands
       above use ``--no-tests=error`` to treat this as a failure.

If ``share/rocpydecode/tests/CMakeLists.txt`` or
``share/rocpyjpegdecode/tests/CMakeLists.txt`` is absent, that installation does
not contain the installed test setup. Obtain matching test files from its
package provider, or build and install from source with ``BUILD_TESTING=ON``.
