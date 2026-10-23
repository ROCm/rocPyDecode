# rocPyVideoDecode samples

The video samples demonstrate GPU decoding, color conversion, resizing, and
PyTorch interoperability. They also include examples of decoding from memory
and distributing work across processes.

## Prerequisites

Follow the [installation instructions](../install/rocPyDecode-install.rst) for the
complete SDK version requirements, development packages, test assets, and
`PYTHONPATH`/`LD_LIBRARY_PATH` setup.

Install the dependencies required by the samples you want to run:

- **Container demuxing:** Install PyAV using the component's
  [requirements.txt](https://github.com/ROCm/rocPyDecode/blob/develop/videoDecode/requirements.txt),
  following the installation instructions above.
- **PyTorch samples:** Install ROCm-compatible PyTorch. The ResNet-50 sample also
  requires torchvision.
- **Performance sample:** Install hip-python for GPU-property and device-count
  queries.
- **MPI sample:** Install an MPI runtime, mpi4py, ROCm-compatible PyTorch,
  NumPy, and PyAV.

The following settings apply to the video samples and tests:

- **Codecs:** Samples accept H.264, HEVC, VP9, and AV1, subject to GPU and SDK
  support.
- **Output memory:** For scripts with a `--mem_type` option, the default is copied
  device memory (1). The performance sample defaults to unmapped output (3).
- **Test input:** The Python API test uses an H.264 MP4 file from the SDK. This
  fixture does not limit the codecs supported by the samples.
- **Test dependencies:** Both video runtime tests require PyAV. The pixel
  regression test also requires NumPy. Both tests remain registered when
  prerequisites are missing and report a skip at runtime. Install the missing
  prerequisites and rerun CTest without reconfiguring.

## videodecode.py

Demux and decode a video file, optionally saving decoded YUV frames. Supply
the input path with `-i`. All other arguments are optional.

### Arguments
The sample accepts the following arguments. Use `--help` to view the current CLI.
```text
-h, --help                                       : Show detail help message and exit
-i INPUT, --input INPUT                          : Input File Path - required
-o OUTPUT, --output OUTPUT                       : Output File Path - optional
-d DEVICE, --device DEVICE                       : GPU device ID - optional, default - 0
-m MEM_TYPE, --mem_type MEM_TYPE                 : Memory Type of output surface - 0: Internal 1: dev_copied 2: host_copied 3: MEM not mapped - optional, default 1
-z ZERO_LATENCY, --zero_latency ZERO_LATENCY     : Force zero latency - [options: yes,no], default: no
-crop CROP_RECT, --crop_rect CROP_RECT           : Crop rectangle (left, top, right, bottom) - optional, default: None (no cropping)
-s SEEK, --seek SEEK                             : seek this number of frames, optional, default: no seek
-sm SEEK_MODE, --seek_mode SEEK_MODE             : seek mode, 0 - exact packet position, 1 - previous keyframe, optional, default: 1 - by previous key frame
-sc SEEK_CRITERIA, --seek_criteria SEEK_CRITERIA : seek criteria, 0 - by frame number, 1 - by time stamp, optional, default: 0 - by frame number
-resize RESIZE_DIM RESIZE_DIM, --resize_dim RESIZE_DIM RESIZE_DIM : Width & Height of new resized frame, optional, default: no resizing
-p {yes,no}, --planes {yes,no}                  : Export separate YUV planes, default: no
```

For inter-predicted video, seek to a previous keyframe and decode forward.
See the [demuxer reference](../reference/demuxerClass.rst) for exact-packet
seeking limits and timestamp units.

## videodecodemem.py

This sample reads the input video into memory and demuxes it through a memory
stream. It supports decoding, seeking, cropping, and resizing. Run
`python videoDecode/samples/rocdecode/videodecodemem.py --help` from the repository
root for its arguments.

## videodecodeperf.py

Run multiple decoding processes to measure throughput. Supply the input path
with `-i` and select the process count with `-t`.

### Arguments

The sample accepts the following arguments. Use `--help` to view the current CLI.

```text
-h, --help                                                  : Show detail help message and exit
-i INPUT, --input INPUT                                     : Input File Path - required
-d DEVICE, --device DEVICE                                  : GPU device ID - optional, default - 0
-m MEM_TYPE, --mem_type MEM_TYPE                            : Memory Type of output surface - 0: Internal 1: dev_copied 2: host_copied 3: MEM not mapped - optional, default 3
-t Number of Processes, --num_process Number of Processes   : Number of Processes - optional, default 4
```

## videodecodergb.py

Decode video frames and convert them to the selected RGB output format.
Supply the input path with `-i`. Use `-o` to save raw output bytes.

### Arguments
The sample accepts the following arguments. Use `--help` to view the current CLI.
```text
-h, --help                                    : Show detail help message and exit
-i INPUT, --input INPUT                       : Input File Path - required
-o OUTPUT, --output OUTPUT                    : Output File Path - optional
-d DEVICE, --device DEVICE                    : GPU device ID - optional, default - 0
-m MEM_TYPE, --mem_type MEM_TYPE              : Memory Type of output surface - 0: Internal 1: dev_copied 2: host_copied 3: MEM not mapped - optional, default 1
-z ZERO_LATENCY, --zero_latency ZERO_LATENCY  : Force zero latency - [options: yes,no], default: no
-crop CROP_RECT, --crop_rect CROP_RECT        : Crop rectangle (left, top, right, bottom) - optional, default: None (no cropping)
-of RGB_FORMAT, --rgb_format RGB_FORMAT       : Rgb Format to use - 1:bgr, 3:rgb, converts decoded YUV frame to Tensor in RGB format, optional, default: 3
```

## videodecodetorch.py

Decode video and export frames to PyTorch through DLPack. Supply the input
path with `-i`. Output-file saving is optional.

### Arguments
The sample accepts the following arguments. Use `--help` to view the current CLI.
```text
-h, --help                                    : Show detail help message and exit
-i INPUT, --input INPUT                       : Input File Path - required
-o OUTPUT, --output OUTPUT                    : Output File Path - optional
-d DEVICE, --device DEVICE                    : GPU device ID - optional, default - 0
-m MEM_TYPE, --mem_type MEM_TYPE              : Memory Type of output surface - 0: Internal 1: dev_copied 2: host_copied 3: MEM not mapped - optional, default 1
-z ZERO_LATENCY, --zero_latency ZERO_LATENCY  : Force zero latency - [options: yes,no], default: no
-crop CROP_RECT, --crop_rect CROP_RECT        : Crop rectangle (left, top, right, bottom) - optional, default: None (no cropping)
```

## videodecodetorch_yuv.py

Export separate decoded YUV planes to PyTorch through DLPack. Supply the
input path with `-i`. When saving output, `-y yes` selects luma and `-y no`
selects chroma.

### Arguments
The sample accepts the following arguments. Use `--help` to view the current CLI.
```text
-h, --help                                    : Show detail help message and exit
-i INPUT, --input INPUT                       : Input File Path - required
-o OUTPUT, --output OUTPUT                    : Output File Path - optional
-y {yes,no}, --yplane {yes,no}                : Save luma when yes, chroma when no; default: yes
-d DEVICE, --device DEVICE                    : GPU device ID - optional, default - 0
-m MEM_TYPE, --mem_type MEM_TYPE              : Memory Type of output surface - 0: Internal 1: dev_copied 2: host_copied 3: MEM not mapped - optional, default 1
-z ZERO_LATENCY, --zero_latency ZERO_LATENCY  : Force zero latency - [options: yes,no], default: no
-crop CROP_RECT, --crop_rect CROP_RECT        : Crop rectangle (left, top, right, bottom) - optional, default: None (no cropping)
```

## videodecodetorch_resnet50.py

Decode frames, export them to PyTorch through DLPack, and resize them for
ResNet-50 inference. The sample prints the top five predictions for each frame.
Supply the input path with `-i`. An ImageNet labels file is optional.

### Arguments
The sample accepts the following arguments. Use `--help` to view the current CLI.
```text
-h, --help                                    : Show detail help message and exit
-i INPUT, --input INPUT                       : Input File Path - required
-d DEVICE, --device DEVICE                    : GPU device ID - optional, default - 0
-o OUTPUT, --output OUTPUT                    : Optional output file
-of RGB_FORMAT, --rgb_format RGB_FORMAT       : RGB output format, default: 3
--labels LABELS                              : Optional ImageNet labels file
```

## videodecode_mpi.py

This sample distributes video decoding across MPI processes and uses MPI-IO
to write each process's output. Launch with `mpiexec -n N` or the host configuration
required by your MPI installation. Each participating host needs compatible
GPU hardware, software, and access to the input file.

### Prerequisites

Install an MPI implementation and mpi4py compatible with it, plus ROCm-compatible
PyTorch, NumPy, and PyAV in the Python environment used by the MPI ranks.

### Example of shell command

Run from the repository root after configuring the installed bindings.
This command starts four local MPI processes and saves decoded frames to
`outputfile.yuv`. Use input supported by the selected GPU and SDK.

```bash
mpiexec -n 4 python videoDecode/samples/rocdecode/videodecode_mpi.py \
    -i "$ROCM_PATH/share/rocdecode/video/AMD_driving_virtual_20-AV1.mp4" \
    -o outputfile.yuv
```

### Arguments
The sample accepts the following arguments. Use `--help` to view the current CLI.
```text
-h, --help                                    : Show detail help message and exit
-i INPUT, --input INPUT                       : Input File Path - required
-o OUTPUT, --output OUTPUT                    : Output File Path - optional
```
