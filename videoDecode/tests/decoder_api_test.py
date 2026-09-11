# Copyright (c) 2018 - 2023 Advanced Micro Devices, Inc. All rights reserved.
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

import rocpydecode
from rocpydecode import DLPackPyTensor
import argparse
import sys
 
parser = argparse.ArgumentParser(description='PyRocDecode Video Decode Input Arguments')
parser.add_argument(
    '-i',
    '--input',
    type=str,
    help='Input File Path - required',
    required=True)
try:
    args = parser.parse_args()
except SystemExit as e:
    print(f"Error: {e}. Please check the input arguments and try again.")
    sys.exit(1)

input_file_path = args.input

if hasattr(rocpydecode, "TestAllClassCalls"):
    rocpydecode.TestAllClassCalls(input_file_path)
    rocpydecode.TestAll_roc_pybuffer()
    rocpydecode.Test_DLPack()
    rocpydecode.Test_PyReconfigureFlushCallback()
    rocpydecode.Test_CalculateRgbImageSize()
    print('rocPyDecode APIs test finished.\n')

    DLPackPyTensor.test_all()
    print('rocPyDecode DLPackPyTensor APIs test completed successfully.\n')
else:
    print("rocPyDecode host/CPU test APIs not built; skipping decoder_api_test.")
