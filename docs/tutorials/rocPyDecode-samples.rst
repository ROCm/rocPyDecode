rocPyDecode samples and tests
=============================

Prepare :doc:`../install/rocPyDecode-install` before running samples. Samples
that import NumPy, ROCm-compatible PyTorch, hip-python, or mpi4py also need those
packages in the selected Python environment.

Video
-----

:doc:`video-samples` describes the scripts under
``videoDecode/samples/rocdecode``. Container-demuxing samples require PyAV.
``videodecodeperf.py`` uses hip-python to query GPU properties and device counts;
hip-python is not a dependency of the rocPyDecode library itself.

JPEG
----

:doc:`jpeg-samples` describes the scripts and notebooks under
``jpegDecode/samples/rocjpeg``. Batched decoding reports and skips bad inputs;
a folder containing no decodable images causes the sample to fail.
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
