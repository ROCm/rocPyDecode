.. meta::
  :description: rocPyDecode Installation documentation
  :keywords: install, rocPyDecode, AMD, ROCm

********************************************************************
Installation
********************************************************************

rocDecode Python Binding, rocPyDecode, is a tool that allows users to access 
rocDecode APIs in both Python and C/C++ languages. It works by connecting 
Python and C/C++ libraries, enabling function calling and data passing between the two languages. 
The rocpydecode.so library is a wrapper that facilitates the use of rocDecode APIs that are 
written primarily in C/C++ language within Python.

Supported codecs
================

 * H.265 (HEVC) - 8 bit, and 10 bit
 * H.264 (H264) - 8 bit

Prerequisites
=============

* Linux distribution

  * Ubuntu: ``22.04 or above``

* `ROCm-supported hardware <https://rocm.docs.amd.com/projects/install-on-linux/en/latest/reference/system-requirements.html>`_
  (``gfx908`` or higher is required)

* Install ROCm 6.2.0 or later with
  `amdgpu-install <https://rocm.docs.amd.com/projects/install-on-linux/en/latest/how-to/amdgpu-install.html>`_

  * Run: ``--usecase=rocm``
  * To install rocDecode with minimum requirements, follow the :doc:`rocDecode QuickStart Guide<rocdecode:install/quick-start>`

* CMake 3.12 or later

  .. code:: shell

    sudo apt install cmake

* AMD Clang++ Version `18.0.0` or later - installed with ROCm

* Python3 and Python3 PIP

  .. code:: shell

    sudo apt install python3-dev python3-pip

* PyBind11

  .. code:: shell

    sudo apt install pybind11-dev

* rocDecode

  .. code:: shell

    sudo apt install rocdecode-dev

* `pkg-config <https://en.wikipedia.org/wiki/Pkg-config>`_

  .. code:: shell

    sudo apt install pkg-config

* `FFmpeg <https://ffmpeg.org/about.html>`_ runtime and headers - for tests and samples

  .. code:: shell

    sudo apt install ffmpeg libavcodec-dev libavformat-dev libavutil-dev

* DLPack

  .. code:: shell

    sudo apt install libdlpack-dev   

.. note::

  * All package installs are shown with the ``apt`` package manager. Use the appropriate package manager for your operating system.

  * On ``Ubuntu 22.04`` - Additional package required: ``libstdc++-12-dev``

  .. code:: shell

    sudo apt install libstdc++-12-dev

Prerequisites setup script
--------------------------

For your convenience, we provide the setup script, rocPyDecode-requirements.py,
which installs all required dependencies. Run this script only once.

.. code:: shell

  python3 rocPyDecode-requirements.py

Installation instructions
========================================

To build rocPyDecode from source, run:

If using bare-metal, sudo access is needed.

.. code:: shell

  git clone https://github.com/ROCm/rocPyDecode.git
  cd rocPyDecode
  sudo pip3 install .

If using a docker environment or any system with root access. Do NOT use sudo.

.. code:: shell

  git clone https://github.com/ROCm/rocPyDecode.git
  cd rocPyDecode
  python rocPyDecode-docker-install.py 

Run tests (this requires FFmpeg dev install):

Dependencies:

.. code:: shell

  python3 -m pip install --upgrade pip
  python3 -m pip install -i https://test.pypi.org/simple hip-python

Run test:

.. code:: shell

  cd rocPyDecode
  cmake .
  ctest -VV

To run tests with verbose option, use ``make test ARGS="-VV"``.

Hardware capabilities
=====================

The following table shows the codec support and capabilities of the VCN for each supported GPU
architecture.

.. csv-table::
  :header: "GPU Architecture", "VCN Generation", "Number of VCNs", "H.265/HEVC", "Max width, Max height - H.265", "H.264/AVC", "Max width, Max height - H.264"

  "gfx908 - MI1xx", "VCN 2.5.0", "2", "Yes", "4096, 2176", "Yes", "4096, 2160"
  "gfx90a - MI2xx", "VCN 2.6.0", "2", "Yes", "4096, 2176", "Yes", "4096, 2160"
  "gfx942 - MI3xx A", "VCN 3.0", "3", "Yes", "7680, 4320", "Yes", "4096, 2176"
  "gfx942 - MI3xx X", "VCN 3.0", "4", "Yes", "7680, 4320", "Yes", "4096, 2176"
  "gfx1030, gfx1031, gfx1032 - Navi2x", "VCN 3.x", "2", "Yes", "7680, 4320", "Yes", "4096, 2176"
  "gfx1100, gfx1102 - Navi3x", "VCN 4.0", "2", "Yes", "7680, 4320", "Yes", "4096, 2176"
  "gfx1101 - Navi3x", "VCN 4.0", "1", "Yes", "7680, 4320", "Yes", "4096, 2176"
