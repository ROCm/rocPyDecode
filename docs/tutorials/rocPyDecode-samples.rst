.. meta::
  :description: rocPyDecode samples and tests
  :keywords: rocPyJpegDecode, rocPyDecode, rocDecode, rocJPEG, ROCm, API, documentation, video, decode, decoding, samples

*******
Samples
*******

Follow :doc:`../install/rocPyDecode-install` before running samples. Samples
that import NumPy, ROCm-compatible PyTorch, hip-python, or mpi4py also need those
packages in the selected Python environment.

Video
-----

See :doc:`video-samples` for information about the samples in
``videoDecode/samples/rocdecode``.

Container-demuxing samples require PyAV.
``videodecodeperf.py`` uses hip-python to query GPU properties and device counts.
hip-python is not a dependency of the rocPyDecode library itself.

JPEG
----

See :doc:`jpeg-samples` for information about the samples in
``jpegDecode/samples/rocjpeg``.

Batched decoding skips bad inputs and reports them. A folder containing no
decodable images causes the sample to fail.
Performance and PyTorch examples are available after installing their optional
dependencies.

Tests
-----

See :ref:`installed-package-tests` for the installed CTest suites, or
:doc:`../install/rocPyDecode-cmake-install` for source-build tests. A skipped or
omitted test does not establish runtime correctness.

With NumPy and ROCm-compatible PyTorch installed, the optional JPEG tensor test
compares DLPack and NumPy pixels, single and batched decoding, filename and
memory inputs, and interleaved and planar RGB. From the repository root with
the installed bindings on ``PYTHONPATH``, run:

.. code-block:: shell

   python jpegDecode/tests/jpeg_tensor_test.py --media-dir "$ROCM_PATH/share/rocjpeg/images"
