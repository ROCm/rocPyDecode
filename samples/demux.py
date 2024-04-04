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
    bUseSeek = False
    if (seek_pos > -1):
        bUseSeek = True
        print("Seeking to timestamp = ", seek_pos)

    # demuxer instance
    demuxer = dmx.demuxer(input_file_path)
    cnt = 0

    # Demuxing loop
    n_frame = 0
    total_dec_time = 0.0
    while True:
        start_time = datetime.datetime.now()

        if bUseSeek:
            if cnt > seek_pos:
                # demuxer.Seek(seek_pos) # should be uncommented when Seek is
                # implemented
                bUseSeek = False
        cnt = cnt + 1

        packet = demuxer.DemuxFrame()

        if (packet.end_of_stream):
            break

    print("Total frames = ", cnt - 1)
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
    if (os.path.exists(input_file_path) == False):
        print("ERROR: input file doesn't exist.")
        exit()

    Demuxer(
        input_file_path,
        seek_pos)
