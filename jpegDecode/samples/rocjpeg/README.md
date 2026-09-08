# rocPyJpegDecode samples

## Prerequisites
* [rocJPEG C/C++ Library](https://github.com/ROCm/rocJPEG)
* [rocPyJpegDecode installed](../../README.md)
* [DLPack](https://pypi.org/project/dlpack/)

## jpegdecode.py

This sample decodes single JPEG images, and optionally saves the decoded images. \
To run this python sample script, you need to provide input JPEG file full path name, other arguments are optional.

### Arguments

The following are full list of arguments that can be passed to the sample.

```bash
-h, --help                                 : Show detail help message and exit
-i INPUT, --input INPUT                    : Input File Path - required
-d DEVICE, --device DEVICE                 : GPU device ID - optional, default - 0
-fmt {3,4}, --output_format {3,4}          : Set output image format: 3 for ROCJPEG_OUTPUT_RGB (interleaved), 4 for ROCJPEG_OUTPUT_RGB_PLANAR. Optional, default is 3
-bk {0,1}, --backend {0,1}                 : Set backend choice 0:GPU and 1:CPU, Optional, default is 0
-o OUTPUT, --output OUTPUT                 : Output File Name Path - optional
```

## jpegdecodebatched.py

This sample decodes batch of JPEG images. \
The user specifies the root folder full path with the -i argument for multiple JPEG images with any number of files and/or subfolders.

### Arguments

The following are full list of arguments that can be passed to the sample.

```bash
-h, --help                                 : Show this help message and exit
-i INPUT, --input INPUT                    : Input Files FULL Path - required
-b BATCH, --batch BATCH                    : batch size > 0 process the batch of files with this batch size, if 0 means do not process as batch, optional, default is 2
-fmt {3,4}, --output_format {3,4}          : Set output image format: 3 for ROCJPEG_OUTPUT_RGB (interleaved), 4 for ROCJPEG_OUTPUT_RGB_PLANAR. Optional, default is 3
-bk {0,1}, --backend {0,1}                 : Set backend choice 0:GPU and 1:CPU, Optional, default is 0
-d DEVICE, --device DEVICE                 : GPU device ID - optional, default 0
```

## jpegdecodeperf.py

This sample decodes batch of JPEG images on multiple processes. \
The user can define the number of parallel jobs to observe performance scaling. \
The user specifies the root folder full path with the -i argument for multiple JPEG images with any number of files and/or subfolders. \
This sample distributes the workload on the available GPU devices if more than one was available.

### Arguments

The following are full list of arguments that can be passed to the sample.

```bash
-h, --help                                 : Show this help message and exit
-i INPUT, --input INPUT                    : Input Files FULL Path - required
-b BATCH, --batch BATCH                    : batch size > 0 process the batch of files with this batch size, if 0 means do not process as batch, optional, default is 2
-fmt {3,4}, --output_format {3,4}          : Set output image format: 3 for ROCJPEG_OUTPUT_RGB (interleaved), 4 for ROCJPEG_OUTPUT_RGB_PLANAR. Optional, default is 3
-bk {0,1}, --backend {0,1}                 : Set backend choice 0:GPU and 1:CPU, Optional, default is 0
-d DEVICE, --device DEVICE                 : GPU device ID - optional, default 0
-t NUM_PROCESS, --num_process NUM_PROCESS  : Num of parallel runs - optional, default 4
```

# Samples Notebook

The following list shows the provided notebooks samples that visually demonstrate the use of rocPyJpegDecode:

| Notebook File             | Description |
|---------------------------|-------------|
| `batch_sample.ipynb`      | This sample decodes a batch of multiple JPEG images, passing all the images full path as a list, or loading their data and passes it as a list of memory buffers. |
| `decode_rgb_planar.ipynb` | This sample decodes a JPEG image into 3 planes, red, green and blue, then it views each of those planes as a separate image and combines them in one RGB image in one row. |
| `decode_source.ipynb`     | This sample decodes one JPEG image, passing the image full path, or loading its data and passing it as memory buffer, and as a NumPy array. |
