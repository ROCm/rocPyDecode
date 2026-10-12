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

import pyRocVideoDecode.types as types

from inspect import getmembers, isfunction

print('rocPyDecode types')
rocpydecodeTypes = getmembers(types, isfunction)
for i in range(len(rocpydecodeTypes)):
    print(rocpydecodeTypes[i])

# Legacy exports must behave like the eagerly bound public API, even without PyAV.
import rocpydecode as native

compatibility_names = {"PyVideoDemuxer", "PyFileStreamProvider",
                       "AVCodec2RocDecVideoCodec", "AVCodecString2RocDecVideoCodec"}
assert compatibility_names <= set(dir(native))
members = dict(getmembers(native))
namespace = {}
exec("from rocpydecode import *", namespace)
public_names = {name for name in vars(native) if not name.startswith("_")}
assert public_names | compatibility_names <= namespace.keys()
for name in compatibility_names:
    assert namespace[name] is members[name] is getattr(native, name)
assert "PyRocVideoDecoderCpu" not in namespace
assert "PyRocVideoDecoderCpu" not in members
print("Compatibility exports are discoverable")
