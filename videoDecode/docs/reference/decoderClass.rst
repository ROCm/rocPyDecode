.. meta::
  :description: rocPyDecode Decoder class documentation
  :keywords: rocPyDecode, rocDecode, ROCm, API, documentation, video, decode, decoding, acceleration

.. _decoder_section:

*************************
rocPyDecode decoder class
*************************

Instantiation
=============

To instantiate a decoder instance, pass the following parameters:

- **codec**: The required first argument, obtained with ``GetRocDecCodecID``.
- **device_id**: The GPU device ID, default is 0.
- **mem_type**: Output surface memory: 0 internal device, 1 copied device, 2 copied host. The Python decoder defaults to 0; command-line samples may select a different default.
- **b_force_zero_latency**: 	Force zero latency flag, default is 'False'
- **crop_rect**:    See :doc:`Structures <structures>`, optional, default: 'None', no cropping. The rectangle must lie within the decoded frame and align to its chroma subsampling.
- **max_width**:    Max width, default is 0
- **max_height**:   Max height, default is 0
- **clk_rate**:     Clock rate, default is 1000

Example:
--------

.. code-block:: python

	# create decoder instance
	viddec = dec.decoder(codec_id, device_id=device_id, mem_type=mem_type,
            b_force_zero_latency=b_force_zero_latency, crop_rect=crop_rect)

Member functions
================

The following are the member functions of the Decode class.

DecodeFrame(packet)
-------------------

Decodes video frames described by the input :ref:`packet` obtained from demux functions. Returns the count of decoded frames.


GetFrameYuv(packet, separate_planes=False)
-----------------------------------------

Retrieves the next decoded YUV frame, updates ``packet.frame_adrs`` and
``packet.ext_buf``, and returns its presentation timestamp, or -1 if no frame
is available. Set ``separate_planes=True`` to export individual YUV planes.
With ``separate_planes=False``, the tensor contains concatenated YUV samples
reshaped to the luma width; a tensor row is not necessarily one chroma row.
Layouts with different plane pitches or vertical gaps are copied into packed
storage for this export. The packet's raw ``frame_adrs`` and surface information
still describe the original retrieved frame. Use ``separate_planes=True`` for
individual plane geometry without this packing step.

Process each retrieved frame before releasing it with ``ReleaseFrame(packet)``.

Example:
^^^^^^^^

.. code-block:: python

    while True:
        packet = demuxer.DemuxFrame()
        decoded_now = viddec.DecodeFrame(packet)
        for _ in range(decoded_now):
            viddec.GetFrameYuv(packet)
            # Process this frame here, before releasing its decoder surface.
            viddec.ReleaseFrame(packet)
        if packet.bitstream_size <= 0:
            break

GetFrameRgb(packet, rgb_format)
------------------------------

Retrieves the next decoded frame, converts it to interleaved RGB/RGBA or
BGR/BGRA in device memory, and updates ``packet.frame_adrs_rgb`` and
``packet.ext_buf[0]``. Returns its presentation timestamp, or -1 if no frame
is available. Use this instead of ``GetFrameYuv`` when retrieving a frame for
RGB output; both calls retrieve a frame from the decoder queue.

- **packet**: The packet to receive the converted frame and buffer metadata.
- **rgb_format**: 1 BGR, 2 BGR48, 3 RGB, 4 RGB48, 5 BGRA, 6 BGRA64,
  7 RGBA, or 8 RGBA64. Format 0 (native YUV) is invalid for this call.

Odd-numbered formats use uint8 channels; even-numbered formats use uint16
channels. Buffers have shape ``(height, width, channels)`` with three channels
for formats 1-4 and four for formats 5-8. Buffer strides are in elements;
interleaved pixel/channel strides are ``(channels, 1)``. DLPack exports retain
the converted allocation after the decoder is destroyed.

GetFrameSize()
--------------

Returns the size of the current decoded frame in bytes.

GetGpuInfo()
------------

Returns the :ref:`configinfo` for the current GPU device

Example:
^^^^^^^^

.. code-block:: python

		# Get GPU device information
		cfg = viddec.GetGpuInfo()

		# print GPU info out
		print("GPU device " + str(device_id) + " - " + cfg.device_name + "[" + cfg.gcn_arch_name + "] on PCI bus " + str(cfg.pci_bus_id) + ":" + str(cfg.pci_domain_id) + "." + str(cfg.pci_device_id))


GetHeight()
-----------

Returns the height of the current decoded frame.

GetOutputSurfaceInfo()
----------------------

Returns the current decoded frame's surface information structure pointer.

GetResizedOutputSurfaceInfo()
-----------------------------

Returns the current decoded and resized frame's surface information structure pointer.

GetStride()
-----------

Returns the stride of the current decoded frame.

GetWidth()
----------

Returns the width of the current decoded frame.

ResizeFrame(packet, resize_dim, surface_info)
---------------------------------------------

Resizes the YUV frame using center-sampled nearest-neighbor interpolation. The
output preserves the input plane layout and sample type and resides in device
memory, including when the input resides in host memory. Resizing completes
before this call returns.

Dimensions must be positive and align to the chroma subsampling (even width and
height for YUV 4:2:0). Invalid dimensions raise ``ValueError``. A request matching
the input dimensions returns zero without resizing. Otherwise, use
``packet.frame_adrs_resized`` and the returned surface information while the
decoder remains alive and before the next resize overwrites that buffer.

- **packet**: The demuxed packet contains the demuxed frames information, and the desired rgb format
- **resize_dim**:  The new dimension, width and height
- **surface_info**: The current surface info obtained by GetOutputSurfaceInfo API

Example:
^^^^^^^^

.. code-block:: python

		# resize frame to new dimension
		resize_dim = [1024, 720]

		surface_info = viddec.GetOutputSurfaceInfo()

		frame_is_resized = False

		if(viddec.ResizeFrame(packet, resize_dim, surface_info) != 0):
			frame_is_resized = True

SaveFrameToFile(output_file_path, frame_adrs, surface_info=0, output_format=native)
-------------------------------------------------------------------------------

Appends the supplied frame to a raw output file. The default output format is
native YUV. For an RGB frame, pass the matching enum from
``dec.GetOutputFormat(rgb_format)``. This writes raw pixel bytes, not a PNG or
JPEG image.

- **output_file_path**: The output file path.
- **frame_adrs**: The frame address stored in the packet.
- **surface_info**: An output surface information pointer; zero uses the current
  decoded surface information. Pass resized surface information for resized YUV.
- **output_format**: The output format enum, default ``native``.

For example, after retrieving RGB with ``GetFrameRgb(packet, 3)``:

.. code-block:: python

    viddec.SaveFrameToFile(output_file_path, packet.frame_adrs_rgb,
                          output_format=dec.GetOutputFormat(3))

ReleaseFrame(packet)
--------------------

Releases the decoder surface associated with the retrieved frame. Release each
frame after processing it. Borrowed internal YUV storage must not be used after
release. This call does not flush pending decoded frames from the decoder.

GetNumOfFlushedFrames()
-----------------------

Returns the count of the flushed frames.

Example:
^^^^^^^^

.. code-block:: python

		# beyond the decoding loop
		n_frame += viddec.GetNumOfFlushedFrames()

SetReconfigParams(flush_mode, out_file_name)
--------------------------------------------

Specify the flush mode and the output file name to use in multi resolution video support.

- **flush_mode**:

	- 0: Just flush to get the frame count
	- 1: The remaining frames will be dumped to file in this mode

- **out_file_name**: The full path disk file name to save the YUV frames

Example:
^^^^^^^^

.. code-block:: python

		# set reconfiguration params based on user arguments
		flush_mode = 0

		if (output_file_path is not None):
			flush_mode = 1

		viddec.SetReconfigParams(flush_mode, output_file_path if (output_file_path is not None) else str(""))
