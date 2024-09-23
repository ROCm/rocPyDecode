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
import shutil
import glob
from setuptools import setup, find_packages
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel

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

def copy_roc_decode():
    # default directory for ROCm shared libraries
    lib_dir = '/opt/rocm/lib'
    # find all versions of librocdecode
    lib_files = glob.glob(os.path.join(lib_dir, 'librocdecode.so*'))
    if lib_files:
        # Prepare the destination directory within the build folder
        dest_dir = os.path.join(os.getcwd(), 'pyRocVideoDecode', 'libs')
        os.makedirs(dest_dir, exist_ok=True)
        # copy first found library and strip version info in name
        for lib_file in lib_files:
            dest_lib_name = 'librocdecode.so'
            shutil.copy(lib_file, os.path.join(dest_dir, dest_lib_name))
            print(f"Copied {lib_file} to {os.path.join(dest_dir, dest_lib_name)}")
            break  # Copy the first found version and exit loop
    else:
        print(f"No rocDecode shared library found in {lib_dir}. Skipping copy.")

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

# Copy rocDecode shared library if it exists
copy_roc_decode()

setup(
      name='rocPyDecode',
      description='AMD ROCm Video Decoder Library',
      url='https://github.com/ROCm/rocPyDecode',
      version='1.0.0' + '.' + get_rocm_rev(),
      author='AMD',
      license='MIT License',
      packages=['pyRocVideoDecode'],
      package_dir={'pyRocVideoDecode':'pyRocVideoDecode'},
      package_data={"pyRocVideoDecode":["__init__.pyi", "libs/*.so"]},
      cmdclass={'bdist_wheel': custom_bdist_wheel,},
      )

# Test built binaries -- TBD: Optional
# subprocess.check_call(['ctest', '--test-dir', build_dir, '-VV'])
