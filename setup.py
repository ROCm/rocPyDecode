# Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import subprocess
import os
from setuptools import setup, find_packages, Extension
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel
from setuptools.dist import Distribution

class BinaryDistribution(Distribution):
    """Distribution which always forces a binary package with platform name"""
    @classmethod
    def has_ext_modules(self):
        return True

def get_rev_based_on_os():
    os.system("cat /etc/os-release | grep ID= > os_release")
    if os.path.exists('os_release'):
        with open('os_release', 'r') as file:
            os_release_info = file.read()
        if "ubuntu" in os_release_info:
            os.system('apt show rocm-libs -a | grep Version > rev_file')
            return True
        elif "rhel" in os_release_info:
            os.system('yum info rocm-libs | grep Version > rev_file')
            return True
        elif "sles" in os_release_info:
            os.system('zypper info rocm-libs | grep Version > rev_file')
            return True
        elif "centos" in os_release_info:
            os.system('yum info rocm-libs | grep Version > rev_file')
            return True
    return False

def get_rocm_rev():
    try:
        if get_rev_based_on_os() == False:
            return "0" # "Unknown os"
        # "apt/yum/zepper" abs in file: rev_file
        if os.path.exists('rev_file'):
            with open("rev_file", 'r') as file:
                rev_data = file.read().split(".")
                return rev_data[3][:5]
    except Exception as e:
        print("An error occurred:", e)
    return "0" # error, can't get the rev

class custom_bdist_wheel(_bdist_wheel):
    def get_tag(self):
        python, abi, plat = super().get_tag()
        # Customize the platform tag, CI team requirements
        plat = 'manylinux_2_28_x86_64'
        return python, abi, plat

# Call CMake to configure and build the project
build_dir = os.path.join(os.getcwd(), 'build')
os.makedirs(build_dir, exist_ok=True)
cmake_args=["cmake", ".", "-B"+build_dir, "-H"+os.getcwd()]

subprocess.check_call(cmake_args,cwd=os.getcwd())

# Invoke cmake --build to build the project
subprocess.check_call(['cmake', '--build', build_dir, '--config', 'Release', '--parallel'])

# Install the built binaries
subprocess.check_call(['cmake', '--install', build_dir])

# Calculate Relative Path, to avoid error: arguments must *always* be /-separated paths relative to the setup.py directory
def get_relative_path(target_path, current_folder):
    relative_path = str(os.path.relpath(target_path, current_folder))
    return relative_path

# pickup cmake path location(s)
with open('export_path', 'r') as file:
    rocdecode_headers = file.readline().strip() # rocDecode H
    utils_folder = file.readline().strip() # UTIL
    decoder_class_folder = file.readline().strip() # Video Decode
    hip_headers = file.readline().strip() # HIP
    pybind11_headers = file.readline().strip() #  pybind11
    rocm_path = file.readline().strip() #  ROCM_PATH
# bring in reltaive path
current_folder = str(os.system('pwd'))
src_utils = get_relative_path(utils_folder, current_folder)
vdu_utils = get_relative_path(decoder_class_folder, current_folder)
# use compiler recognize the kernel code
os.environ["CC"] = rocm_path+'/bin/hipcc'
os.environ["CXX"] = rocm_path+'/bin/hipcc'

# Define the extension module
ext_modules = [Extension('rocPyDecode',
    sources=['src/roc_pydecode.cpp','src/roc_pybuffer.cpp','src/roc_pydlpack.cpp','src/roc_pyvideodecode.cpp','src/roc_pyvideodemuxer.cpp',src_utils+'/colorspace_kernels.cpp', src_utils+'/resize_kernels.cpp', vdu_utils+'/roc_video_dec.cpp'],
    include_dirs=[rocdecode_headers,utils_folder,decoder_class_folder,hip_headers,pybind11_headers],
    extra_compile_args=['-D__HIP_PLATFORM_AMD__','-Wno-sign-compare','-Wno-reorder','-Wno-int-in-bool-context', '-Wno-unused-variable','-Wno-missing-braces','-Wno-unused-private-field','-Wno-unused-function'],
    distclass=BinaryDistribution,
    library_dirs=[rocm_path+'/lib/'],
    libraries=['rocdecode','avcodec','avformat','avutil'])]

setup(
    name='rocPyDecode',
    description='AMD ROCm Video Decoder Library',
    url='https://github.com/ROCm/rocPyDecode',
    version='0.2.0' + '.' + get_rocm_rev(),
    author='AMD',
    license='MIT License',
    include_package_data=True,
    packages=['pyRocVideoDecode', 'pyRocVideoDecode/samples'],
    package_dir={'pyRocVideoDecode':'pyRocVideoDecode', 'pyRocVideoDecode/samples':'samples'},
    package_data={"pyRocVideoDecode":["__init__.pyi"], 'rocPyDecode': ['*.so']},  # Include .so files in the package
    cmdclass={'bdist_wheel': custom_bdist_wheel,},
    ext_modules=ext_modules,
    )

# Test built binaries -- TBD: Optional
# subprocess.check_call(['ctest', '--test-dir', build_dir, '-VV'])
