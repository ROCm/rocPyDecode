.. meta::
  :description: rocPyDecode wheel
  :keywords: install, rocPyDecode, rocPyJpegDecode, AMD, ROCm, wheel

********************************************************************
Create a rocPyDecode wheel file
********************************************************************

A wheel distribution file for rocPyDecode can be created using a Python script located in the root of the `rocPyDecode GitHub repository <https://github.com/ROCm/rocPyDecode/blob/develop/>`_.

Use the develop branch if you want to preview new features or contribute to the rocPyDecode and rocPyJpegDecode code base. If you don't intend to preview new features or contribute to the codebase, clone the branch that corresponds to your version of ROCm.

.. note::

  To include rocPyJpegDecode in the wheel file, install `rocJPEG <https://rocm.docs.amd.com/projects/rocJPEG/en/latest/index.html>`_ before running the wheel generation script. 

Before running the wheel generation script, run `rocPyDecode-requirements.py <https://github.com/ROCm/rocPyDecode/blob/develop/rocPyDecode-requirements.py>`_ from the ``rocPyDecode`` root directory:

.. code:: shell

  cd rocPyDecode
  python3 rocPyDecode-requirements.py

If you're installing on Ubuntu 22.04, install libstdc++-12-dev:

.. code:: shell

  apt install libstdc++-12-dev

Run ``pip3 install``:

.. code:: shell

  pip3 install .

Run the `build_rocpydecode_wheel.py <https://github.com/ROCm/rocPyDecode/blob/develop/build_rocpydecode_wheel.py>`_ script to generate the wheel distribution file:

.. code:: shell
  
  python3 build_rocpydecode_wheel.py
  
You can also run:

.. code:: shell

  python3 setup.py bdist_wheel

The resulting wheel file will be saved to ``dist`` directory.
