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
import traceback
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
        traceback.print_stack()
        exit(status)

# Arguments
parser = argparse.ArgumentParser()
parser.add_argument('--rocm_path', type=str, default='/opt/rocm',
                    help='ROCm Installation Path - optional (default:/opt/rocm) - ROCm Installation Required')
parser.add_argument('--rocdecode', type=str, default='ON',
                    help='rocDecode Installation - optional (default:ON) [options:ON/OFF]')
parser.add_argument('--docker', type=str, default='NO',
                    help='running on docker image - optional (default:NO) [options:NO/YES]')

args = parser.parse_args()

rocdecodeInstall = args.rocdecode.upper()
ROCM_PATH = args.rocm_path
docker_image = args.docker.upper()

if "ROCM_PATH" in os.environ:
    ROCM_PATH = os.environ.get('ROCM_PATH')
print("\nROCm PATH set to -- " + ROCM_PATH + "\n")

if rocdecodeInstall not in ('OFF', 'ON'):
    print(
        "ERROR: rocDecode Install Option Not Supported - [Supported Options: OFF or ON]\n")
    parser.print_help()
    exit()

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
sudoValidateOption= '-v'
if "redhat" in platfromInfo or os.path.exists('/usr/bin/yum'):
    linuxSystemInstall = 'yum -y'
    linuxSystemInstall_check = '--nogpgcheck'
    if "redhat-7" in platfromInfo:
        print("\nrocPyDecode Setup on "+platfromInfo+" is unsupported\n")
        exit(-1)
    if not "redhat" in platfromInfo:
        platfromInfo = platfromInfo+'-redhat'
elif "Ubuntu" in platfromInfo or os.path.exists('/usr/bin/apt-get'):
    linuxSystemInstall = 'apt-get -y'
    linuxSystemInstall_check = '--allow-unauthenticated'
    linuxFlag = '-S'
    if not "Ubuntu" in platfromInfo:
        platfromInfo = platfromInfo+'-Ubuntu'
else:
    print("\nrocPyDecode Setup on "+platfromInfo+" is unsupported\n")
    print("\nrocPyDecode Setup Supported on: Ubuntu 20/22; RedHat 8/9\n")
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
    'inxi',
    'python3',
    'python3-pip'
]

# Debian packages
coreDebianPackages = [
    'rocdecode',
    'rocdecode-dev',
    'rocdecode-test',
    'python3-dev'
]

# core RPM packages
coreRPMPackages = [
    'rocdecode',
    'rocdecode-devel',
    'rocdecode-test',
    'python3-devel'
]

# common packages
ERROR_CHECK(os.system('sudo -v'))
for i in range(len(commonPackages)):
    ERROR_CHECK(os.system('sudo '+linuxFlag+' '+linuxSystemInstall +
            ' '+linuxSystemInstall_check+' install '+ commonPackages[i]))

# rocPyDecode Requirements
ERROR_CHECK(os.system('sudo -v'))

#pybind11 install for both Ubuntu and RHEL
if(docker_image == 'YES'):
    ERROR_CHECK(os.system('pip3 install "pybind11[global]"'))
else:
    ERROR_CHECK(os.system('pip3 install pybind11'))

# if "Ubuntu" in platfromInfo:
# core debian packages
if rocdecodeInstall == 'ON':
    for i in range(len(coreDebianPackages)):
        ERROR_CHECK(os.system('sudo '+linuxFlag+' '+linuxSystemInstall +
                ' '+linuxSystemInstall_check+' install '+ coreDebianPackages[i]))
# dlpack
# TODO: make this dynamic allowing to change the ver# via parameter
ERROR_CHECK(os.system('wget http://archive.ubuntu.com/ubuntu/pool/universe/d/dlpack/libdlpack-dev_0.6-1_amd64.deb'))
# install needed z package
ERROR_CHECK(os.system(linuxSystemInstall+' install zstd'))
# Extract files from the archive
ERROR_CHECK(os.system('ar x libdlpack-dev_0.6-1_amd64.deb'))
# Uncompress zstd files an re-compress them using xz
ERROR_CHECK(os.system('zstd -d < control.tar.zst | xz > control.tar.xz'))
ERROR_CHECK(os.system('zstd -d < data.tar.zst | xz > data.tar.xz'))
# Re-create the Debian package in /tmp/
ERROR_CHECK(os.system('ar -m -c -a sdsd /tmp/libdlpack-dev_0.6-1_amd64.deb debian-binary control.tar.xz data.tar.xz'))
# Clean up
ERROR_CHECK(os.system('rm debian-binary control.tar.xz data.tar.xz control.tar.zst data.tar.zst'))
# install the deb now
ERROR_CHECK(os.system('dpkg -i /tmp/libdlpack-dev_0.6-1_amd64.deb'))

# elif "redhat" in platfromInfo:
#     # core RPM packages
#     if rocdecodeInstall == 'ON':
#         for i in range(len(coreRPMPackages)):
#             ERROR_CHECK(os.system('sudo '+linuxFlag+' '+linuxSystemInstall +
#                     ' '+linuxSystemInstall_check+' install '+ coreRPMPackages[i]))
#     # dlpack
#     # TODO

print("\rocPyDecode Dependencies Installed with rocPyDecode-setup.py V-"+__version__+"\n")
