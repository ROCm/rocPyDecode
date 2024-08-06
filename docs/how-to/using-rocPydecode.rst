.. meta::
  :description: Using rocPyDecode
  :keywords: parse video, parse, decode, video decoder, video decoding, rocDecode, AMD, ROCm

********************************************************************
Using rocPyDecode
********************************************************************

RocPyDecode is a python library module which allows Python decoding functionality using rocDecode C++ SDK library backend.

1. API overview
====================================================

All rocPyDecode APIs are exposed in the header files ``decoder.py`` and ``demuxer.py``. You can find
these files in the `pyRocVideoDecode` folder in the rocPyDecode repository.

For detailed explanation of rocDecode API, please refer to `rocDecode documentation <https://rocm.docs.amd.com/projects/rocDecode/en/latest/>`.
The samples use the ``pyRocVideoDecode`` python module to interface with the low level ``rocVideoDecode`` class in the C++ rocDecode SDK library.

The ``pyRocVideoDecode`` module exposes the following APIs thorugh two python classes ``PyRocVideoDecoder`` and ``PyVideoDemuxer``.

class PyRocVideoDecoder (Video decoder class)
    * ``GetDeviceinfo(self)`` -> API to get device information of the current device
    * ``SetReconfigParams(self)`` -> API to reconfigure decoder when codec/resolution changes
    * ``DecodeFrame(self)`` -> API to trigger decoding of a frame thorugh the low level decoder
    * ``GetFrame(self)`` -> API to receive a decoded output frame
    * ``SaveFrameToFile(self)`` -> API to save decoded output frame to a file
    * ``ReleaseFrame(self)`` -> API to release decoded frame after use
    * ``GetOutputSurfaceInfo(self)`` -> API to get the output surface information of the decoded frame
    * ``GetNumOfFlushedFrames(self)`` -> API to get number of flushed frames during reconfigure

class PyVideoDemuxer (python demuxer class)
    * ``GetCodecId(self)`` -> API to get the Codec_id of the current stream from the demuxer
    * ``DemuxFrame(self)`` -> API to demultiplex a frame using FFMPEG API

2. Create a decoder
====================================================

``PyRocVideoDecoder()`` in turn creates an instance of ``RocVideoDecoder()`` class and returns a handle upon successful creation. 

3. Decode the frame
====================================================

After de-multiplexing and parsing, you can decode bitstream data containing a frame/field using
hardware.

Use the ``DecodeFrame()`` API to submit a new frame for hardware decoding. Underneath the
driver, the Video Acceleration API (VA-API) is used to submit compressed picture data to the driver.
The parser extracts all the necessary information from the bitstream and fills the ``RocdecPicParams``
structure that's appropriate for the codec. The high-level ``RocVideoDecoder`` class connects the parser
and decoder used for all sample applications.

The ``DecodeFrame()`` call will call the C++ ``rocDecDecodeFrame()`` call takes the decoder handle and the pointer to the ``RocdecPicParams``
structure and initiates the video decoding using VA-API.

4. Prepare the decoded frame for further processing
====================================================

The decoded frames can be used for further postprocessing using ``GetFrame()``. The
successful completion of ``GetFrame()`` indicates that the decoding process is complete and
the device memory pointer is inter-opped into the ROCm HIP address space in order to further process
the decoded frame in device memory. The caller gets the necessary information on the output surface,
such as YUV format, dimensions, and pitch from this call. In the high-level ``RocVideoDecoder`` class, we
provide four different surface type modes for the mapped surface, as specified in
``OutputSurfaceMemoryType``.

.. code:: cpp

    typedef enum OutputSurfaceMemoryType_enum {
        OUT_SURFACE_MEM_DEV_INTERNAL = 0,      /**<  Internal interopped decoded surface memory **/
        OUT_SURFACE_MEM_DEV_COPIED = 1,        /**<  decoded output will be copied to a separate device memory **/
        OUT_SURFACE_MEM_HOST_COPIED = 2        /**<  decoded output will be copied to a separate host memory **/
        OUT_SURFACE_MEM_NOT_MAPPED = 3         /**<  decoded output is not available (interop won't be used): useful for decode only performance app*/
    } OutputSurfaceMemoryType;


If the mapped surface type is ``OUT_SURFACE_MEM_DEV_INTERNAL``, the direct pointer to the decoded
surface is provided. You must call ``ReleaseFrame()`` (``RocVideoDecoder`` class). If the requested surface
type is ``OUT_SURFACE_MEM_DEV_COPIED`` or ``OUT_SURFACE_MEM_HOST_COPIED``, the internal
decoded frame is copied to another buffer, either in device memory or host memory. After that, it's
immediately unmapped for re-use by the ``RocVideoDecoder`` class.

Refer to the ``RocVideoDecoder`` class and
`samples <https://github.com/ROCm/rocDecode/tree/develop/samples>`_ for details on how to use
these APIs.

``PyRocVideoDecoder()`` can pass the appropriate mem_type to ``rocVideoDecoder`` class on creation.

5.  Reconfigure the decoder
====================================================

You can call ``rocDecReconfigureDecoder()`` to reuse a single decoder for multiple clips or when the
video resolution changes during the decode. The API currently supports resolution changes, resize
parameter changes, and target area parameter changes for the same codec without destroying an
ongoing decoder instance. This can improve performance and reduce overall latency.

The API inputs are:

* ``decoder_handle``: A ``RocDecoder`` handler, ``rocDecDecoderHandle``.
* ``reconfig_params``: You must specify the parameters for the changes in
  ``RocdecReconfigureDecoderInfo``. The width and height used for reconfiguration cannot exceed the
  values set for ``max_width`` and ``max_height``, defined in ``RocDecoderCreateInfo``. If you need to
  change these values, you have to destroy and recreate the session.

.. note::
``SetReconfigParams()`` api is used to set reconfig parameters from Python to the low_lever decoder.
  .

6.  Destroy the decoder
====================================================

The decoder resources will be destroyed when the Python class object is released.
