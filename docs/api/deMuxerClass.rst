.. _demuxer_section:

The Demuxer Class
=================

To instantiate a demuxer instance, pass the video file name as a parameter.
	
**Example:**
	.. code-block:: python 

		# demuxer instance
		demuxer = dmx.demuxer(input_file_path)

The demuxer class member functions
==================================
  
**DemuxFrame()**
================

Demux the video file and returns a packet structure, see :doc:`Structures Classess <structures>`, filled with information to be used in decoding the demuxed frames.

	Returns: A filled 'packet' structure with information to be used in decoding the demuxed frames

**Example:**
	.. code-block:: python   

		while True:
			packet = demuxer.DemuxFrame()

			n_frame_returned = viddec.DecodeFrame(packet)

			for i in range(n_frame_returned):
				viddec.GetFrame(packet)

				# save frames
				surface_info = viddec.GetOutputSurfaceInfo()

				viddec.SaveFrameToFile(output_file_path, packet.frame_adrs, surface_info)

				# release frame
				viddec.ReleaseFrame(packet)

				if (packet.frame_size <= 0):  # EOF: no more to decode
					break
				
**GetCodecId()**
================

Gets codec ID, to be used in latter decoder APIs.

	Returns: Codec ID

**Example:**
	.. code-block:: python   

		# demuxer instance
		demuxer = dmx.demuxer(input_file_path)

		# get the used coded id
		codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())

		# decoder instance
		viddec = dec.decoder(device_id, mem_type, codec_id, b_force_zero_latency, crop_rect, 0, 0, 1000)
	
**SeekFrame(frame_number, seek_mode, seek_criteria)**
=====================================================

Demux the video file starting from the specified seek parameters, and returns a c, filled with information to be used in decoding the demuxed frames.

Input:
- frame_number:   Seek this number of frames
- seek_mode:      0 - by exact frame number, 1 - by previous key frame
- seek_criteria:  0 - by frame number, 1 - by time stamp

	Returns: A filled 'packet' structure with information to be used in decoding the demuxed frames

**Example:**
	.. code-block:: python   

		not_seeking = False

		seek_frame = 100     # seek 100 frames
		seek_mode = 0        # by exact frame number
		seek_criteria = 0    # by frame number
		
		# The decoding loop to start by seek
		while True:
			start_time = datetime.datetime.now()
			if(not_seeking):
				packet = demuxer.DemuxFrame()
			else:
				packet = demuxer.SeekFrame(seek_frame, seek_mode, seek_criteria)
				not_seeking = True


