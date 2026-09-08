# rocPyDecode changelog

Full documentation for for rocPyDecode is available at [https://rocm.docs.amd.com/projects/rocPyDecode/en/latest/](https://rocm.docs.amd.com/projects/rocPyDecode/en/latest/)

## (Unreleased) rocPyDecode 1.0.0

### Added

* Added independent `videoDecode` and `jpegDecode` CMake projects, each with
  component-owned bindings, samples, tests, support sources, documentation,
  installation rules, and licensing.
* Added root options to build both components or select either component
  independently.
* Added component ownership, implementation-plan, and validation documents.
* Added dependency-minimal CTest suites for direct H.264/H.265 raw video
  decoding and batched JPEG decoding.

### Changed

* Replaced the monolithic root build with a lightweight umbrella orchestrator.
* Moved all video and JPEG files into their respective component directories.
* Duplicated the buffer and DLPack implementations as private, independently
  owned child sources so neither project depends on its sibling.
* Registered each private `BufferInterface` binding as module-local so the
  video and JPEG extensions can coexist in one Python process.
* Made ImageNet labels optional in the ResNet50 video sample through the
  `--labels` argument.
* Limited default CTest registration to tests without optional FFmpeg host,
  PyTorch, hip-python, or VAAPI-sensitive sample dependencies.

### Removed

* Removed the root `data` and `conda-recipe` directories.
* Removed obsolete wheel, setup, dependency-install, and Docker-install helper
  files. Wheel generation remains outside the scope of this release.
* Removed obsolete root source, package, sample, and test layouts after moving
  their maintained content into the independent child projects.

### Validation

* Verified standalone video and JPEG configure, build, install, import, and
  CTest workflows.
* Verified root video-only, JPEG-only, and combined workflows.
* Verified all four combined dependency-minimal CTests and both installed
  Python bindings on an AMD GPU system.

## (Unreleased) rocPyDecode 0.9.0 

### Added
* Added raw Annex‑B Python sample and CTest cases for H.264/H.265.
* Build no longer stops if rocDecode/FFmpeg are missing: rocPyDecode builds GPU-only, and rocPyJPEG builds independently.

### Changed
* Tests no longer depend on FFmpeg to run the raw sample.
* CPU backend samples no longer queries GPU info (pure CPU path).
* The default AMD clang compiler location has changed to `${ROCM_PATH}/lib/llvm/bin`.
* rocPyDecode now installs even when FFmpeg isn't installed.

### Resolved issues
* Fixed missing raw sample tests when FFmpeg is absent.
* Prevented CPU torch sample crash from invalid device name decoding.

##  (Unreleased) rocPyDecode 0.8.0

### Added
* 

### Changed
* CXX Compiler location - Use default `${ROCM_PATH}/lib/llvm/bin` for amd clang

### Resolved issues
* 

## rocPyDecode 0.7.0 for ROCm 7.1.0

### Added
* rocPyJpegPerfSample - samples for JPEG decode

### Changed
* Package - rocjpeg set as required dependency
* rocDecode host - rocdecode host linking updates

### Resolved issues
* rocJPEG Bindings - bugfixes
* Test package - find dependencies updated

## rocPyDecode 0.6.0 for ROCm 7.0.0

### Added

* rocpyjpegdecode package
* Added src/rocjpeg source new subfolder
* Added samples/rocjpeg new subfolder
* Added 'numpy' as pre-requisite needed for test and sample scripts

### Changed
* Minimum version for rocdecode and rocjpeg updated to V1.0.0

### Removed

### Optimized

### Resolved issues

## (Unreleased) rocPyDecode 0.4.0

### Added
* rocpydecode package 
* Moved rocPyDecode source files into new subfolder src/rocdecode
* Moved the dlpack and buffer CPP & H files into src/common new subfolder
* Created new samples/rocdecode sub-folder
* Moved rocPyDecode Python Samples under samples/rocdecode
* Moved rocPyDecode notebook under samples/rocdecode

### Changed

### Removed

MD5 functionality has been removed, and all APIs and samples related to MD5 functionality have also been removed.

### Optimized

### Resolved issues

## (Unreleased) rocPyDecode 0.3.1

### Added

### Changed

### Removed

All MD5 functionality, APIs, and sample code have been removed.

### Optimized

### Resolved issues

## (Unreleased) rocPyDecode 0.3.0

### Added

### Changed

* AMD Clang is now the default CXX and C compiler.
* MD5 code moved in rocDecode to a separate class provider under utilities. This move is transparent to rocPyDecode.

### Removed

### Optimized

### Resolved issues

## rocPyDecode 0.2.0 for ROCm 6.3

### Added

* RGB and YUV pytorch tensors
* Python distribution wheel (.whl)
* Multiple usecase samples

### Changed

* Clang is now the default CXX compiler.

### Removed

* Make tests have been removed. CTEST is now used for both Make tests and package tests.
* hipcc. Clang is now the default CXX compiler.

### Optimized

* Setup Script: Build and runtime install options
* Pre-requisite installation helper python scripts
* Same GPU memory viewed as pytorch tensor

### Resolved issues

* Setup
  * no dependency on hipcc
  * building rocPyDecode only once

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
