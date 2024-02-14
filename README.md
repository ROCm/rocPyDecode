# rocDecode Python Binding

rocDecode Python Binding allows you to call functions and pass data from Python to rocDecode C/C++ libraries,
letting you take advantage of the rocDecode functionality in both languages.

rocPyDecode.so is a wrapper library that bridges python and C/C++, so that a rocDecode functionality
written primarily in C/C++ language can be used effectively in Python.

## Prerequisites
* [rocDecode C/C++ Library](https://github.com/ROCm/rocDecode)
* CMake Version `3.5` or higher
* Python Version `3`
* PIP3
* [CuPy for rocm](https://github.com/ROCmSoftwarePlatform/cupy)

## rocPyDecode install

rocPyDecode installs during rocDecode build with below command
```
sudo cmake --build . --target PyPackageInstall
```

#### Prerequisites install to run test scripts

* Install PIP3
  + Ubuntu 20/22
    ```
    sudo apt install python3-pip
    ```
* Install `CuPy` for `ROCm` - `https://github.com/ROCmSoftwarePlatform/cupy`

#### Run Test Scripts
* Test scripts and instructions to run them can be found [here](examples/)
