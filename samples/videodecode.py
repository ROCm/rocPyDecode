import pyRocVideoDecode.decoder as dec
import pyRocVideoDecode.demuxer as dmx
import datetime
import sys
import argparse
import os.path


def Decoder(
        input_file_path,
        output_file_path,
        device_id,
        mem_type,
        b_force_zero_latency,
        crop_rect,
        b_generate_md5,
        ref_md5_file,
        seek_frame,
        seek_mode,
        seek_criteria):

    # demuxer instance
    demuxer = dmx.demuxer(input_file_path)

    # get the used coded id
    codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())

    # decoder instance
    viddec = dec.decoder(
        device_id,
        mem_type,
        codec_id,
        b_force_zero_latency,
        p_crop_rect,
        0,
        0,
        1000)

    # Get GPU device information
    cfg = viddec.GetGpuInfo()

    #  print some GPU info out
    print("\ninfo: Input file: " +
          input_file_path +
          '\n' +
          "info: Using GPU device " +
          str(device_id) +
          " - " +
          cfg.device_name +
          "[" +
          cfg.gcn_arch_name +
          "] on PCI bus " +
          str(cfg.pci_bus_id) +
          ":" +
          str(cfg.pci_domain_id) +
          "." +
          str(cfg.pci_device_id))
    print("info: decoding started, please wait! \n")

    # md5 file full path & md5 flag
    b_md5_check = False
    if (ref_md5_file is not None):
        if os.path.exists(ref_md5_file):
            b_generate_md5 = True
            b_md5_check = True

    # init MD5 if requested
    if b_generate_md5:
        viddec.InitMd5()

    # -----------------
    # The decoding loop
    # -----------------
    n_frame = 0
    total_dec_time = 0.0
    not_seeking = True if seek_frame is None else False

    while True:
        start_time = datetime.datetime.now()

        if(not_seeking):
            packet = demuxer.DemuxFrame()
        else:
            packet = demuxer.SeekFrame(seek_frame, seek_mode, seek_criteria)
            not_seeking = True

        n_frame_returned = viddec.DecodeFrame(packet)
        for i in range(n_frame_returned):
            viddec.GetFrame(packet)
            if (b_generate_md5):
                surface_info = viddec.GetOutputSurfaceInfo()
                viddec.UpdateMd5ForFrame(packet.frame_adrs, surface_info)
            if (output_file_path is not None):
                surface_info = viddec.GetOutputSurfaceInfo()
                viddec.SaveFrameToFile(
                    output_file_path, packet.frame_adrs, surface_info)
            # release frame
            viddec.ReleaseFrame(packet)

        # measure after completing a whole frame
        end_time = datetime.datetime.now()
        time_per_frame = end_time - start_time
        total_dec_time = total_dec_time + time_per_frame.total_seconds()

        # increament frames counter
        n_frame += n_frame_returned

        if (packet.frame_size <= 0):
            break

    # beyond the decoding loop
    n_frame += viddec.GetNumOfFlushedFrames()

    print("info: Total frame decoded: " + str(n_frame))

    if (output_file_path is None):
        if (n_frame > 0 and total_dec_time > 0):
            time_per_frame = (total_dec_time / n_frame) * 1000
            frame_per_second = n_frame / total_dec_time
            print("info: avg decoding time per frame: " +
                  "{0:0.2f}".format(round(time_per_frame, 2)) + " ms")
            print("info: avg frame per second: " + "{0:0.2f}".format(round(frame_per_second, 2)) + "\n")
        else:
            print("info: frame count= ", n_frame)

    # if MD5 check requested
    if b_generate_md5:
        digest = viddec.FinalizeMd5()
        print("MD5 message digest: ", end=" ")
        str_digest = ""
        for i in range(16):
            str_digest = str_digest + str(format('%02x' % int(digest[i])))
        print(str_digest)

        if (b_md5_check):
            f = open(ref_md5_file)
            md5_from_file = f.read(16 * 2)
            b_match = (md5_from_file == str_digest)
            if (b_match):
                print("MD5 digest matches the reference MD5 digest.\n")
            else:
                print(
                    "MD5 digest does not match the reference MD5 digest: ",
                    md5_from_file)


if __name__ == "__main__":

    # get passed arguments
    parser = argparse.ArgumentParser(
        description='PyRocDecode Video Decode Arguments')
    parser.add_argument(
        '-i',
        '--input',
        type=str,
        help='Input File Path - required',
        required=True)
    parser.add_argument(
        '-o',
        '--output',
        type=str,
        help='Output File Path - optional',
        required=False)
    parser.add_argument(
        '-d',
        '--device',
        type=int,
        default=0,
        help='GPU device ID - optional, default 0',
        required=False)
    parser.add_argument(
        '-m',
        '--mem_type',
        type=int,
        default=1,
        help='mem_type of output surfce - 0: Internal 1: dev_copied 2: host_copied optional, default 1',
        required=False)
    parser.add_argument(
        '-z',
        '--zero_latency',
        type=str,
        default='no',
        choices=['yes', 'no'],
        help='Force zero latency',
        required=False)
    parser.add_argument(
        '-crop',
        '--crop_rect',
        nargs=4,
        type=int,
        help='Crop rectangle (left, top, right, bottom), optional, default: no cropping',
        required=False)
    parser.add_argument(
        '-md5',
        '--generate_md5',
        type=str,
        default='no',
        choices=['yes', 'no'],
        help='Generate MD5 message digest')
    parser.add_argument(
        '-md5_check',
        '--input_md5',
        type=str,
        help='Input MD5 file path, optional',
        required=False)
    parser.add_argument(
        '-s',
        '--seek',
        type=int,
        default=1,
        help='seek this number of frames, optional, default: no seek',
        required=False)
    parser.add_argument(
        '-sm',
        '--seek_mode',
        type=int,
        default=1,
        help='seek mode, 0 - by exact frame number, 1 - by previous key frame, optional, default: 1 - by previous key frame',
        required=False)
    parser.add_argument(
        '-sc',
        '--seek_criteria',
        type=int,
        default=0,
        help='seek criteria, 0 - by frame number, 1 - by time stamp, optional, default: 0 - by frame number',
        required=False)


    try:
        args = parser.parse_args()
    except BaseException:
        sys.exit()

    input_file_path = args.input
    output_file_path = args.output
    device_id = args.device
    mem_type = args.mem_type
    b_force_zero_latency = args.zero_latency
    crop_rect = args.crop_rect
    b_generate_md5 = args.generate_md5
    ref_md5_file = args.input_md5
    seek_frame = args.seek
    seek_mode = args.seek_mode
    seek_criteria = args.seek_criteria

    b_force_zero_latency = True if b_force_zero_latency == 'yes' else False
    b_generate_md5 = True if b_generate_md5 == 'yes' else False

    # rect from user
    p_crop_rect = dec.GetRectangle(crop_rect)

    # Input file (must exist)
    if not os.path.exists(input_file_path):
        print("ERROR: input file doesn't exist.")
        exit()

    Decoder(
        input_file_path,
        output_file_path,
        device_id,
        mem_type,
        b_force_zero_latency,
        crop_rect,
        b_generate_md5,
        ref_md5_file,
        seek_frame,
        seek_mode,
        seek_criteria)
