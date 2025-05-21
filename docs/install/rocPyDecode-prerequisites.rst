.. meta::
  :description: rocPyDecode and rocPyJPEGDecode Installation Prerequisites
  :keywords: install, rocPyDecode, rocPyJPEGDecode, AMD, ROCm, prerequisites, dependencies, requirements

********************************************************************
rocPyDecode and rocPyJPEGDecode prerequisites
********************************************************************

rocPyDecode and rocPyJPEGDecode require Ubuntu 22.04 or 24.04 with ROCm running on `accelerators based on the CDNA architecture <https://rocm.docs.amd.com/projects/install-on-linux/en/latest/reference/system-requirements.html>`_.

ROCm needs to be installed using the `AMDGPU installer <https://rocm.docs.amd.com/projects/install-on-linux/en/latest/how-to/amdgpu-install.html>`_ with the ``rocm`` usecase:

.. code:: shell

  sudo amdgpu-install --usecase=rocm
    
rocPyDecode and rocPyJPEGDecode have the following prerequisites:

* `CMake 3.12 or later <https://cmake.org/>`_
* `Python3 3.9 or later <https://www.python.org/>`_
* Python3 pip
* `PyBind11 <https://github.com/pybind/pybind11>`_
* `rocDecode <https://rocm.docs.amd.com/projects/rocDecode/en/latest/index.html>`_
* `FFmpeg runtime and headers <https://ffmpeg.org>`_
* `DLPack <https://dmlc.github.io/dlpack/latest/>`_
* `NumPy, for running tests and samples <https://numpy.org/>`_

rocPyJPEGDecode additionally requires `rocJPEG <https://rocm.docs.amd.com/projects/rocJPEG/en/latest/index.html>`_.

All prerequisites with the exception of NumPy and rocJPEG are installed with the `rocPyDecode-requirements.py <https://github.com/ROCm/rocPyDecode/blob/develop/rocPyDecode-requirements.py>`_ script. 
