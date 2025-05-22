.. meta::
  :description: rocPyDecode documentation
  :keywords: rocPyDecode, rocDecode, ROCm, documentation, video, decode, decoding, acceleration

********************************************************************
rocPyDecode and rocPyJPEGDecode documentation
********************************************************************

rocPyDecode and rocPyJPEGDecode are Python bindings for the `rocDecode <https://rocm.docs.amd.com/projects/rocDecode/en/latest/>`_ and `rocJPEG <https://rocm.docs.amd.com/projects/rocJPEG/en/latest/>`_ APIs, respectively. They connect Python and C/C++ libraries, enabling function calling and data passing between the two languages.

rocPyDecode decodes compressed video streams while keeping the resulting decoded frames in video memory, avoiding unnecessary data copies. rocPyDecode lets you use the FFMpeg demultiplexer (demuxer) to seek and demultiplex (demux) packetized media files, and integrates with machine learning frameworks such as PyTorch to facilitate machine learning on decoded surfaces.

The rocPyDecode python library uses the rocDecode C++ SDK library to decode video streams based on the number of available media engines (VCNs) on the GPU. 

The rocPyJPEGDecode python library uses the rocJPEG C++ SDK library to decode compressed JPEG streams while keeping the resulting YUV images in video memory. With decoded images in video memory, you can run image post-processing using ROCm HIP, thereby avoiding unnecessary data copies via PCIe bus. 

rocPyJPEGDecode is installed as part of rocPyDecode but requires the installation of rocJPEG.

The rocPyDecode public repository is located at `https://github.com/ROCm/rocPyDecode <https://github.com/ROCm/rocPyDecode>`_.

rocPyJPEGDecode is available from within the rocPyDecode GitHub repository at `https://github.com/ROCm/rocPyDecode/tree/develop/pyRocJpegDecode <https://github.com/ROCm/rocPyDecode/tree/develop/pyRocJpegDecode>`_.

.. grid:: 2
  :gutter: 3

  .. grid-item-card:: Install

    * :doc:`rocPyDecode and rocPyJPEGDecode prerequisites <./install/rocPyDecode-prerequisites>`
    * :doc:`rocPyDecode and rocPyJPEGDecode installation overview <./install/rocPyDecode-install>`
    * :doc:`rocPyDecode and rocPyJPEGDecode CMake installation <./install/rocPyDecode-cmake-install>`
    * :doc:`rocPyDecode and rocPyJPEGDecode pip installation <./install/rocPyDecode-pip-install>`

.. grid:: 2
  :gutter: 3

  .. grid-item-card:: Conceptual

    * :doc:`rocPyDecode surface data memory locations <./conceptual/rocPyDecode-mem-types>`
  
  .. grid-item-card:: How to

    * :doc:`Use rocPyDecode <how-to/using-rocPydecode>`  
    * :doc:`Use rocPyJPEGDecode <how-to/using-rocPyJPEGDecode>`

  .. grid-item-card:: Samples

    * :doc:`rocPyDecode and rocPyJPEGDecode samples <./tutorials/rocPyDecode-samples>`  


  .. grid-item-card:: Reference

    * :doc:`rocDecode supported codecs and hardware <reference/rocPyDecode-codecs-and-hardware>`
    * :doc:`rocPyDecode API reference<reference/rocPyDecode>`
      
      * :doc:`rocPyDecode structures <./reference/structures>`
      * :doc:`rocPyDecode decoder class <./reference/decoderClass>`
      * :doc:`rocPyDecode demuxer class <./reference/demuxerClass>`
      * :doc:`rocPyJPEGDecode decoder class <./reference/rocPyJPEGDecode-api>


To contribute to the documentation, refer to
`Contributing to ROCm <https://rocm.docs.amd.com/en/latest/contribute/contributing.html>`_.

You can find licensing information on the
`Licensing <https://rocm.docs.amd.com/en/latest/about/license.html>`_ page.
