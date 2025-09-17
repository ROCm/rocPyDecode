.. meta::
  :description: rocPyDecode documentation
  :keywords: rocPyDecode, rocDecode, ROCm, documentation, video, decode, decoding, acceleration

********************************************************************
rocPyDecode documentation
********************************************************************

rocPyDecode provides Python bindings for the `rocDecode <https://rocm.docs.amd.com/projects/rocDecode/en/latest/>`_ C++ APIs, enabling function calling and data passing between C++ and Python.

rocPyDecode uses the rocDecode C++ SDK library to decode video streams based on the number of available media engines (VCNs) on the GPU. 

rocPyJpegDecode provides Python bindings for the rocJPEG APIs and is installed as part of rocPyDecode when `rocJPEG <https://rocm.docs.amd.com/projects/rocJPEG/en/latest/>`_ is already installed on the system. 

The rocPyDecode public repository is located at `https://github.com/ROCm/rocPyDecode <https://github.com/ROCm/rocPyDecode>`_.


.. grid:: 2
  :gutter: 3

  .. grid-item-card:: Install

    * :doc:`rocPyDecode prerequisites <./install/rocPyDecode-prerequisites>`
    * :doc:`rocPyDecode installation overview <./install/rocPyDecode-install>`
    * :doc:`rocPyDecode CMake installation <./install/rocPyDecode-cmake-install>`

.. grid:: 2
  :gutter: 3

  .. grid-item-card:: Conceptual

    * :doc:`rocPyDecode surface data memory locations <./conceptual/rocPyDecode-mem-types>`
  
  .. grid-item-card:: How to

    * :doc:`Use rocPyDecode <how-to/using-rocPydecode>` 
    * :doc:`Create a rocPyDecode wheel file <how-to/rocPyDecode-wheel>`

  .. grid-item-card:: Samples

    * :doc:`rocPyDecode samples <./tutorials/rocPyDecode-samples>`  


  .. grid-item-card:: Reference

    * :doc:`rocPyDecode supported codecs and hardware <reference/rocPyDecode-codecs-and-hardware>`
    * :doc:`rocPyDecode API reference<reference/rocPyDecode>`
      
      * :doc:`rocPyDecode structures <./reference/structures>`
      * :doc:`rocPyDecode decoder class <./reference/decoderClass>`
      * :doc:`rocPyDecode demuxer class <./reference/demuxerClass>`
      * :doc:`rocPyJpegDecode decoder class <./reference/rocPyJPEGDecode-api>`


To contribute to the documentation, refer to
`Contributing to ROCm <https://rocm.docs.amd.com/en/latest/contribute/contributing.html>`_.

You can find licensing information on the
`Licensing <https://rocm.docs.amd.com/en/latest/about/license.html>`_ page.
