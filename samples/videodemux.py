import pyRocVideoDecode.decoder as dec
import pyRocVideoDecode.demuxer as dmx
import datetime
import sys
import argparse
import os.path


def Demuxer(
        input_file_path,
        seek_pos
):
    # demuxer instance
    demuxer = dmx.demuxer(input_file_path)
    frames_counter = 0

    # Demuxing loop
    n_frame = 0
    total_dec_time = 0.0
    while True:
        start_time = datetime.datetime.now()
        frames_counter = frames_counter + 1
        packet = demuxer.DemuxFrame()
        if (packet.end_of_stream):
            break

    print("Total frames = ", frames_counter - 1)
    print("Demuxing has ended")


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description='PyRocDecode Video Decode Arguments')
    parser.add_argument(
        '-i',
        '--input',
        type=str,
        help='Input File Path - required',
        required=True)
    parser.add_argument(
        "-s",
        "--seek_pos",
        type=int,
        default=-
        1,
        help="Seek x positions in backward direction from the current position",
    )

    try:
        args = parser.parse_args()
    except BaseException:
        sys.exit()

    input_file_path = args.input
    seek_pos = args.seek_pos

    # Input file (must exist)
    if not os.path.exists(input_file_path):
        print("ERROR: input file doesn't exist.")
        exit()

    Demuxer(
        input_file_path,
        seek_pos)
