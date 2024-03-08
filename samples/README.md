# rocPyDecode sample: py_videodecode.py

To run this sample python script you need to provide input video file full path name, all other arguments are optional. If you chose to provide md5 input file, the file must exist on the path specified in its name. The following are full list of all arguments that can be passed to the sample:

## Prerequisites
* [rocDecode C/C++ Library](https://github.com/ROCm/rocDecode)
* rocPyDecode installed (use pip3 install . on main folder)

### PyRocDecode Video Decode Arguments:

options:
  -h, --help            show this help message and exit
  -i INPUT, --input INPUT
                        Input File Path - required
  -o OUTPUT, --output OUTPUT
                        Output File Path - optional
  -d DEVICE, --device DEVICE
                        GPU device ID - optional, default 0
  -z ZERO_LATENCY, --zero_latency ZERO_LATENCY
                        Force zero latency - [optios: yes,no], default: no
  -sei EXTRACT_SEI, --extract_sei EXTRACT_SEI
                        Extract SEI messages - [optios: yes,no], default: no
  -md5 GENERATE_MD5, --generate_md5 GENERATE_MD5
                        Generate MD5 message digest - [optios: yes,no], default: no
  -md5_check INPUT_MD5, --input_md5 INPUT_MD5
                        Input MD5 File Path, optional, if passed then -md5 set to yes
  -crop CROP_RECT [CROP_RECT ...], --crop_rect CROP_RECT [CROP_RECT ...]
                        Crop rectangle (left, top, right, bottom), optional, default: no cropping
  -m MEM_TYPE, --mem_type MEM_TYPE
                        Output surface memory type, default 0, [options: 0:OUT_SURFACE_MEM_DEV_INTERNAL, 1:OUT_SURFACE_MEM_DEV_COPIED, 2:OUT_SURFACE_MEM_HOST_COPIED]
