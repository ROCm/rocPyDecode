# rocDecode Python Binding

The rocDecode Python Binding is a tool that allows users to access rocDecode APIs in both Python and C/C++ languages. It works by connecting Python and C/C++ libraries, enabling the calling of functions and data passing between the two languages. The rocPyDecode.so library is a wrapper that facilitates the use of rocDecode APIs that are written primarily in C/C++ language within Python.

## Prerequisites
* [rocDecode C/C++ Library](https://github.com/ROCm/rocDecode)
* CMake Version `3.5` or higher
* Python Version `3`
* PIP3
* [CuPy for rocm](https://github.com/ROCmSoftwarePlatform/cupy)

## rocPyDecode install

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
