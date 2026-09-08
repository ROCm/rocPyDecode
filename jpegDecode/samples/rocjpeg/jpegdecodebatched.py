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

import pyRocJpegDecode.decoder as jdec
import rocpyjpegdecode.jpegTypes as jpegt
import argparse
import os
import sys


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Example of decoding whole folder and sub-folders containing jpeg images
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
def jpeg_decode_batch(
        input_file_path,
        batch_size,
        output_format,
        device_id,
        backend):

    # initialize hip
    print("")
    devices_count, ret = jdec.initialize_hip(device_id)
    if(ret == False):
        print(f"Exiting jpegdecodebatched application, Device#: {device_id} not found.")
        sys.exit()

    # create the decoder instance
    decoder = jdec.decoder(device_id, backend)
    decoder.set_output_image_format(output_format)  # set the output image to the desired format

    print(f"Decoding whole folder of images: {input_file_path} on Device: {'CPU' if backend else 'GPU'} with Device ID: {device_id}")

    files_full_path_list = [os.path.join(root, f) for root, _, files in os.walk(input_file_path) for f in files]
    total = len(files_full_path_list)
    total_decode_time_in_milli_sec = 0.0
    total_valid_images_processed = 0

    print(f"Decoding Starting for {total} files with batch size = {batch_size}.")
    for i in range(0, total, batch_size):
        current_batch = files_full_path_list[i:i + batch_size]
        batch_time_msec, img_list = decoder.decode(current_batch)
        total_decode_time_in_milli_sec += batch_time_msec
        total_valid_images_processed += len(img_list)

    print(f"Total files processed : {total_valid_images_processed}")
    print(f"Total Bad files found : {total-total_valid_images_processed}")
    if (total_valid_images_processed > 0):
        avg_time_per_image = total_decode_time_in_milli_sec / float(total)
        ips = 1000.0 / avg_time_per_image
        print("info: Average processing time per image (ms):      " + str(round(avg_time_per_image, 3)))
        print("info: Average decoded images per sec (Images/Sec): " + str(round(ips, 3)) + "\n")


if __name__ == "__main__":

    # get passed arguments
    parser = argparse.ArgumentParser(
        description='Batch decode example Arguments')
    parser.add_argument(
        '-i',
        '--input',
        type=str,
        help='Input Files FULL Path - required',
        required=True)
    parser.add_argument(
        '-b',
        '--batch',
        type=int,
        default=2,
        help='batch size > 0 process the batch of files with this batch size, if 0 means do not process as batch, optional, default is 2',
        required=False)
    parser.add_argument(
        '-fmt',
        '--output_format',
        type=int,
        choices=[3, 4],
        default=3,
        help='Set output image format: 3 for ROCJPEG_OUTPUT_RGB (interleaved), 4 for ROCJPEG_OUTPUT_RGB_PLANAR. Optional, default is 3.',
        required=False)
    parser.add_argument(
        '-bk',
        '--backend',
        type=int,
        choices=[0, 1],
        default=0,
        help='Set backend choice 0:GPU and 1:CPU, Optional, default is 0',
        required=False)    
    parser.add_argument(
        '-d',
        '--device',
        type=int,
        default=0,
        help='GPU device ID - optional, default 0',
        required=False)

    try:
        args = parser.parse_args()
    except BaseException:
        sys.exit()

    # get params
    input_file_path = args.input
    batch_size = args.batch
    output_format = args.output_format
    device_id = args.device
    backend = args.backend

    if not isinstance(batch_size, int) or batch_size <= 0:
        print(f"Args Error: batch_size must be a positive integer, got {batch_size}\n")
        exit()
    if not os.path.isdir(input_file_path):
        print(f"Args Error: '{input_file_path}' is not a directory.\n")
        exit()
    if not os.path.exists(input_file_path):  # Input file or folder (must exist)
        print("ERROR: input folder doesn't exist.")
        exit()

    jpeg_decode_batch(input_file_path, batch_size, output_format, device_id, backend)
