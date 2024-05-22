[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

# rocDecode Python Binding

The rocDecode Python Binding, rocPyDecode, is a tool that allows users to access rocDecode APIs in both Python and C/C++ languages. It works by connecting Python and C/C++ libraries, enabling function calling and data passing between the two languages. The rocpydecode.so library is a wrapper that facilitates the use of rocDecode APIs that are written primarily in C/C++ language within Python.

## Prerequisites

* Linux distribution
  * Ubuntu - `20.04` / `22.04`
  * RHEL - `8` / `9`

* [ROCm-supported hardware](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/reference/system-requirements.html)
> [!IMPORTANT] 
> `gfx908` or higher GPU required

* Install ROCm `6.2.0` or later with [amdgpu-install](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/how-to/amdgpu-install.html): Required usecase - rocm
> [!IMPORTANT]
> `sudo amdgpu-install --usecase=rocm`

* [rocDecode](https://github.com/ROCm/rocDecode)
  
  ```shell
  sudo apt install rocdecode-dev
  ```

* CMake `3.5` or higher
  
  ```shell
  sudo apt install cmake
  ```

* Python3 and Python3 PIP
  
  ```shell
  sudo apt install python3-dev python3-pip
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

## Run Sample Scripts

* Sample scripts and instructions to run them can be found [here](samples/)
