.. _structures_section:

rocPyDecode **structures**
==========================

The following are the structures that are used in API calls:

- Packet  
- ConfigInfo
- Dim  
- Rect

The **Packet** structure
========================

- **end_of_stream**: Boolean flag, indicates demuxing reach end of stream when 'True'  
- **pkt_flags**: Integer value indicate the status of the packet  
- **frame_pts**: The time stamp of the current frame  
- **frame_size**: The current packet size  
- **frame_adrs**: The current YUV frame address in memory
- **frame_adrs_rgb**: The current RGB frame address in memory, valid only when RGB conversion used  
- **frame_adrs_resized**: The current resized frame address in memory, valid only when frame is resized  

**Example:**
	.. code-block:: python

		packet = demuxer.DemuxFrame()

		n_frame_returned = viddec.DecodeFrame(packet)

		viddec.UpdateMd5ForFrame(packet.frame_adrs, surface_info)

The **ConfigInfo** structure
============================

- **device_name**: String Contains the device name  
- **gcn_arch_name**: String Contains the GPU architecture name  
- **pci_bus_id**: Integer value contains the PCIE bus ID  
- **pci_domain_id**: Integer value contains the PCIE domain ID  
- **pci_device_id**: Integer value contains the PCIE device ID  

**Example:**
	.. code-block:: python   

		# Get GPU device information
		cfg = viddec.GetGpuInfo()

		#  print GPU info out
		print("GPU device " + str(device_id) + " \- " + cfg.device_name + "[" + cfg.gcn_arch_name + "] on PCI bus " + str(cfg.pci_bus_id) + ":" + str(cfg.pci_domain_id) + "." + str(cfg.pci_device_id)) 
	
The **Dim** structure
=====================

- **width**: Integer value contains the width  
- **height**: Integer value contains the height  

**Example:**
	.. code-block:: python

		import rocPyDecode as rocpydec

		frame_is_resized = False

		resize_dim = rocpydec.Dim()

		resize_dim.width = 200

		resize_dim.height = 200

		surface_info = viddec.GetOutputSurfaceInfo()

		if(viddec.ResizeFrame(packet, resize_dim, surface_info) != 0):

			frame_is_resized = True

The **Rect** structure
======================

- **left**: Integer value contains the left column of a rectangle
- **top**: Integer value contains the top row of a rectangle
- **rigth**: Integer value contains the right column of a rectangle
- **bottom**: Integer value contains the bottom row of a rectangle  

**Example:**
	.. code-block:: python  

		import rocPyDecode as rocpydec

		p_crop_rect = rocpydec.Rect()

		p_crop_rect.left = 100

		p_crop_rect.top = 100

		p_crop_rect.right = 300

		p_crop_rect.bottom = 300


