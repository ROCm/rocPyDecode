import subprocess
import os
import shutil

# Clean any existing build/dist files
if os.path.exists('build'):
    shutil.rmtree('build')
if os.path.exists('dist'):
    shutil.rmtree('dist')

# Build the source distribution and wheel
subprocess.run(['python3', 'setup.py', 'bdist_wheel', 'sdist', 'install'], check=True)

# Verify the content of the created wheel
wheel_dir = os.path.join('dist')
wheel_files = os.listdir(wheel_dir)

print(f"Wheel files created: {wheel_files}")