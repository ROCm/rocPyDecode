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
__version__ = "0.3.0"
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

args = parser.parse_args()

rocdecodeInstall = args.rocdecode.upper()
ROCM_PATH = args.rocm_path

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
    'python3-dev',
    'libdlpack-dev'
]

# core RPM packages
# TODO: dlpack/ pybind11-devel package

coreRPMPackages = [
    'rocdecode',
    'rocdecode-devel',
    'rocdecode-test',
    'python3-devel'
]

# update
ERROR_CHECK(os.system('sudo '+linuxFlag+' '+linuxSystemInstall +' '+linuxSystemInstall_check+' update'))

# common packages
ERROR_CHECK(os.system('sudo -v'))
for i in range(len(commonPackages)):
    ERROR_CHECK(os.system('sudo '+linuxFlag+' '+linuxSystemInstall +
            ' '+linuxSystemInstall_check+' install '+ commonPackages[i]))

# rocPyDecode Requirements
ERROR_CHECK(os.system('sudo -v'))

if "Ubuntu" in platfromInfo:
    # core debian packages
    if rocdecodeInstall == 'ON':
        for i in range(len(coreDebianPackages)):
            ERROR_CHECK(os.system('sudo '+linuxFlag+' '+linuxSystemInstall +
                    ' '+linuxSystemInstall_check+' install '+ coreDebianPackages[i]))

elif "redhat" in platfromInfo:
    # core RPM packages
    if rocdecodeInstall == 'ON':
        for i in range(len(coreRPMPackages)):
            ERROR_CHECK(os.system('sudo '+linuxFlag+' '+linuxSystemInstall +
                    ' '+linuxSystemInstall_check+' install '+ coreRPMPackages[i]))

# make sure we have pybind11 installed via pip
ERROR_CHECK(os.system('sudo pip3 install pybind11'))
GREEN = "\033[32m"
RESET = "\033[0m"
print(f"{GREEN}pybind11 {RESET}successfully installed.\n")

# done
BOLD = '\033[1m'
print(f"{GREEN}{BOLD}rocPyDecode Dependencies Installed {RESET}with rocPyDecode-setup.py V-"+__version__+"\n")
