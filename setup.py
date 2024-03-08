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

from setuptools import setup, find_packages, Extension
from setuptools.dist import Distribution
import sys
import os

# ROCM_PATH = '/opt/rocm'
# if "ROCM_PATH" in os.environ:
#     ROCM_PATH = os.environ.get('ROCM_PATH')
# print("\nROCm PATH set to -- "+ROCM_PATH+"\n")

if sys.version_info < (3, 0):
    sys.exit('rocpydecode Python Package requires Python > 3.0')

class BinaryDistribution(Distribution):
    """Distribution which always forces a binary package with platform name"""
    @classmethod
    def has_ext_modules(self):
        return True
 
setup(
      name='amd-rocpydecode',
      description='AMD ROCm Video Decoder Library',
      url='https://github.com/ROCm/rocPyDecode',
      version='1.0.0',
      author='AMD',
      license='MIT License',
      packages= ['amd/rocpydecode'],  
      package_dir={'amd':'@TARGET_NAME@/amd'},
      package_data={"amd": ["__init__.pyi"]},
      include_package_data=True,
      ext_modules=[Extension('rocpydecode', 
                            sources=['src/roc_pydecode.cpp','src/roc_pyvideodecode.cpp','src/roc_pyvideodemuxer.cpp'], 
                            include_dirs=['/opt/rocm/include/', '@pybind11_INCLUDE_DIRS@', '../rocDecode/api','/opt/rocm/include'], 
                            extra_compile_args=['-D__HIP_PLATFORM_AMD__'], 
                            library_dirs=['/opt/rocm/lib/', '/usr/local/lib/'],
                            libraries=['rocdecode','avcodec','avformat','avfilter','avformat','avutil']
                             )],
      distclass=BinaryDistribution
      )

# 
 