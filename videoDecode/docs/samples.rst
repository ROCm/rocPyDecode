Video samples and tests
=======================

Prepare the SDK, development dependencies, runtime library paths, and test
media described in :doc:`install` before running samples or CTest. That guide
also lists the expected tests and explains which sample dependencies are optional.

Video samples are under ``videoDecode/samples/rocdecode`` and component tests
are under ``videoDecode/tests``. The default CTest suite covers binding types
and direct raw H.264 and H.265 GPU decoding.

Samples requiring FFmpeg, the rocDecode host library, PyTorch, hip-python, or
VAAPI remain available for explicit execution after those optional
dependencies are installed.
