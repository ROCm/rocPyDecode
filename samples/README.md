# rocPyDecode samples

## Prerequisites
* [rocDecode C/C++ Library](https://github.com/ROCm/rocDecode)
* [rocPyDecode installed](../README.md#rocpydecode-install)
* [HIP Python](https://rocm.docs.amd.com/projects/hip-python/en/latest/index.html)

```
python3 -m pip install --upgrade pip
python3 -m pip install -i https://test.pypi.org/simple hip-python
```

## videodecode.py

This sample demuxes & decode frames from a video file, and optionally saves the frames to a file. \
To run this python sample script, you need to provide input video file full path name, other arguments are optional.

### Arguments
The following are full list of arguments that can be passed to the sample.
```
-h, --help                                    : Show detail help message and exit
-i INPUT, --input INPUT                       : Input File Path - required
-o OUTPUT, --output OUTPUT                    : Output File Path - optional
-d DEVICE, --device DEVICE                    : GPU device ID - optional, default - 0
-z ZERO_LATENCY, --zero_latency ZERO_LATENCY  : Force zero latency - [options: yes,no], default: no
-crop CROP_RECT, --crop_rect CROP_RECT        : Crop rectangle (left, top, right, bottom) - optional, default: None (no cropping)
```

## videodecodeperf.py

This sample demuxes & decode frames from a video file on multiple parallel threads. The number of threads is a user given argument, defaulted to 4 threads. \
To run this python sample script, you need to provide input video file full path name, other arguments are optional.

### Arguments
The following are full list of arguments that can be passed to the sample.
```
-h, --help                           : Show detail help message and exit
-i INPUT, --input INPUT              : Input File Path - required
-t NUM_THREADS, --thread NUM_THREADS : Number of threads - optional, default - 0, range - [1,64]
-d DEVICE, --device DEVICE           : GPU device ID - optional, default - 0
```