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

# Call CMake to configure and build the project
build_dir = os.path.join(os.getcwd(), 'build')
os.makedirs(build_dir, exist_ok=True)
cmake_args=["cmake", ".", "-B"+build_dir, "-H"+os.getcwd()]
subprocess.check_call(cmake_args,cwd=os.getcwd())

# Invoke cmake --build to build the project
subprocess.check_call(['cmake', '--build', build_dir, '--config', 'Release'])

# Install the built binaries
subprocess.check_call(['cmake', '--install', build_dir])

# Test built binaries
subprocess.check_call(['ctest', '--test-dir', build_dir, '-VV'])

setup(
      name='rocPyDecode',
      description='AMD ROCm Video Decoder Library',
      url='https://github.com/ROCm/rocPyDecode',
      version='1.0.0',
      author='AMD',
      license='MIT License',
      packages=['pyRocVideoDecode'],
      package_dir={'pyRocVideoDecode':'pyRocVideoDecode'},
      package_data={"pyRocVideoDecode":["__init__.pyi"]},
      )