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
from setuptools import setup, find_packages
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel

def detect_os_command():
    os_release_path = "/etc/os-release"
    if os.path.exists(os_release_path):
        with open(os_release_path, 'r') as file:
            os_release_info = file.read()
        if "ID=ubuntu" in os_release_info:
            return ['apt', 'show', 'rocm-libs', '-a']
        elif "ID=rhel" in os_release_info or "Red Hat Enterprise Linux" in os_release_info:
            return ['dnf', 'info', 'rocm-libs']
        elif "ID=sles" in os_release_info or "SUSE Linux Enterprise Server" in os_release_info:
            return ['zypper', 'info', 'rocm-libs']
    return [''] # "Unknown"

def get_rocm_rev():
    try:
        apt_command = detect_os_command()
        if apt_command == ['']:
            return "00000" # "Unknown os"
        # Execute the "apt show rocm-libs -a" command and capture the output
        result = subprocess.run(apt_command, capture_output=True, text=True, check=True)
        # Split the output into lines and find the line containing the Version/Revision
        lines = result.stdout.split('\n')
        for line in lines:
            # print(line)
            if 'Version:' in line and not line.startswith('#'):
                parts = line.split()
                if parts[0] == 'Version:':
                    words = parts[1].split('.')
                    revision = words[3].split('-')
                    return revision[0]
    except subprocess.CalledProcessError as e:
        print("Failed to obtain rocm revision:", e)
    except Exception as e:
        print("An error occurred:", e)
    return "00000" # error, can't get the rev

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
subprocess.check_call(['cmake', '--build', build_dir, '--config', 'Release'])

# Install the built binaries
subprocess.check_call(['cmake', '--install', build_dir])

setup(
      name='rocPyDecode',
      description='AMD ROCm Video Decoder Library',
      url='https://github.com/ROCm/rocPyDecode',
      version='1.0.0' + '.' + get_rocm_rev(),
      author='AMD',
      license='MIT License',
      packages=['pyRocVideoDecode'],
      package_dir={'pyRocVideoDecode':'pyRocVideoDecode'},
      package_data={"pyRocVideoDecode":["__init__.pyi"]},
      cmdclass={'bdist_wheel': custom_bdist_wheel,},
      )

# Test built binaries -- TBD: Optional
# subprocess.check_call(['ctest', '--test-dir', build_dir, '-VV'])
