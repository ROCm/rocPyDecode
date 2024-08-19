.. _decoder_section:

The Decoder Class
=================

To instantiate a decoder instance, pass the following parameters:  

- device_id  
- mem_type  
- codec  
- b_force_zero_latency  
- crop_rect  
- max_width  
- max_height  
- clk_rate

**Example:**
	
	# create decoder instance
	
	viddec = dec.decoder(device_id, mem_type, codec_id, b_force_zero_latency, crop_rect, 0, 0, 1000)  

- **device_id:**	The GPU device ID, default is 0
- **mem_type:**		The mem type of output surface - 0: Internal 1: dev_copied 2: host_copied, default is 1
- **codec:**        The codec ID obtained by *GetRocDecCodecID* API
- **b_force_zero_latency** Force zero latency flag, default is 'False'
- **crop_rect:**    See :doc:`Structures Classess <structures>`, optional, default: 'None', no cropping
- **max_width:**    Max width, default is 0
- **max_height:**   Max height, default is 0
- **clk_rate:**     Clock rate, default is 1000  


The decoder class member functions
==================================
  
**GetGpuInfo()**
============

Obtains a ConfigInfo, a :doc:`Structures Classess <structures>`, filled with current GPU device information

	Returns: A filled configuration structure with GPU information

**Example:**
	.. code-block:: python
		
		# Get GPU device information
		cfg = viddec.GetGpuInfo()
		
		# print GPU info out
		print("GPU device " + str(device_id) + " \- " + cfg.device_name + "[" + cfg.gcn_arch_name + "] on PCI bus " + str(cfg.pci_bus_id) + ":" + str(cfg.pci_domain_id) + "." + str(cfg.pci_device_id))


**DecodeFrame(packet)**
=======================

Decodes video frames described by the passed packet, see: :doc:`Structures Classess <structures>`. 

	Input: The demuxed packet contains the demuxed frames information
	
	Returns: The count of frames decoded.


**GetFrame(packet)**
====================

Gets a pointer to the current decoded frame.

	Input: 
	- packet: The demuxed packet contains the demuxed frames information.
  
	Return: Frame time stamp, the frame pointer and the time stamp saved in the passed packet.

**Example:**
	.. code-block:: python

		# decoding loop
		while True:
			packet = demuxer.DemuxFrame()

			n_frame_returned = viddec.DecodeFrame(packet)

			for i in range(n_frame_returned):
				viddec.GetFrame(packet)

			# TO DO: process the frame or save it, etc.
			# ...

			# release frame
			viddec.ReleaseFrame(packet)

			if (packet.frame_size <= 0):  EOF: no more to decode
				break

				
**GetFrameRgb(packet, rgb_format)**
===================================

Gets a pointer to the current decoded frame and converts that YUV frame to 'Tensor' in RGB format.

	Input: 
	- packet: The demuxed packet contains the demuxed frames information, and the desired rgb format
	- rgb_format: 1 for bgr, 3 for rgb

	Return: Frame time stamp, the frame pointer and the time stamp saved in the passed packet.


**ResizeFrame(packet, resize_dim, surface_info)**
===================================================

Resizes the decode frame pointed to by the passed packet, see: :doc:`Structures Classess <structures>`, to the new dimension in resize_dim, to the new dimension using the passed surface info.

Input:  
- packet: The demuxed packet contains the demuxed frames information, and the desired rgb format  
- resize_dim:  The new dimension, width and height 
- surface_info: The current surface info obtained by GetOutputSurfaceInfo API

**Example:**
	.. code-block:: python

		# resize frame to new dimension
		resize__dim = [1024, 720]

		surface__info = viddec.GetOutputSurfaceInfo()

		frame_is_resized = False

		if(viddec.ResizeFrame(packet, resize_dim, surface_info) != 0):
			frame_is_resized = True

**GetWidth()**
==============

Gets the width of the current decoded frame.

	Return: The width of the current decoded frame.

**GetHeight()**
=================

Gets the height of the current decoded frame.

	Return: The height of the current decoded frame.

**GetStride()**
=================

Gets the stride of the current decoded frame.

	Return: The stride of the current decoded frame.

**GetFrameSize()**
==================

Gets the size of the current decoded frame in bytes.

	Return: The size of the current decoded frame in bytes.

**GetOutputSurfaceInfo()**
==========================

Obtains the current decode frame surface's information structure pointer.

	Return: Pointer to current decode frame surface's information structure.

**GetResizedOutputSurfaceInfo()**
=================================

Obtains the current decoded and resized frame's surface information structure pointer.

	Return: Pointer to current decode and resized frame's surface information structure.

**SaveFrameToFile(output_file_path, frame_adrs, surface_info)**
===============================================================

Saves all the decoded frames to a disk file in YUV format.

Input: 
- output_file_path: The full path disk file name to save the YUV frames
- frame_adrs: The current frame pointer, obtained from the used packet
- surface_info: The current decode frame surface information structure pointer

**SaveTensorToFile(output_file_path, frame_adrs, width, height, rgb_format, surface_info)**
===========================================================================================

Saves all the decoded frames after being converted to a Tensor to a disk file in RGB format.

Input: 
- output_file_path: The full path disk file name to save the YUV frames
- frame_adrs: The current frame/tensor pointer, obtained from the used packet
- width: The width of the current Tensor
- height: The height of the current Tensor
- rgb_format: 1 for bgr, 3 for rgb 
- surface_info: The current decode frame surface information structure pointer

**ReleaseFrame(packet)**
==========================

Release the GPU memory of the current decoded frame.

**GetNumOfFlushedFrames()**
===========================

Get the count of the flushed frames.  

**Example:**
	.. code-block:: python  
		
		# beyond the decoding loop
		n_frame += viddec.GetNumOfFlushedFrames()  
	
**InitMd5()**
=============

Initialize the process to obtain the MD5 of the decoded frames.  

**Example:**
	.. code-block:: python   

		# init MD5 
		viddec.InitMd5()
		
**UpdateMd5ForFrame(frame_adrs, surface_info)**
=================================================

Update the MD5 calculation with the current decoded frame.  

Input: 
- frame_adrs: The current frame/tensor pointer, obtained from the used packet
- surface_info: The current decode frame surface information structure pointer

**Example:**
	.. code-block:: python  

		# update MD5 with current decoded frame

		surface_info = viddec.GetOutputSurfaceInfo()

		viddec.UpdateMd5ForFrame(packet.frame_adrs, surface_info)

**FinalizeMd5()**
===================

Ends the MD5 process and returns the digest 16 character.  

**Example:**
	.. code-block:: python 

		# finalize and print MD5 check 
		digest = viddec.FinalizeMd5()

		print("MD5 message digest: ", end=" ")

		str_digest = ""

		for i in range(16):

			str_digest = str_digest + str(format('%02x' % int(digest[i])))

		print(str_digest)
		
**SetReconfigParams(flush_mode, out_file_name)**
================================================

Specify the flush mode and the output file name to use in multi resolution video support.  

Input: 

- flush_mode: 

		0: Just flush to get the frame count

		1: The remaining frames will be dumped to file in this mode

		2: Calculate the MD5 of the flushed frames

- out_file_name: 
  
	The full path disk file name to save the YUV frames

**Example:**
	.. code-block:: python  

		# set reconfiguration params based on user arguments
		flush_mode = 0

		if (output_file_path is not None):
			flush_mode = 1
		elif b_generate_md5:
			flush_mode = 2

		viddec.SetReconfigParams(flush_mode, output_file_path if (output_file_path is not None) else str(""))
		

