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
from pybind11.setup_helpers import Pybind11Extension
from distutils.sysconfig import get_python_lib
from setuptools.command.install import install
import pybind11
import subprocess
import os

ROCM_PATH = '/opt/rocm'
if "ROCM_PATH" in os.environ:
    ROCM_PATH = os.environ.get('ROCM_PATH')
print("\nROCm PATH set to -- "+ROCM_PATH+"\n")

UTILS_PATH=ROCM_PATH+'/share/rocdecode/utils/'
UTILS_DEC_PATH=ROCM_PATH+'/share/rocdecode/utils/rocvideodecode/'
ROC_DEC_PATH=ROCM_PATH+'/include/rocdecode/'
ROCM_H_PATH=ROCM_PATH+'/include/'
gpu_list=['--offload-arch=gfx908','--offload-arch=gfx90a','--offload-arch=gfx940','--offload-arch=gfx941','--offload-arch=gfx942','--offload-arch=gfx1030','--offload-arch=gfx1031','--offload-arch=gfx1032','--offload-arch=gfx1100','--offload-arch=gfx1101','--offload-arch=gfx1102']
os.environ["CC"] = ROCM_PATH+'/llvm/bin/clang++ -x hip'
os.environ["CXX"] = ROCM_PATH+'/llvm/bin/clang++'
 
# Custom install to run cmake before installation
class CustomInstall(install):
    def run(self):
        self.build_and_install()
        install.run(self)

    def build_and_install(self):
        # Set the build directory relative to the setup.py file
        build_temp=os.path.join(os.path.dirname(os.path.abspath(__file__)),"build")

        # Run cmake
        cmake_args=["cmake", ".", "-B"+build_temp, "-Dpybind11_DIR="+get_python_lib()]
        subprocess.check_call(cmake_args,cwd=os.getcwd())

        # Run cmake --build to compile
        subprocess.check_call(["cmake","--build",build_temp,"--target","install"],cwd=build_temp)

# Define the extension module
ext_modules = [
    Extension(
        'rocPyDecode', 
        sources=[UTILS_PATH+'colorspace_kernels.cpp', UTILS_DEC_PATH+'roc_video_dec.cpp','src/roc_pyresizeframe.cpp','src/roc_pycolorconversion.cpp','src/roc_pydecode.cpp','src/roc_pyvideodecode.cpp','src/roc_pyvideodemuxer.cpp','src/roc_pybuffer.cpp','src/roc_pydlpack.cpp'], 
        include_dirs=['build/pybind11/include/','build/dlpack/include/', ROCM_H_PATH, ROC_DEC_PATH, UTILS_PATH, UTILS_DEC_PATH, 'src' ],
        extra_compile_args=gpu_list+['-D__HIP_PLATFORM_AMD__','-Wno-unused-private-field','-Wno-ignored-optimization-argument', '-Wno-missing-braces', '-Wno-sign-compare', '-Wno-sign-compare','-Wno-reorder','-Wno-int-in-bool-context', '-Wno-unused-variable'],
        library_dirs=[ROCM_PATH+'/lib/','/usr/local/lib/','/usr/lib/x86_64-linux-gnu/'],
        libraries=['rocdecode','avcodec','avformat','avutil','amdhip64'],
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
