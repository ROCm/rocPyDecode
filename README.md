[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

# rocDecode Python Binding

The rocDecode Python Binding, rocPyDecode, is a tool that allows users to access rocDecode APIs in both Python and C/C++ languages. It works by connecting Python and C/C++ libraries, enabling function calling and data passing between the two languages. The rocpydecode.so library is a wrapper that facilitates the use of rocDecode APIs that are written primarily in C/C++ language within Python.

## Prerequisites

* Linux distribution
  * Ubuntu - `20.04` / `22.04`

* [ROCm-supported hardware](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/reference/system-requirements.html)
> [!IMPORTANT] 
> `gfx908` or higher GPU required

* Install ROCm `6.2.0` or later with [amdgpu-install](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/how-to/amdgpu-install.html): Required usecase - rocm
> [!IMPORTANT]
> `sudo amdgpu-install --usecase=rocm`

* CMake `3.5` or higher
  
  ```shell
  sudo apt install cmake
  ```

* Python3 and Python3 PIP
  
  ```shell
  sudo apt install python3-dev python3-pip
  ```

* [PyBind11](https://github.com/pybind/pybind11)

  ```shell
  pip3 install pybind11
  ```

* [rocDecode](https://github.com/ROCm/rocDecode)
  
  ```shell
  sudo apt install rocdecode-dev
  ```

* [pkg-config](https://en.wikipedia.org/wiki/Pkg-config)

  ```shell
  sudo apt install pkg-config
  ```

* [FFmpeg](https://ffmpeg.org/about.html) runtime and headers - for tests and samples

  ```shell
  sudo apt install ffmpeg libavcodec-dev libavformat-dev libavutil-dev
  ```

* [DLPack](https://pypi.org/project/dlpack/)
  
  ```shell
    sudo pip3 install dlpack
    wget http://archive.ubuntu.com/ubuntu/pool/universe/d/dlpack/libdlpack-dev_0.6-1_amd64.deb
    sudo dpkg -i libdlpack-dev_0.6-1_amd64.deb    
    ```

>[!NOTE]
> * All package installs are shown with the `apt` package manager. Use the appropriate package manager for your operating system.

## Prerequisites setup script

For your convenience, we provide the setup script, [rocPyDecode-requirements.py](rocPyDecode-requirements.py), which installs all required dependencies.\
Run this script only once on bare metal, if using docker please see below instructions.

```bash
python3 rocPyDecode-requirements.py
```

## rocPyDecode install

* If using bare-metal, `sudo` access is needed.

```bash
git clone https://github.com/ROCm/rocPyDecode.git
cd rocPyDecode
sudo pip3 install .
```

* If using a docker environment or any system with `root` access. Do **NOT** use `sudo`.

```bash
git clone https://github.com/ROCm/rocPyDecode.git
cd rocPyDecode
python rocPyDecode-docker-install.py 
```

> [!IMPORTANT] 
> `RHEL`/`SLES` package install requires manual `FFMPEG` dev install

## Run CTest

* This will run the simple videodecode sample and show pass/fail.

```
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

* Linux
  * Ubuntu - `20.04` / `22.04`
* ROCm:
  * rocm-core - `6.2.0.60200-crdnnh.14042`
  * amdgpu-core - `1:6.2.60200-1778439.22.04`
* rocdecode - `0.6.0.60200-crdnnh.14042`
* FFmpeg - `4.2.7` / `4.4.2-0`
