# Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
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

import pyRocVideoDecode.demuxer as dmx
import pyRocVideoDecode.decoder as dec
from pyRocVideoDecode.types import OUT_SURFACE_MEM_DEV_COPIED
import ctypes
import argparse
import sys


def test_geometry(input_file_path, decoder_class, mem_type):
    demuxer = dmx.demuxer(input_file_path)
    codec = dec.GetRocDecCodecID(demuxer.GetCodecId())
    decoder = decoder_class(codec, mem_type=mem_type, b_force_zero_latency=True,
                            crop_rect=(0, 0, 64, 48))
    while True:
        packet = demuxer.DemuxFrame()
        if decoder.DecodeFrame(packet):
            break
        assert packet.bitstream_size > 0, "No frame for crop/resize regression"
    assert decoder.GetFrameYuv(packet, True) != -1
    assert packet.ext_buf[0].shape == (48, 64)
    assert (decoder.GetWidth(), decoder.GetHeight()) == (64, 48)
    expected_device = 1 if mem_type == 2 else 10  # DLPack CPU / ROCm
    assert packet.ext_buf[0].__dlpack_device__()[0] == expected_device
    surface = decoder.GetOutputSurfaceInfo()
    for width, height in ((32, 24), (24, 32), (128, 96)):
        resized = decoder.ResizeFrame(packet, (width, height), surface)
        assert resized
        # OutputSurfaceInfo begins with uint32 width, height and pitch.
        metadata = tuple((ctypes.c_uint32 * 3).from_address(resized))
        itemsize = 1 if packet.ext_buf[0].dtype == "|u1" else 2
        assert metadata == (width, height, width * itemsize), metadata
    for dims in ((-2, 48), (0, 48), (63, 47)):
        try:
            decoder.ResizeFrame(packet, dims, surface)
        except ValueError:
            pass
        else:
            raise AssertionError("Invalid subsampled YUV resize dimensions accepted")
    decoder.ReleaseFrame(packet)


def test_rgb_dlpack(input_file_path, decoder_class=dec.decoder, mem_type=OUT_SURFACE_MEM_DEV_COPIED):
    for fmt in range(1, 9):
        demuxer = dmx.demuxer(input_file_path)
        codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())
        decoder = decoder_class(codec_id, mem_type=mem_type, b_force_zero_latency=True)
        while True:
            packet = demuxer.DemuxFrame()
            if decoder.DecodeFrame(packet):
                assert decoder.GetFrameRgb(packet, fmt) != -1
                buf = packet.ext_buf[0]
                channels = 4 if fmt >= 5 else 3
                assert buf.dtype == ("|u2" if fmt % 2 == 0 else "|u1")
                assert buf.shape == (decoder.GetHeight(), decoder.GetWidth(), channels)
                assert buf.strides[1:] == (channels, 1)
                decoder.ReleaseFrame(packet)
                break
            if packet.bitstream_size <= 0:
                raise RuntimeError("no RGB frame decoded")
    test_geometry(input_file_path, decoder_class, mem_type)
    # A 50-row planar 4:2:0 crop has two 25-row chroma planes. Their
    # concatenated size is 75 luma-width rows, not 74 after per-plane division.
    demuxer = dmx.demuxer(input_file_path)
    decoder = decoder_class(dec.GetRocDecCodecID(demuxer.GetCodecId()),
                            mem_type=mem_type, b_force_zero_latency=True,
                            crop_rect=(0, 0, 64, 50))
    while True:
        packet = demuxer.DemuxFrame()
        if decoder.DecodeFrame(packet):
            break
        assert packet.bitstream_size > 0, "No frame for combined YUV regression"
    assert decoder.GetFrameYuv(packet, False) != -1
    assert packet.ext_buf[0].shape == (75, 64)
    decoder.ReleaseFrame(packet)
    print("rocPyDecode RGB DLPack formats 1–8 and crop/resize checks passed.")


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='PyRocDecode RGB DLPack Test Arguments')
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

    test_rgb_dlpack(input_file_path)
