Video samples and tests
=======================

Video samples are under ``videoDecode/samples/rocdecode`` and component tests
are under ``videoDecode/tests``. The default CTest suite covers binding types
and direct raw H.264 and H.265 GPU decoding.

Samples requiring FFmpeg, the rocDecode host library, PyTorch, hip-python, or
VAAPI remain available for explicit execution after those optional
dependencies are installed.
