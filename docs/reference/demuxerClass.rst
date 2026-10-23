.. meta::
  :description: rocPyDecode Demuxer class documentation
  :keywords: rocPyDecode, rocDecode, ROCm, API, documentation, video, decode, decoding, acceleration

.. _demuxer_section:

*************************
rocPyDecode demuxer class
*************************

Instantiation
=============

To instantiate a demuxer instance, pass the video file name and path as the input parameter.
The demuxer accepts H.264, HEVC, VP9, and AV1 video. Other codecs raise
``ValueError`` before packets are submitted to rocDecode. Decoding also requires
support for the selected codec on the GPU and installed rocDecode SDK.

Use a separate demuxer for each decoding task. An instance is not thread-safe;
reading, seeking, and closing that instance must not run concurrently.

Example:
--------

.. code-block:: python

                # demuxer instance
                demuxer = dmx.demuxer(input_file_path)

Member functions
================

The following are the member functions of the demuxer class.

DemuxFrame()
------------

Demux the input video file and returns a :ref:`packet` for use in decoding the demuxed frames.

Example:
^^^^^^^^

.. code-block:: python

    while True:
        packet = demuxer.DemuxFrame()
        n_frame_returned = viddec.DecodeFrame(packet)
        for _ in range(n_frame_returned):
            viddec.GetFrameYuv(packet)
            surface_info = viddec.GetOutputSurfaceInfo()
            viddec.SaveFrameToFile(output_file_path, packet.frame_adrs, surface_info)
            viddec.ReleaseFrame(packet)
        # Submit the end-of-stream packet to drain the decoder before stopping.
        if packet.bitstream_size <= 0:
            break

GetCodecId()
------------

Returns the demux codec ID to be used in later decoder functions.
These IDs are stable rocPyDecode compatibility values, independent of PyAV's
internal codec numbering. Pass them to ``GetRocDecCodecID`` for GPU decoding.

Example:
^^^^^^^^

.. code-block:: python

                # demuxer instance
                demuxer = dmx.demuxer(input_file_path)

                # get the used coded id
                codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())
                # decoder instance
                viddec = dec.decoder(codec_id, device_id=device_id, mem_type=mem_type,
                    b_force_zero_latency=b_force_zero_latency, crop_rect=crop_rect)

SeekFrame(frame_number, seek_mode, seek_criteria)
-------------------------------------------------

Demux the input video file starting from the specified seek parameters. Returns a :ref:`packet` for use in decoding the demuxed frames.

Input:

- **frame_number**:   Non-negative integer frame number, or finite non-negative seconds when ``seek_criteria=1`` (for example, ``2.5``)
- **seek_mode**:      0 - First packet in decode order at or after the target presentation timestamp, 1 - Previous keyframe
- **seek_criteria**:  0 - Seek by frame number, 1 - Seek by timestamp in seconds, including fractional seconds

Mode 0 preserves packet decode order; with B-frames, it does not guarantee that
the first returned packet has the closest presentation timestamp. It also omits
preceding reference frames. For decoding inter-predicted video, use mode 1 and
decode forward, selecting output frames by timestamp. Reset or recreate the
decoder before seeking to a different position.

Example:
^^^^^^^^

.. code-block:: python

    # Start with a fresh decoder when seeking to a different position.
    viddec = dec.decoder(dec.GetRocDecCodecID(demuxer.GetCodecId()))
    packet = demuxer.SeekFrame(100, 1, 0)  # Previous keyframe, by frame number
    while True:
        n_frame_returned = viddec.DecodeFrame(packet)
        for _ in range(n_frame_returned):
            viddec.GetFrameYuv(packet)
            # Process this frame before releasing its decoder surface.
            viddec.ReleaseFrame(packet)
        if packet.bitstream_size <= 0:
            break
        packet = demuxer.DemuxFrame()
