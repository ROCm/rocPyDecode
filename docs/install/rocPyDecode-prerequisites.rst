Prerequisites
=============

Use Linux. GPU decoding and runtime tests require an AMD GPU supported by the
selected ROCm release and decoder library; building and installing do not require
a GPU. Install a complete ROCm 7.0 or newer SDK with
AMD Clang 18 or newer and C++17 support. See the
`ROCm installation guide <https://rocm.docs.amd.com/projects/install-on-linux/en/latest/>`_ for OS, GPU, driver,
and repository setup. The Ubuntu package examples below target Ubuntu 24.04
and Python 3.12; use packages appropriate to your OS and selected ROCm release.

Video builds require a compiler that supports all selected GPU targets. See
:doc:`rocPyDecode-cmake-install` for the default list and explicit selection.
JPEG compiles host code and uses the installed rocJPEG library's GPU kernels.
Install only the SDK dependencies for the components you intend to build.

Required build dependencies:

- CMake 3.20 or newer for the commands below, including ``ctest --test-dir``.
  The CMake project itself requires CMake 3.18 or newer for Python module discovery.
- Python 3.9 or newer and matching development headers. The examples
  select Python 3.12; change ``PYTHON_VERSION_SUGGESTED`` to your installed version.
- pybind11 3.1.0 and DLPack 1.3 headers; bundled in the repository.
- rocDecode **1.0.0 or newer**, including development files, CMake package
  configuration, and utility sources under ``share/rocdecode/utils``.
- rocJPEG **1.0.0 or newer**, including development files and CMake package
  configuration.

Install the build tools. For Ubuntu 24.04 and Python 3.12:

.. code-block:: shell

   sudo apt-get update
   sudo apt-get install -y build-essential cmake pkg-config python3.12-dev

Install the decoder SDK dependencies
------------------------------------

Follow the `rocDecode installation guide
<https://rocm.docs.amd.com/projects/rocDecode/en/latest/install/rocDecode-install.html>`_
and the `rocJPEG installation guide
<https://rocm.docs.amd.com/projects/rocJPEG/en/latest/install/rocjpeg-install.html>`_
for your selected ROCm release. Use the same release for its runtime,
development files, utility sources, and test media.

For repositories using the current ``amdrocm`` package names, the Ubuntu
video build dependencies include:

.. code-block:: shell

   sudo apt install amdrocm-decode-dev amdrocm-decode-test

The test package supplies utility sources needed to build the video binding,
as well as test media. Installing only the development package is insufficient.
For JPEG development files, install ``amdrocm-jpeg-dev`` and obtain its matching
JPEG test images. On RHEL or SLES, the development suffix is ``-devel`` instead
of ``-dev``; follow the SDK guide for repository and version selection.

Older ROCm repositories use names such as ``rocdecode-dev``, ``rocdecode-test``,
``rocjpeg-dev``, and ``rocjpeg-test``. Use the package names for your release,
not a mixture of release streams. For SDK archives, include the development
and test artifacts in the same SDK prefix. The media locations are listed below.

Check the candidate versions with ``apt-cache policy`` before installing. An old
ROCm repository can offer decoder packages below the required version; merely
installing those packages will not satisfy CMake. Select a compatible complete
SDK instead of mixing decoder libraries from one release with another SDK.
Run dependency setup in the environment where you will build and test:
packages installed inside a container do not provision the bare-metal host.

.. _optional-python-packages:

Optional Python packages
------------------------

Container demuxing requires Python 3.11 or newer and PyAV >=18.1. Newer PyAV
releases may require a newer Python; pip selects a compatible version.
PyAV is installed in the Python environment
that runs rocPyDecode. From the repository root, install it using
``videoDecode/requirements.txt`` in the same Python environment used to
build or run the bindings:

.. code:: shell

   python -m pip install --only-binary=:all: -r videoDecode/requirements.txt

PyAV wheels include their FFmpeg libraries. rocPyDecode does not require
system FFmpeg headers, libraries, an executable, or the rocDecode host library.
Use a prebuilt PyAV wheel compatible with your Python version and platform.
The command above installs only wheels and fails if no compatible PyAV wheel
is available. Building PyAV from source requires additional native dependencies.
GPU decoding still requires HIP, the rocDecode SDK, and its GPU utility sources.
Building the bindings and JPEG decoding do not require PyAV. With video testing enabled,
the Python API and PyAV pixel tests are always registered. At runtime they report
skipped if compatible PyAV or the H.264 MP4 test media are missing; the pixel test
also needs NumPy. Installing these prerequisites does not require rerunning CMake.

The installed test suite does not contain a source checkout. In that case,
install PyAV directly:

.. code-block:: shell

   python -m pip install --only-binary=:all: "av>=18.1"

The video pixel regression also requires NumPy:

.. code-block:: shell

   python -m pip install numpy

NumPy is not required to build the bindings or decode JPEG inputs that do not
use NumPy arrays. ROCm-compatible PyTorch, hip-python, and mpi4py are required
only by samples or additional tests that import them. See
:doc:`../tutorials/rocPyDecode-samples`.

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

.. _sdk-environment:

Select the SDK
--------------

Set ``ROCM_PATH`` to the actual SDK prefix; ``/opt/rocm`` is the default.
Some distributions use a versioned directory instead. Activate your intended
Python environment first. For a custom SDK, the following settings expose its
tools, CMake packages, and runtime libraries:

.. code-block:: shell

   export ROCM_PATH=/opt/rocm
   export PATH="$ROCM_PATH/bin:$ROCM_PATH/lib/llvm/bin:$PATH"
   export CMAKE_PREFIX_PATH="$ROCM_PATH${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
   export LD_LIBRARY_PATH="$ROCM_PATH/lib:$ROCM_PATH/lib/rocm_sysdeps/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

   # Runtime GPU check; skip on build-only machines.
   "$ROCM_PATH/bin/rocminfo"


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

.. _sdk-test-media:

Locate utility sources and test media
-------------------------------------

Check only the paths for the components you will build or test:

.. code-block:: shell

   # Video build utility sources
   test -d "$ROCM_PATH/share/rocdecode/utils/rocvideodecode"
   # Video runtime fixture
   ls "$ROCM_PATH/share/rocdecode/video/AMD_driving_virtual_20-H264.mp4"
   # JPEG runtime fixtures (top-level .jpg files)
   ls "$ROCM_PATH/share/rocjpeg/images/"*.jpg

Installed video tests can use a different media location through
``-DROCPYVIDEO_MEDIA_DIR=/path/to/video``; JPEG tests use
``-DROCPYJPEG_MEDIA_DIR=/path/to/images``. These CMake settings select directories,
not individual filenames. Missing video prerequisites cause runtime skips;
missing JPEG fixtures prevent installed-test configuration. For source-build
behavior and utility-source overrides, see :doc:`rocPyDecode-cmake-install`.
