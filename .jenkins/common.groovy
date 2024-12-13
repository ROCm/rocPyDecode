// This file is for internal AMD use.
// If you are interested in running your own Jenkins, please raise a github issue for assistance.

def runCompileCommand(platform, project, jobName, boolean debug=false, boolean staticLibrary=false) {
    project.paths.construct_build_prefix()

    String buildTypeArg = debug ? '-DCMAKE_BUILD_TYPE=Debug' : '-DCMAKE_BUILD_TYPE=Release'
    String buildTypeDir = debug ? 'debug' : 'release'
    
    def command = """#!/usr/bin/env bash
                set -ex
                
                echo Build rocDecode
                pwd
                cd ${project.paths.project_build_prefix}/..
                pwd
                rm -rf rocDecode
                git clone http://github.com/ROCm/rocDecode.git
                cd rocDecode
                sudo python3 rocDecode-setup.py
                mkdir build
                cd build
                sudo cmake ..
                sudo make -j
                sudo make install
                cd ../..

                echo Build rocPyDecode - ${buildTypeDir}
                pwd
                cd rocpydecode
                pwd
                wget https://github.com/dmlc/dlpack/archive/refs/tags/v0.6.tar.gz
                tar -xvf v0.6.tar.gz
                cd dlpack-0.6
                mkdir build
                cd build
                sudo cmake ..
                sudo make
                sudo make install
                cd ../..

                sudo pip3 install pybind11[global]

                sudo mkdir -p /opt/rocm/share/rocdecode/utils

                sudo python3 rocPyDecode-docker-install.py
                """

    platform.runCommand(this, command)
}

def runTestCommand (platform, project) {

    String libLocation = ''

    if (platform.jenkinsLabel.contains('rhel')) {
        libLocation = ':/usr/local/lib'
    }
    else if (platform.jenkinsLabel.contains('sles')) {
        libLocation = ':/usr/local/lib'
    }

    def command = """#!/usr/bin/env bash
                set -ex
                export HOME=/home/jenkins
                export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib64/:/usr/local/lib/x86_64-linux-gnu:\$LD_LIBRARY_PATH
                echo make samples
                sudo pip3 install --pre torch torchvision torchaudio --index-url https://download.pytorch.org/whl/nightly/rocm6.2
                cd ${project.paths.project_build_prefix}
                echo \$PYTHONPATH
                LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:/opt/rocm/lib${libLocation} sudo python3 samples/videodecode.py
                """

    platform.runCommand(this, command)
}

def runPackageCommand(platform, project) {

    def packageHelper = platform.makePackage(platform.jenkinsLabel, "${project.paths.project_build_prefix}/build/release")

    String packageType = ''
    String packageInfo = ''
    String packageDetail = ''
    String osType = ''
    String packageRunTime = ''
    
    def command = """#!/usr/bin/env bash
                set -ex
                export HOME=/home/jenkins
                echo rocPyDecode Package
                """

    platform.runCommand(this, command)
}

return this
