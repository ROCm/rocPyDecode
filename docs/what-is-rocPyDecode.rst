.. meta::
  :description: What is rocPyDecode?
  :keywords: video decoding, rocPyDecode, rocDecode, AMD, ROCm

********************************************************************
What is rocPyDecode?
********************************************************************

AMD GPUs contain one or more media engines (VCNs) that provide fully accelerated, hardware-based
video decoding. Hardware decoders consume lower power than CPU-based decoders. Dedicated
hardware decoders offload decoding tasks from the CPU, boosting overall decoding throughput. With
proper power management, decoding on hardware decoders can lower the overall system power
consumption and improve decoding performance.

Using the rocPyDecode Python API, you can decode compressed video streams while keeping the resulting YUV
frames in video memory. With decoded frames in video memory, you can run video post-processing
using ROCm HIP, thereby avoiding unnecessary data copies via PCIe bus. You can post-process video
frames using scaling or color conversion and augmentation kernels (on a GPU or host) in a format for
GPU/CPU-accelerated inferencing and training.

rocPyDecode API uses the rocDecode in the backend to do single or multiple instance video decoding based on the
number of available VCNs in AMD GPU device. In addition, rocPyDecode can seemlessly interop with other machine learning
frameworks like PyTorch to facilitate machine learning application on decoded surfaces. It also allows to use FFMpeg 
demultiplexer to seek and demux packetized media file.

rocDecode Python Binding, rocPyDecode, is a tool that allows users to access 
rocDecode APIs in both Python and C/C++ languages. It works by connecting 
Python and C/C++ libraries, enabling function calling and data passing between the two languages. 
The rocpydecode.so library is a wrapper that facilitates the use of rocDecode APIs that are 
written primarily in C/C++ language within Python.