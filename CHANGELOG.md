# rocPyDecode changelog

Documentation for rocPyDecode is available at
[https://rocm.docs.amd.com/projects/rocPyDecode/en/latest/](https://rocm.docs.amd.com/projects/rocPyDecode/en/latest/)

## rocPyDecode 0.2.0 (Unreleased)

### Changes

* Clang - Default CXX compiler
* Rgb & Yuv pytorch tensors
* Producing python standard wheel (.whl)
* Updated rocPyDecode README
* Updated samples README

### Removals

* CTest - Core tests for make test and package test
* hipcc - now using default CXX

### Optimizations

* Setup Script - Build and runtime install options
* pre-requisite installation helper python scripts
* Same GPU memory viewed as pytorch tensor

### Resolved issues

* Setup
  * no dependency on hipcc
  * building rocPyDecode only once
* Sample
  * multiple use cases samples added

### Known issues

### Upcoming changes

### Tested configurations

* Linux
  * Ubuntu - `20.04` / `22.04`
  * RHEL - `8` / `9`
  * SLES - `15 SP5`
* ROCm:
  * rocm-core - `6.2.0.60200-66`
  * amdgpu-core - `1:6.2.60200-2009582`
* rocDecode
  * latest release 0.7.0

## rocPyDecode 0.1.0

### Additions

* Clang - Default CXX compiler
* Full rocDecode functionality
* Supporting all rocDecode codecs

### Optimizations

* Setup Script - Build and runtime install options
* pre-requisite installation helper python scripts

### Changes

* Samples - added more use cases samples
* Supported codecs - a check is added

### Fixes

### Upcoming changes
* Clang - Default CXX compiler
* Supported codecs - adding check if the input video codec is supported
* Setup to produce python wheel for end-user

### Tested configurations

* Linux
  * Ubuntu - `20.04` / `22.04`
  * RHEL - `8` / `9`
  * SLES - `15 SP5`
* ROCm:
  * rocm-core - `6.2.0.60200-66`
  * amdgpu-core - `1:6.2.60200-2009582`
* rocDecode - version `0.6.0'
