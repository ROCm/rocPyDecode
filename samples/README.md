# rocPyDecode samples

## Prerequisites
* [rocDecode C/C++ Library](https://github.com/ROCm/rocDecode)
* [rocPyDecode installed](../README.md#rocpydecode-install)
* [DLPack](https://pypi.org/project/dlpack/)
* [pytorch for ROCm](https://pytorch.org/get-started/locally/)

The torch python sample requires pytorch for ROCm, which can be installed as follow:

- If using bare-metal, `sudo` access is needed.
```bash
    pip3 install --pre torch torchvision torchaudio --index-url https://download.pytorch.org/whl/nightly/rocm6.0
    sudo reboot 
```

- If using a docker environment or any system with `root` access, no need for reboot.
```bash
    pip3 install --pre torch torchvision torchaudio --index-url https://download.pytorch.org/whl/nightly/rocm6.0
```

## videodecode.py

This sample demuxes & decode frames from a video file, and optionally saves the frames to a file. \
To run this python sample script, you need to provide input video file full path name, other arguments are optional.

### Arguments
The following are full list of arguments that can be passed to the sample.
```bash
-h, --help                                    : Show detail help message and exit
-i INPUT, --input INPUT                       : Input File Path - required
-o OUTPUT, --output OUTPUT                    : Output File Path - optional
-d DEVICE, --device DEVICE                    : GPU device ID - optional, default - 0
-m MEM_TYPE, --mem_type MEM_TYPE 			  : Memory Type of output surfce - 0: Internal 1: dev_copied 2: host_copied - optional, default 1
-z ZERO_LATENCY, --zero_latency ZERO_LATENCY  : Force zero latency - [options: yes,no], default: no
-crop CROP_RECT, --crop_rect CROP_RECT        : Crop rectangle (left, top, right, bottom) - optional, default: None (no cropping)
```

## videodecodetorch.py

This sample demuxes & decode frames from a video file, and convert it to pytorch tensor via DLPack. Optionally you can save it to a file. \
To run this python sample script, you need to provide input video file full path name, other arguments are optional.

### Arguments
The following are full list of arguments that can be passed to the sample.
```bash
-h, --help                                    : Show detail help message and exit
-i INPUT, --input INPUT                       : Input File Path - required
-o OUTPUT, --output OUTPUT                    : Output File Path - optional
-d DEVICE, --device DEVICE                    : GPU device ID - optional, default - 0
-m MEM_TYPE, --mem_type MEM_TYPE 			  : Memory Type of output surfce - 0: Internal 1: dev_copied 2: host_copied - optional, default 1
-z ZERO_LATENCY, --zero_latency ZERO_LATENCY  : Force zero latency - [options: yes,no], default: no
-crop CROP_RECT, --crop_rect CROP_RECT        : Crop rectangle (left, top, right, bottom) - optional, default: None (no cropping)
```

## videodecodetorch_resnet50.py

This sample demuxes & decode frames from a video file, converts each frame to pytorch tensor via DLPack. \
It resizes the frame to match resnet50 model, and feed it to the model inference, printing out 5 possible predictions with precision ratio, for each frame it decodes. \
To run this python sample script, you need to provide input video file full path name, other argument is optional.

### Arguments
The following are full list of arguments that can be passed to the sample.
```bash
-h, --help                                    : Show detail help message and exit
-i INPUT, --input INPUT                       : Input File Path - required
-d DEVICE, --device DEVICE                    : GPU device ID - optional, default - 0
-m MEM_TYPE, --mem_type MEM_TYPE 			  : Memory Type of output surfce - 0: Internal 1: dev_copied 2: host_copied - optional, default 1
```

## videodedemux.py

This sample demuxes frames from a video file. \
To run this python sample script, you need to provide input video file full path name.

### Arguments
The following are full list of arguments that can be passed to the sample.
```bash
-h, --help                                    : Show detail help message and exit
-i INPUT, --input INPUT                       : Input File Path - required
```
