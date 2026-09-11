JPEG samples and tests
======================

Prepare the SDK, development dependencies, runtime library paths, and test
media described in :doc:`install` before running samples or CTest. That guide
also lists the expected tests and explains which sample dependencies are optional.

JPEG samples and notebooks are under ``jpegDecode/samples/rocjpeg`` and
component tests are under ``jpegDecode/tests``. The dependency-minimal default
CTest suite validates batched JPEG decoding, RGB layout metadata, and failure
exit codes without requiring PyTorch. The batch sample fails when no image can
be decoded; mixed folders still report and skip bad files.

PyTorch-dependent and performance samples remain available for explicit
execution after their optional dependencies are installed.

Optional pixel regression
-------------------------

With NumPy and ROCm-compatible PyTorch installed, run the following from the
component directory after setting up the installed bindings as described in
:doc:`install`:

.. code-block:: shell

   python3.12 tests/jpeg_tensor_test.py --media-dir "$ROCM_PATH/share/rocjpeg/images"

This compares DLPack and NumPy pixels, single and batched decoding, filename
and memory inputs, and interleaved and planar RGB for the SDK test images.
