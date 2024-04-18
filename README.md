[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

# rocDecode Python Binding

The rocDecode Python Binding, rocPyDecode, is a tool that allows users to access rocDecode APIs in both Python and C/C++ languages. It works by connecting Python and C/C++ libraries, enabling functions calling and data passing between the two languages. The rocpydecode.so library is a wrapper that facilitates the use of rocDecode APIs that are written primarily in C/C++ language within Python.

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

* [rocDecode C/C++ Library](https://github.com/ROCm/rocDecode)
* CMake `3.5` or higher
  * Ubuntu 20/22

    ```bash
    sudo apt install cmake
    ```

  * RHEL 8/9
    ```bash
    sudo yum install cmake
    ```

* Python `3`
  * Ubuntu 20/22

    ```bash
    sudo apt install python3
    ```

  * RHEL 8/9

    ```bash
    sudo yum install python3
    ```

* PIP3
  * Ubuntu 20/22

    ```bash
    sudo apt install python3-pip
    ```
  * RHEL 8/9

    ```bash
    sudo yum install python3-pip
    ```

* [DLPack](https://pypi.org/project/dlpack/)
  * Ubuntu 20/22

    ```bash
    sudo pip3 install dlpack
    wget http://archive.ubuntu.com/ubuntu/pool/universe/d/dlpack/libdlpack-dev_0.6-1_amd64.deb
    sudo dpkg -i libdlpack-dev_0.6-1_amd64.deb    
    ```

## Prerequisites setup script

For your convenience, we provide the setup script, [rocPyDecode-requirements.py](rocPyDecode-requirements.py), which installs all required dependencies.Run this script only once.

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
pip3 install .
```

## Run Sample Scripts

* Sample scripts and instructions to run them can be found [here](samples/)
