.. meta::
  :description: rocPyJpegDecode API documentation
  :keywords: rocPyJpegDecode, rocDecode, rocJPEG, ROCm, API, documentation, video, decode, decoding, acceleration

*********************************
rocPyJpegDecode Python API
*********************************

The rocPyJpegDecode API is available in the rocDecode GitHub repository under the `pyRocJpegDecode <https://github.com/ROCm/rocPyDecode/tree/develop/pyRocJpegDecode>`_ directory.

The following rocPyJpegDecode API calls are used to decode a JPEG image. 

``initialize_hip()``
	Initializes the HIP device for decoding. The user can specify the device to use for decoding. By default device 0 is used. 
	
	``initialize_hip()`` must be called before any other call is made to the rocPyJpeg library.
		
``decoder()``
	Constructor for the main decoding class.

``decoder().set_output_image_format()``
	Passes the output image format to the decoder engine. The output image format can be set to ``ROCJPEG_OUTPUT_RGB`` or ``ROCJPEG_OUTPUT_RGB_PLANAR``. These types are defined in ``pyRocJpegDecode/types.py``. 
		
``decoder().read`` and 	``decoder().decode``
	Both these functions read in a JPEG image or batch of JPEG images to be decoded and pass them to the decoder, then return the decoded images. 
	
	If only one image is passed to the function, it will return one image tensor in the specified output format. If more than one image is passed to the function, it will return a list of image tensors in the specified output format.

	If any image fails to be decoded because it's in an unsupported format or is corrupted, no image will be returned and ``decode`` will produce an error message. 

	Input can be any of the following:

	* The full path to one image.
	* The full path to a folder containing multiple images. The batch size must also be passed to the function.
	* One image stored as a memory buffer
	* Multiple images stored as memory buffers. The batch size must also be passed to the function.
	* One image stored as a NumPy array.
	* Multiple images stored as NumPy arrays. The batch size must also be passed to the function.
		