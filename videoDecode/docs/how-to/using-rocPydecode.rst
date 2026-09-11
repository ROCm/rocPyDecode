.. meta::
  :description: Using rocPyDecode
  :keywords: parse video, parse, decode, video decoder, video decoding, rocDecode, AMD, ROCm

********************************************************************
Using rocPyDecode
********************************************************************

Two rocPyDecode classes, |demuxer|_ and |decoder|_, contain the APIs needed to demultiplex and decode a video file. These can be found in ``pyRocVideoDecode/demuxer.py`` and ``pyRocVideoDecode/decoder.py``, respectively.

.. |demuxer| replace:: ``demuxer``
.. _demuxer: https://rocm.docs.amd.com/projects/rocPyDecode/en/latest/reference/demuxerClass.html

.. |decoder| replace:: ``decoder``
.. _decoder: https://rocm.docs.amd.com/projects/rocPyDecode/en/latest/reference/decoderClass.html

Before decoding a video file with rocPyDecode, a ``demuxer`` and a ``decoder`` need to be instantiated.

The demuxer and decoder instances are used together to decode the video file.

Import both the ``pyRocVideoDecode.decoder`` and ``pyRocVideoDecode.demuxer`` modules:

.. code:: python

    import pyRocVideoDecode.decoder as dec
    import pyRocVideoDecode.demuxer as dmx

Instantiate the demuxer by passing the path to the video file:

.. code:: python

	demuxer = dmx.demuxer(input_file_path)

Instantiate the decoder using the ``decoder()`` function. The input file's codec ID needs to be passed to the decoder to instantiate it.

The codec ID is obtained by passing the output of ``demuxer.GetCodecId()`` to the ``GetRocDecCodecID()`` function in ``pyRocVideoDecode.decoder``.

.. code:: python

    codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())

The following is the complete list of parameters for ``decoder()``. Only ``codec`` is required. All other parameters are optional.

.. list-table::
    :widths: 15 65 20
    :header-rows: 1

    *   - Parameter
        - Description
        - Default value

    *   - ``codec``
        - The video file's codec ID. Use ``dec.GetRocDecCodecID(demuxer.GetCodecId())`` to get the codec ID.
        - No default. A value needs to be provided.

    *   - ``device_id``
        - The GPU device ID.
        - Default 0

    *   - ``mem_type``
        - The memory type where the surface data, such as the decoded frames, resides. |br| |br| Set it to 0 to use internal decoder device surfaces. |br| |br| Set it to 1 if the surface data resides on the GPU. |br| |br| Set it to 2 if the surface data resides on the CPU. |br| |br| See :doc:`rocPyDecode memory types <../conceptual/rocPyDecode-mem-types>` for details.
        - Default 0, internal decoder device surfaces

    *   - ``b_force_zero_latency``
        - Set to ``True`` to force zero latency.
        - Default ``False``

    *   - ``crop_rect``
        - The dimensions of the crop rectangle, ``(left, top, right, bottom)``. See :ref:`Rect structure` for more details.
        - Default ``None``, no cropping

    *   - ``max_width``
        - Max width
        - Default 0

    *   - ``max_height``
        - Max height
        - Default 0

    *   - ``clk_rate``
        - Clock rate
        - Default 1000


.. |br| raw:: html

      </br>

After instantiating the demuxer and the decoder, verify that the codec is supported using ``IsCodecSupported()``.

From the ``videodecode.py`` example:

.. code:: python

    # Instantiate a demuxer
    demuxer = dmx.demuxer(input_file_path)

    # Get the coded id
    codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())

    # Instantiate a decoder
    viddec = dec.decoder(
        codec_id,
        device_id,
        mem_type,
        b_force_zero_latency,
        crop_rect,
        0,
        0,
        1000)

    # Get the GPU device information
    cfg = viddec.GetGpuInfo()

    # Check that the codec is supported
    if (viddec.IsCodecSupported(device_id, codec_id, demuxer.GetBitDepth()) == False):
        print("ERROR: Codec is not supported on this GPU " + cfg.device_name)
        exit()

Before entering the decoder loop, set the flush mode with ``SetReconfigParams()``. Flushing is needed to finalize the decoding process when the video being decoded has frames with different resolutions.  The flush mode depends on whether decoded frames are written to file.

From the ``videodecode.py`` example:

.. code:: python

    flush_mode = 0
    if (output_file_path is not None):
        flush_mode = 1

    viddec.SetReconfigParams(flush_mode, output_file_path if (output_file_path is not None) else str(""))


Once it's been determined that the codec is supported and the flush mode has been set, the input can be demuxed and then decoded. ``demuxer.DemuxFrame()`` demuxes frames sequentially starting at the beginning of the file. To start from a different frame, ``demuxer.SeekFrame()`` is used. Both return a packet that can be passed to the decoder. The packet contains information related to the demuxed frames, such as time stamp, YUV frame address, and  resized frame address. See :ref:`packet` for more information.

The packet is passed to ``decoder.DecodeFrame()`` which updates the packet with the decoded frame information and returns the number of frames that have been decoded.

From the ``videodecode.py`` example:

.. code:: python

    if(not_seeking):
        packet = demuxer.DemuxFrame()
    else:
        packet = demuxer.SeekFrame(seek_frame, seek_mode, seek_criteria)
        not_seeking = True

    n_frame_returned = viddec.DecodeFrame(packet)

Frames can be further processed using the ``GetFrameRgb()`` and ``GetFrameYuv()`` functions. Both functions retrieve a frame, update the packet with its frame address and exported buffers, and return its presentation timestamp (or -1 when no frame is available). Use one retrieval function for each frame, then release that frame after processing. The ``GetOutputSurfaceInfo()`` function returns information about the decoded frames that need to be passed to the processing functions.

From the ``videodecode.py`` example:

.. code:: python

    surface_info = viddec.GetOutputSurfaceInfo()
    if(viddec.ResizeFrame(packet, resize_dim, surface_info) != 0):
        frame_is_resized = True
    else:
        frame_is_resized = False

After decoding a frame, release it with ``ReleaseFrame()``.

From the ``videodecode.py`` example:

.. code:: python

    for i in range(n_frame_returned):
        viddec.GetFrameYuv(packet)

        [...]

        if (resize_dim is not None):
            surface_info = viddec.GetOutputSurfaceInfo()
            if(viddec.ResizeFrame(packet, resize_dim, surface_info) != 0):
                frame_is_resized = True
            else:
                frame_is_resized = False

        if (output_file_path is not None):
            if (frame_is_resized):
                resized_surface_info = viddec.GetResizedOutputSurfaceInfo()
                viddec.SaveFrameToFile(output_file_path, packet.frame_adrs_resized, resized_surface_info)
            else:
                viddec.SaveFrameToFile(output_file_path, packet.frame_adrs)

        # release frame
        viddec.ReleaseFrame(packet)

``ReleaseFrame()`` releases the retrieved decoder surface; it does not flush pending decoded frames.

Decoder resources are destroyed when the Python class object is released.

The rocPyDecode samples are available in ``samples/rocdecode``.
