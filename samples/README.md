# rocPyDecode sample: py_videodecode.py
~~~
This sample demux & decode frames from a video file, and optionally saves the frames to a file. 
To run this python sample script, you need to provide input video file full path name, other arguments are optional.
The following are full list of arguments that can be passed to the sample:
~~~
## Prerequisites
* [rocDecode C/C++ Library](https://github.com/ROCm/rocDecode)
* [rocPyDecode](https://github.com/ROCm/rocPyDecode) [installed](../README.md).

### Video Decode Sample 'py_videodecode.py' Arguments:
```
-h, --help            
  Show detail help message and exit
-i INPUT, --input INPUT
  Input File Path - required
-o OUTPUT, --output OUTPUT
  Output File Path - optional
-d DEVICE, --device DEVICE
  GPU device ID - optional, default 0
-z ZERO_LATENCY, --zero_latency ZERO_LATENCY
  Force zero latency - [options: yes,no], default: no
-crop CROP_RECT [CROP_RECT ...], --crop_rect CROP_RECT [CROP_RECT ...]
  Crop rectangle (left, top, right, bottom), optional, default: no cropping
```