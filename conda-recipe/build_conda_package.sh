#!/bin/bash

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

# assumes the meta.yaml file on same folder, build the package
conda build .



