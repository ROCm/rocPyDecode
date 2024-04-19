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
 
from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension
from setuptools.command.install import install
import subprocess
import os
import site
from glob import glob

ROCM_PATH = '/opt/rocm'
if "ROCM_PATH" in os.environ:
    ROCM_PATH = os.environ.get('ROCM_PATH')
print("\nROCm PATH set to -- " + ROCM_PATH + "\n")

# This block is NOT HARDCODED, it is relative to the environment os.environ.get('ROCM_PATH') if found
UTILS_PATH = ROCM_PATH + '/share/rocdecode/utils/'
UTILS_DEC_PATH = ROCM_PATH + '/share/rocdecode/utils/rocvideodecode/'
ROC_DEC_PATH = ROCM_PATH + '/include/rocdecode/'
ROCM_H_PATH = ROCM_PATH + '/include/'
os.environ["CC"] = ROCM_PATH + '/llvm/bin/clang++'
 
# Custom install to run cmake before installation
class CustomInstall(install):
    def run(self):
        self.build_and_install()
        install.run(self)

    def build_and_install(self):
        # Set the build directory relative to the setup.py file
        build_temp=os.path.join(os.path.dirname(os.path.abspath(__file__)),"build")

        #path to python pacakges like pybind11
        python_site_package_path = site.getsitepackages()
        python_site_package_path = ';'.join(python_site_package_path)

        # Run cmake
        cmake_args=["cmake", ".", "-B"+build_temp, "-Dpybind11_DIR="+python_site_package_path]
        subprocess.check_call(cmake_args,cwd=os.getcwd())

        # Run cmake --build to compile
        subprocess.check_call(["cmake","--build",build_temp,"--target","install"],cwd=build_temp)

# Define the extension module
ext_modules = [
    Pybind11Extension(
        'rocPyDecode', 
        sources=sorted(glob("src/*.cpp"))+[UTILS_DEC_PATH+"roc_video_dec.cpp"],
        include_dirs=[ROCM_H_PATH, ROC_DEC_PATH, UTILS_PATH, UTILS_DEC_PATH, 'src' ],
        extra_compile_args=['-D__HIP_PLATFORM_AMD__','-Wno-unused-private-field','-Wno-ignored-optimization-argument', '-Wno-missing-braces', '-Wno-sign-compare', '-Wno-sign-compare','-Wno-reorder','-Wno-int-in-bool-context', '-Wno-unused-variable'],
        library_dirs=[ROCM_PATH+'/lib/','/usr/local/lib/','/usr/lib/x86_64-linux-gnu/'],
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
      setup_requires=["pybind11"],
      ext_modules=ext_modules,
      cmdclass={'install':CustomInstall},
      packages=['pyRocVideoDecode'],
      package_dir={'pyRocVideoDecode':'pyRocVideoDecode'},
      package_data={"pyRocVideoDecode":["__init__.pyi"]},
      )
