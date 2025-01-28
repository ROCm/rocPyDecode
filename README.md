[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

# rocDecode Python Binding

> [!NOTE]
> The published documentation is available at [rocPyDecode](https://rocm.docs.amd.com/projects/rocPyDecode/en/latest/index.html) in an organized, easy-to-read format, with search and a table of contents. The documentation source files reside in the `docs` folder of this repository. As with all ROCm projects, the documentation is open source. For more information on contributing to the documentation, see [Contribute to ROCm documentation](https://rocm.docs.amd.com/en/latest/contribute/contributing.html).

The rocDecode Python Binding, rocPyDecode, is a tool that allows users to access rocDecode APIs in both Python and C/C++ languages. It works by connecting Python and C/C++ libraries, enabling function calling and data passing between the two languages. The `rocpydecode.so` library is a wrapper that facilitates the use of rocDecode APIs that are written primarily in C/C++ language within Python.

## Prerequisites

### Operating Systems
* Linux
  * Ubuntu - `22.04` / `24.04`

### Hardware
* **GPU**: [AMD Radeon&trade; Graphics](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/reference/system-requirements.html) / [AMD Instinct&trade; Accelerators](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/reference/system-requirements.html)

> [!IMPORTANT] 
> `gfx908` or higher GPU required

* Install ROCm `6.3.0` or later with [amdgpu-install](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/how-to/amdgpu-install.html): **Required** usecase:`rocm`
> [!IMPORTANT]
> `sudo amdgpu-install --usecase=rocm`

### Compiler
* AMD Clang++ Version 18.0.0 or later - installed with ROCm

### Libraries
* CMake `3.12` or higher

  ```shell
  sudo apt install cmake
  ```

* [rocDecode](https://github.com/ROCm/rocDecode)

  ```shell
  sudo apt install rocdecode-dev
  ```

* [DLPack](https://pypi.org/project/dlpack/)
  
  ```shell
  sudo apt install libdlpack-dev
  ```

* Python3 and Python3 PIP

  ```shell
  sudo apt install python3-dev python3-pip
  ```

* [PyBind11](https://github.com/pybind/pybind11)

  ```shell
  sudo pip3 install pybind11
  ```

* [pkg-config](https://en.wikipedia.org/wiki/Pkg-config)

  ```shell
  sudo apt install pkg-config
  ```

* [FFmpeg](https://ffmpeg.org/about.html) runtime and headers - for tests and samples

  ```shell
  sudo apt install libavcodec-dev libavformat-dev libavutil-dev
  ```

> [!IMPORTANT]
> * Required compiler support
>   * C++17
>   * Threads

>[!NOTE]
> * All package installs are shown with the `apt` package manager. Use the appropriate package manager for your operating system.

## Prerequisites setup script

For your convenience, we provide the setup script, [rocPyDecode-requirements.py](rocPyDecode-requirements.py), which installs all required dependencies.\
Run this script only once on bare metal, if using docker please see below instructions.

```shell
python3 rocPyDecode-requirements.py
```

## rocPyDecode install

### using bare-metal
```shell
git clone https://github.com/ROCm/rocPyDecode.git
cd rocPyDecode
sudo pip3 install .
```
>[!NOTE]
> `sudo` access is needed

### creating python distribution wheel
```shell
# the generated .whl file will be located under subfolder ./dist/
git clone https://github.com/ROCm/rocPyDecode.git
cd rocPyDecode

# Create/Build the wheel and install it
sudo python3 build_rocpydecode_wheel.py

# alternative method
sudo python3 setup.py bdist_wheel
```
>[!NOTE]
> `sudo` access is needed

### using docker environment

```shell
git clone https://github.com/ROCm/rocPyDecode.git
cd rocPyDecode
python rocPyDecode-docker-install.py 
```
>[!NOTE]
> Do NOT use `sudo`

### creating rocPyDecode conda package
* Information on how to create and install rocPyDecode conda package is located [here](https://github.com/ROCm/rocPyDecode/blob/develop/conda-recipe/README.md).

## Run CTest

This will run python samples and show pass/fail.

### Dependencies:
```shell
python3 -m pip install --upgrade pip
python3 -m pip install -i https://test.pypi.org/simple hip-python
```

### Run test:
```shell
cd rocPyDecode
cmake .
ctest -VV
```

## Run Sample Scripts

* Sample scripts and instructions to run them can be found [here](samples/)

## Documentation

Run the following code to build our documentation locally.

```shell
cd docs
pip3 install -r sphinx/requirements.txt
python3 -m sphinx -T -E -b html -d _build/doctrees -D language=en . _build/html
```

For more information on documentation builds, refer to the
[Building documentation](https://rocm.docs.amd.com/en/latest/contribute/building.html)
page.

## Tested configurations

* Linux distribution
  * Ubuntu - `22.04` / `24.04`
* ROCm: rocm-core - `6.3.0.60300`
* CMake - Version `3.12`+
* AMD Clang++ - Version `18.0.0`+
