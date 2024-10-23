# rocPyDecode conda package

## Prerequisites
* conda or mini-conda installed

If you do not have conda or mini-conda installed you can use following steps to install it:

```bash
    # if inside docker you might not have wget
    sudo apt install wget -y 
    # download latest minimal conda
    wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
    # to automate use '-b'
    bash Miniconda3-latest-Linux-x86_64.sh -b
    source ~/miniconda3/etc/profile.d/conda.sh
    # to speed up installations
    conda config --add channels conda-forge
    conda config --set channel_priority strict
    # update to ensure latest
    conda update conda -y
    # finally the build pkg
    conda install conda-build -y
```
## Building conda package

Change directory to rocPyDecode subfolder 'conda-recipe', then execute the conda build command:

```bash
    conda build .   # assumes the meta.yaml file on same folder
```
## Location of rocPyDecode conda package

After successful build from the previous step the generated conda package will be on the following path(es):

```bash
    /root/miniconda3/pkgs/rocpydecode-0.2.0-py312_0.tar.bz2
    /root/miniconda3/conda-bld/linux-64/rocpydecode-0.2.0-py313_0.tar.bz2
 ```

## Installing rocPyDecode conda package

To install the generated package use the following command:

```bash
    conda install --use-local rocpydecode
```

## Installing rocPyDecode package on conda virtual environment

To install rocPyDecode conda package on virtual environment use the following commands:
```bash
    conda create -n test_env python=3.10  # Specify the Python version if necessary
    conda activate test_env
    # when inside the env
    conda install --use-local rocpydecode

    # when not in use to deactivate the env
    deactivate
```





