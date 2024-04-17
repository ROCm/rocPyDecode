# Copyright (c) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.
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

import os
import sys
import argparse
import platform
if sys.version_info[0] < 3:
    import commands
else:
    import subprocess

__copyright__ = "Copyright (c) 2024, AMD ROCm rocPyDecode"
__version__ = "1.0.0"
__email__ = "mivisionx.support@amd.com"
__status__ = "Shipping"

# error check calls
def ERROR_CHECK(call):
    status = call
    if(status != 0):
        print('ERROR_CHECK failed with status:'+str(status))
        exit(status)

# Arguments
parser = argparse.ArgumentParser()
parser.add_argument('--rocm_path', 	type=str, default='/opt/rocm',
                    help='ROCm Installation Path - optional (default:/opt/rocm) - ROCm Installation Required')

args = parser.parse_args()

ROCM_PATH = args.rocm_path

if "ROCM_PATH" in os.environ:
    ROCM_PATH = os.environ.get('ROCM_PATH')
print("\nROCm PATH set to -- " + ROCM_PATH + "\n")

# check ROCm installation
if os.path.exists(ROCM_PATH):
    print("\nROCm Installation Found -- " + ROCM_PATH + "\n")
    os.system('echo ROCm Info -- && ' + ROCM_PATH + '/bin/rocminfo')
else:
    print(
        "WARNING: If ROCm installed, set ROCm Path with \"--rocm_path\" option for full installation [Default:/opt/rocm]\n")
    print("ERROR: rocPyDecode Setup requires ROCm install\n")
    exit(-1)

# get platfrom info
platfromInfo = platform.platform()

# sudo requirement check
sudoLocation = ''
userName = ''
if sys.version_info[0] < 3:
    status, sudoLocation = commands.getstatusoutput("which sudo")
    if sudoLocation != '/usr/bin/sudo':
        status, userName = commands.getstatusoutput("whoami")
else:
    status, sudoLocation = subprocess.getstatusoutput("which sudo")
    if sudoLocation != '/usr/bin/sudo':
        status, userName = subprocess.getstatusoutput("whoami")

# setup for Linux
linuxSystemInstall = ''
linuxCMake = 'cmake'
linuxSystemInstall_check = ''
linuxFlag = ''
if "Ubuntu" in platfromInfo or os.path.exists('/usr/bin/apt-get'):
    linuxSystemInstall = 'apt-get -y'
    linuxSystemInstall_check = '--allow-unauthenticated'
    linuxFlag = '-S'
    if not "Ubuntu" in platfromInfo:
        platfromInfo = platfromInfo+'-Ubuntu'
else:
    print("\nrocPyDecode Setup on "+platfromInfo+" is unsupported\n")
    print("\nrocPyDecode Setup Supported on: Ubuntu 20/22\n")
    exit(-1)

# rocPyDecode Setup
print("\nrocPyDecode Setup on: "+platfromInfo+"\n")
print("\nrocPyDecode Dependencies Installation with rocPyDecode-setup.py V-"+__version__+"\n")

if userName == 'root':
    ERROR_CHECK(os.system(linuxSystemInstall+' update'))
    ERROR_CHECK(os.system(linuxSystemInstall+' install sudo'))

# source install - common package dependencies
commonPackages = [
    'gcc',
    'cmake',
    'git',
    'wget',
    'unzip',
    'pkg-config',
    'inxi'
]

# Debian packages
coreDebianPackages = [
    'python3-pip',
    'rocdecode',
    'rocdecode-dev',
    'rocdecode-test',
]

# FFMPEG packages
ffmpegDebianPackages = [
    'ffmpeg',
    'libavcodec-dev',
    'libavformat-dev',
    'libavutil-dev'
]

# common packages
ERROR_CHECK(os.system('sudo -v'))
for i in range(len(commonPackages)):
    ERROR_CHECK(os.system('sudo '+linuxFlag+' '+linuxSystemInstall +
            ' '+linuxSystemInstall_check+' install '+ commonPackages[i]))

# rocPyDecode Requirements
ERROR_CHECK(os.system('sudo -v'))
if "Ubuntu" in platfromInfo:
    #pybind11 install
    ERROR_CHECK(os.system('pip3 install pybind11'))    
    ERROR_CHECK(os.system('pip3 install dlpack'))    
    ERROR_CHECK(os.system('wget http://archive.ubuntu.com/ubuntu/pool/universe/p/pybind11/pybind11-dev_2.9.1-2_all.deb'))
    ERROR_CHECK(os.system('dpkg -i pybind11-dev_2.9.1-2_all.deb'))
    ERROR_CHECK(os.system('wget http://archive.ubuntu.com/ubuntu/pool/universe/d/dlpack/libdlpack-dev_0.6-1_amd64.deb'))
    ERROR_CHECK(os.system('dpkg -i libdlpack-dev_0.6-1_amd64.deb'))

    # core debian packages
    for i in range(len(coreDebianPackages)):
        ERROR_CHECK(os.system('sudo '+linuxFlag+' '+linuxSystemInstall +
                ' '+linuxSystemInstall_check+' install '+ coreDebianPackages[i]))

    # ffmpeg packages
    for i in range(len(ffmpegDebianPackages)):
        ERROR_CHECK(os.system('sudo '+linuxFlag+' '+linuxSystemInstall +
                ' '+linuxSystemInstall_check+' install '+ ffmpegDebianPackages[i]))


print("\rocPyDecode Dependencies Installed with rocPyDecode-setup.py V-"+__version__+"\n")
