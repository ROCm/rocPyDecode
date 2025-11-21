.. meta::
  :description: rocPyDecode Installation Prerequisites
  :keywords: install, rocPyDecode, rocPyJpegDecode, AMD, ROCm, prerequisites, dependencies, requirements

********************************************************************
rocPyDecode prerequisites
********************************************************************

rocPyDecode has been tested on Ubuntu 22.04 and 24.04.

For supported codecs and hardware capabilities, see :doc:`../reference/rocPyDecode-codecs-and-hardware`. See `Supported operating systems <https://rocm.docs.amd.com/projects/install-on-linux/en/latest/reference/system-requirements.html#supported-operating-systems>`_ for the complete list of ROCm supported Linux environments.
    
rocPyDecode has the following prerequisites:

* `CMake 3.12 or later <https://cmake.org/>`_
* `Python3 3.9 or later <https://www.python.org/>`_
* `Python3 pip <https://pypi.org/project/pip/>`_
* `PyBind11 <https://github.com/pybind/pybind11>`_
* `rocDecode <https://rocm.docs.amd.com/projects/rocDecode/en/latest/index.html>`_
* `FFmpeg runtime and headers <https://ffmpeg.org>`_
* `DLPack <https://dmlc.github.io/dlpack/latest/>`_
* `NumPy, for running tests and samples <https://numpy.org/>`_

rocPyJpegDecode additionally requires `rocJPEG <https://rocm.docs.amd.com/projects/rocJPEG/en/latest/index.html>`_.

All prerequisites except for rocJPEG are installed with the `rocPyDecode-requirements.py <https://github.com/ROCm/rocPyDecode/blob/develop/rocPyDecode-requirements.py>`_ script. 


