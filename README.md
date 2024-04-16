# rocDecode Python Binding

The rocDecode Python Binding, rocPyDecode, is a tool that allows users to access rocDecode APIs in both Python and C/C++ languages. It works by connecting Python and C/C++ libraries, enabling functions calling and data passing between the two languages. The rocpydecode.so library is a wrapper that facilitates the use of rocDecode APIs that are written primarily in C/C++ language within Python.

## Prerequisites

* [rocDecode C/C++ Library](https://github.com/ROCm/rocDecode)
* CMake Version `3.5` or higher
* Python Version `3`
* PIP3
* [DLPack](https://pypi.org/project/dlpack/)

## Prerequisites setup script

For your convenience, we provide the setup script, [rocPyDecode-requirements.py](rocPyDecode-requirements.py), which installs all required dependencies.Run this script only once.

```bash
python3 rocPyDecode-requirements.oy
```

## rocPyDecode install

* If using bare-metal, `sudo` access is needed.

```bash
git clone https://github.com/ROCm/rocPyDecode.git
cd rocPyDecode
sudo pip3 install .
```

* If using a docker environment or any system with `root` access. Do NOT use `sudo`.

```bash
git clone https://github.com/ROCm/rocPyDecode.git
cd rocPyDecode
pip3 install .
```

#### Prerequisites install to run test scripts

* Install PIP3
  + Ubuntu 20/22
    ```
    sudo apt install python3-pip
    ```
* Install [dlpack](https://pypi.org/project/dlpack/)
  + Ubuntu 20/22
    ```
    apt install dlpack 
    ```
#### Run Test Scripts
* Test scripts and instructions to run them can be found [here](samples/)
