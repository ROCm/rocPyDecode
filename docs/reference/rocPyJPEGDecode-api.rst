.. meta::
  :description: rocPyJpegDecode API documentation
  :keywords: rocPyJpegDecode, rocJPEG, ROCm, API, documentation, JPEG, image, decode, acceleration

*********************************
rocPyJpegDecode Python API
*********************************

The rocPyJpegDecode API is available in the ``pyRocJpegDecode`` directory.

The following rocPyJpegDecode API calls are used to decode a JPEG image.

``initialize_hip()``
	Initializes the HIP device for decoding. The user can specify the device to use for decoding. By default device 0 is used.

	``initialize_hip()`` must be called before any other call is made to the rocPyJpeg library.

``decoder()``
	Constructor for the main decoding class.

``decoder().set_output_image_format()``
	Passes the output image format to the decoder engine. The output image format can be set to ``ROCJPEG_OUTPUT_RGB`` or ``ROCJPEG_OUTPUT_RGB_PLANAR``. These types are defined in ``pyRocJpegDecode/types.py``.

``decoder().read(jpeg_item_to_decode)`` and ``decoder().decode(jpeg_item_to_decode)``
    Both methods decode a single encoded JPEG input or a list of inputs. Each
    returns a two-item tuple: ``(elapsed_time_in_msec, image_or_images)``.
    The time measures the native decode call in milliseconds, not the complete
    Python call including input preparation and output allocation.

    A single input returns one image object as the second item. A list input
    returns a list of decoded image objects, including when the input list
    contains only one item. There is no separate batch-size argument: divide
    inputs into lists of the desired size before calling the method.

    Supported input forms include a filename string, encoded JPEG ``bytes``,
    a NumPy uint8 array containing encoded JPEG bytes, or a ``DecodeSource``.
    Pass a list of these inputs for batch decoding. A NumPy input represents
    the compressed JPEG stream, not an already decoded pixel array.
    To decode a folder, enumerate its image files and pass their paths as a
    list; a directory path is not itself a batch input.

    Invalid streams or unsupported images can yield an uninitialized image
    for a single input or be skipped in a batch. A batch can therefore return
    fewer images than requested, or an empty list. Native decode failures can
    raise ``RuntimeError``. Do not assume that a returned image has valid
    pixels or that a batch preserves one output slot per input.

Example:
========

.. code-block:: python

    from pathlib import Path
    import pyRocJpegDecode.decoder as jdec

    _, ready = jdec.initialize_hip()
    if not ready:
        raise RuntimeError("GPU initialization failed")
    decoder = jdec.decoder()
    elapsed_ms, image = decoder.decode("image.jpg")
    filenames = [str(path) for path in sorted(Path("images").glob("*.jpg"))]
    elapsed_ms, images = decoder.decode(filenames)

Pixel exports
=============

``image.to_numpy(index=0)`` copies the selected decoded plane into a new
host NumPy uint8 array. The array owns its storage and remains valid after
image or decoder deletion; it is not a view of GPU memory.

For interleaved RGB, ``image.to_numpy()`` has shape ``(height, width, 3)``.
For planar RGB, call ``image.to_numpy(i)`` for each channel index 0, 1, and 2;
each array has shape ``(height, width)``.

With ROCm-compatible PyTorch, ``torch.from_dlpack(image)`` exports the first
buffer: the complete interleaved RGB image or the first plane for planar RGB.
Use ``torch.from_dlpack(image.ext_buf[i])`` to access individual planar
channels. DLPack tensors retain the GPU allocation after image or decoder
deletion. GPU rows can include pitch padding; consumers must respect the
exported strides instead of assuming tightly packed rows.
