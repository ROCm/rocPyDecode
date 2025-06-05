.. meta::
  :description: rocPyDecode Installation
  :keywords: install, rocPyDecode, rocPyJpegDecode, AMD, ROCm, wheel, docker, bare metal

********************************************************************
Installing rocPyDecode with pip
********************************************************************

Installing rocPyDecode with pip is used for generating wheel and egg files. If you won't be generating wheel and egg files, the :doc:`CMake installation <./rocPyDecode-cmake-install>` is recommended.

rocPyJpegDecode will only be installed if `rocJPEG <https://rocm.docs.amd.com/projects/rocJPEG/en/latest/index.html>`_ is already installed. 

.. note::

  The generated egg and wheel files include rocPyJpegDecode.

The rocPyDecode source code and its installation scripts are available from the `rocPyDecode GitHub Repository <https://github.com/ROCm/rocPyDecode>`_. 

The develop branch is the default branch and is intended for users who want to preview new features or contribute to the rocPyDecode and rocPyJpegDecode code base. If you don't intend to preview new features or contribute to the codebase, clone the branch that corresponds to your version of ROCm.

.. note:: 

  sudo access is required to install rocPyDecode on bare metal.

Before installing rocPyDecode, run `rocPyDecode-requirements.py <https://github.com/ROCm/rocPyDecode/blob/develop/rocPyDecode-requirements.py>`_ from the ``rocPyDecode`` root directory:

.. code:: shell

  python3 rocPyDecode-requirements.py

If you're installing on Ubuntu 22.04, install libstdc++-12-dev:

.. code:: shell

  apt install libstdc++-12-dev

To install rocPyDecode on bare metal, run ``pip3 install`` from the ``rocPyDecode`` root directory:

.. code:: shell

  pip3 install .

To build and install rocPyDecode in a Docker container, run the `rocPyDecode-docker-install.py <https://github.com/ROCm/rocPyDecode/blob/develop/rocPyDecode-docker-install.py>`_ script from the ``rocPyDecode`` root directory:

.. code:: shell

  python3 rocPyDecode-docker-install.py 

The resulting egg file will saved to ``rocPyDecode/dist``.

To create a wheel distribution file, run the `build_rocpydecode_wheel.py <https://github.com/ROCm/rocPyDecode/blob/develop/build_rocpydecode_wheel.py>`_ script from the ``rocPyDecode`` root directory:

.. code:: shell
  
  python3 build_rocpydecode_wheel.py
  
You can also run:

.. code:: shell

  python3 setup.py bdist_wheel

The resulting wheel file will be saved to ``rocPyDecode/dist``.
