Video samples and tests
=======================

Prepare the SDK, development dependencies, runtime library paths, and test
media described in :doc:`install` before running samples or CTest. That guide
also lists the expected tests and explains which sample dependencies are optional.

Video samples are under ``videoDecode/samples/rocdecode`` and component tests
are under ``videoDecode/tests``. The default CTest suite covers binding types
and direct raw H.264 and H.265 GPU decoding.

Install PyAV for samples that demux video containers. Samples that import
NumPy, PyTorch, hip-python, or mpi4py require those packages as well. The ``videodecodeperf.py`` sample uses
``hip-python`` to query GPU properties and the number of available devices;
it is not required by the rocPyDecode library.
