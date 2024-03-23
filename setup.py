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
 
from setuptools import setup, Extension
from setuptools.command.install import install
import subprocess
import os

# Custom install command to run cmake before installation
class CustomInstall(install):
    def run(self):
        self.build_and_install()
        install.run(self)

    def build_and_install(self):
        # Set the build directory relative to the setup.py file
        build_temp = os.path.join(os.path.dirname(os.path.abspath(__file__)), "build")
        
        # Run cmake
        cmake_args = ["cmake", "."]
        subprocess.check_call(cmake_args + ["-B" + build_temp], cwd=os.getcwd())

        # Run cmake --build to compile
        subprocess.check_call(["cmake", "--build", build_temp, "--target", "install"], cwd=build_temp)

# Define the extension module
ext_modules = [
    Extension(
        'rocPyDecode', 
        sources=['src/roc_pydecode.cpp','src/roc_pyvideodecode.cpp','src/roc_pyvideodemuxer.cpp'], 
        include_dirs=['src','/opt/rocm/include/', '/opt/rocm/include/rocdecode/'], 
        extra_compile_args=['-D__HIP_PLATFORM_AMD__'], 
        library_dirs=['/opt/rocm/lib/', '/usr/local/lib/'],
        libraries=['rocdecode','avcodec','avformat','avutil'],
        runtime_library_dirs=[],
        extra_link_args=[],
    )
]

# Setup
setup(
      name='rocPyDecode',
      description='AMD ROCm Video Decoder Library',
      url='https://github.com/ROCm/rocPyDecode',
      version='1.0.0',
      author='AMD',
      license='MIT License',
      ext_modules=ext_modules,
      cmdclass={'install': CustomInstall},
      packages= ['rocCodec'],  
      package_dir={'rocCodec':'rocCodec'},
      package_data={"rocCodec": ["__init__.pyi"]},
      include_package_data=True,       
      )