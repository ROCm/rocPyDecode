# rocPyJpegDecode samples

## Prerequisites

Follow the [installation instructions](../install/rocPyDecode-install.rst) for the
complete SDK version requirements, development packages, test assets, and
`PYTHONPATH`/`LD_LIBRARY_PATH` setup.

`jpegdecode.py` imports ROCm-compatible PyTorch, NumPy, and Pillow. Install
those packages before running it. The batched and performance scripts do not
import those packages; the default CTest suite does not require them.

## jpegdecode.py

Decode one JPEG image and optionally save the output. Supply the image
path with `-i`. All other arguments are optional.

### Arguments

The sample accepts the following arguments. Use `--help` to view the current CLI.

```text
-h, --help                                 : Show detail help message and exit
-i INPUT, --input INPUT                    : Input File Path - required
-d DEVICE, --device DEVICE                 : GPU device ID - optional, default - 0
-fmt {3,4}, --output_format {3,4}          : Set output image format: 3 for ROCJPEG_OUTPUT_RGB (interleaved), 4 for ROCJPEG_OUTPUT_RGB_PLANAR. Optional, default is 3
-bk {0,1}, --backend {0,1}                 : Set backend choice 0:GPU and 1:CPU, Optional, default is 0
-o OUTPUT, --output OUTPUT                 : Output File Name Path - optional
```

## jpegdecodebatched.py

Decode JPEG images in batches from the directory supplied with `-i`,
including its subdirectories. Invalid arguments, unavailable GPUs, and folders
with no decodable images return a nonzero exit status. Mixed folders report
and skip bad files.

### Arguments

The sample accepts the following arguments. Use `--help` to view the current CLI.

```text
-h, --help                                 : Show this help message and exit
-i INPUT, --input INPUT                    : Input Files FULL Path - required
-b BATCH, --batch BATCH                    : Positive batch size, optional, default is 2
-fmt {3,4}, --output_format {3,4}          : Set output image format: 3 for ROCJPEG_OUTPUT_RGB (interleaved), 4 for ROCJPEG_OUTPUT_RGB_PLANAR. Optional, default is 3
-bk {0,1}, --backend {0,1}                 : Set backend choice 0:GPU and 1:CPU, Optional, default is 0
-d DEVICE, --device DEVICE                 : GPU device ID - optional, default 0
```

## jpegdecodeperf.py

Decode JPEG images with multiple processes to measure throughput. Supply
the input directory with `-i` and the process count with `-t`. The sample
distributes work across available GPUs.

### Arguments

The sample accepts the following arguments. Use `--help` to view the current CLI.

```text
-h, --help                                 : Show this help message and exit
-i INPUT, --input INPUT                    : Input Files FULL Path - required
-b BATCH, --batch BATCH                    : batch size > 0 process the batch of files with this batch size, if 0 means do not process as batch, optional, default is 2
-fmt {3,4}, --output_format {3,4}          : Set output image format: 3 for ROCJPEG_OUTPUT_RGB (interleaved), 4 for ROCJPEG_OUTPUT_RGB_PLANAR. Optional, default is 3
-bk {0,1}, --backend {0,1}                 : Set backend choice 0:GPU and 1:CPU, Optional, default is 0
-d DEVICE, --device DEVICE                 : GPU device ID - optional, default 0
-t NUM_PROCESS, --num_process NUM_PROCESS  : Num of parallel runs - optional, default 4
```

## Sample notebooks

The sample directory also contains these notebooks:

| Notebook File             | Description |
|---------------------------|-------------|
| `batch_sample.ipynb` | Decodes a batch of multiple JPEG images. Images are passed to the decoder either as a list of paths or by loading their data and passing a list of image buffers. |
| `decode_rgb_planar.ipynb` | Decodes a JPEG image into red, green, and blue planes. Displays the separate planes and the combined RGB image in one row. |
| `decode_source.ipynb` | Decodes one JPEG image. The image is passed to the decoder as a NumPy array, a memory buffer, or by providing its path. |
