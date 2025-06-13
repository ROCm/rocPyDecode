.. meta::
  :description: rocPyDecode Installation
  :keywords: install, rocPyDecode, rocPyJpegDecode, AMD, ROCm, wheel, docker, bare metal

********************************************************************
Installing rocPyDecode 
********************************************************************

The rocPyDecode source code and its installation scripts are available from the `rocPyDecode GitHub Repository <https://github.com/ROCm/rocPyDecode>`_. 

rocPyJpegDecode is installed with rocPyDecode if `rocJPEG <https://rocm.docs.amd.com/projects/rocJPEG/en/latest/index.html>`_ is already installed on the system. If rocJPEG is not already installed, the rocPyJpegDecode libraries won't be installed.

The full list of prerequisites for both rocPyDecode and rocPyJpegDecode can be found in :doc:`rocPyDecode prerequisites <./rocPyDecode-prerequisites>`.

The develop branch is the default rocPyDecode branch. The develop branch is intended for users who want to preview new features or contribute to the rocPyDecode code base. If you don't intend to preview new features or contribute to the codebase, clone the rocPyDecode branch that corresponds to your version of ROCm.

rocPyDecode can be installed with :doc:`CMake <./rocPyDecode-cmake-install>` or with the :doc:`pip package manager <./rocPyDecode-pip-install>`. 

The CMake installation method can be used to create deb files and zipped tar files for distribution.

The pip installation method can be used to generate Python egg and wheel files for distribution. 

.. note::

  The deb files, tar files, and wheel files will include rocPyJpegDecode if rocJPEG was installed on the system before installing rocPyDecode.
