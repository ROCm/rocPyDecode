.. meta::
  :description: rocPyDecode Docker egg file
  :keywords: install, rocPyDecode, rocPyJpegDecode, AMD, ROCm, docker, egg

********************************************************************
Create a rocPyDecode egg file to use with Docker
********************************************************************

An egg distribution file for using rocPyDecode on Docker can be created using a Python script located in the root of the `rocPyDecode GitHub repository <https://github.com/ROCm/rocPyDecode/blob/develop/>`_.

Use the develop branch if you want to preview new features or contribute to the rocPyDecode and rocPyJpegDecode code base. If you don't intend to preview new features or contribute to the codebase, clone the branch that corresponds to your version of ROCm.

.. note::

  To include rocPyJpegDecode in the egg file, install `rocJPEG <https://rocm.docs.amd.com/projects/rocJPEG/en/latest/index.html>`_ before running the egg generation script. 

Before running the egg generation script, run `rocPyDecode-requirements.py <https://github.com/ROCm/rocPyDecode/blob/develop/rocPyDecode-requirements.py>`_ from the ``rocPyDecode`` root directory:

.. code:: shell

  cd rocPyDecode
  python3 rocPyDecode-requirements.py

If you're installing on Ubuntu 22.04, install libstdc++-12-dev:

.. code:: shell

  apt install libstdc++-12-dev

Run ``pip3 install``:

.. code:: shell

  pip3 install .

Run the `rocPyDecode-docker-install.py <https://github.com/ROCm/rocPyDecode/blob/develop/rocPyDecode-docker-install.py>`_ script:

.. code:: shell

  python3 rocPyDecode-docker-install.py 

The resulting egg file will saved to the ``dist`` directory.
