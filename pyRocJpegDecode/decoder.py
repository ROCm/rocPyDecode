# Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

import rocpyjpegdecode as rocpyjpeg
import rocpyjpegdecode.jpegTypes as jpegt

def initialize_hip(device_id = 0, display_prop = True):
    hip = rocpyjpeg.PyRocJpegUtils()
    num_devices, ret = hip.init_hip_device(device_id, display_prop)
    return num_devices, ret

class decoder(object):
    def __init__(
            self, 
            device_id = 0, 
            backend = 0,
            output_format = jpegt.ROCJPEG_OUTPUT_RGB):
        self.device_id = device_id
        self.backend = backend
        self.output_format = output_format
        self.jpegdec = rocpyjpeg.Decoder(self.device_id, self.backend, self.output_format)

    # read image or batch of images
    def read(self, jpeg_item_to_decode):
        elapsed_time_in_msec, img = self.jpegdec.read(jpeg_item_to_decode) # elapsed_time_in_msec back in milliseconds
        return elapsed_time_in_msec, img

    # decode image or batch of images
    def decode(self, jpeg_item_to_decode):
        elapsed_time_in_msec, img = self.jpegdec.decode(jpeg_item_to_decode) # elapsed_time_in_msec back in milliseconds
        return elapsed_time_in_msec, img

    def set_output_image_format(self, output_format):
        self.output_format = jpegt.RocJpegOutputFormat(output_format)
        self.jpegdec.SetOutputFormat(self.output_format)
        return
