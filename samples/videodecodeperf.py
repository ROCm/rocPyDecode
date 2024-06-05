import pyRocVideoDecode.decoder as dec
import pyRocVideoDecode.demuxer as dmx
import datetime
import sys
import argparse
import os.path
import multiprocessing
from multiprocessing import Process, Value
from array import array


def DecProc(device_id, input_file_path, p_frames, p_fps):

    # demuxer instance
    demuxer = dmx.demuxer(input_file_path)
    # get the used coded id
    codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())

    # decoder instance
    viddec = dec.decoder(
        device_id=device_id,
        mem_type=1,
        codec=codec_id,
        b_force_zero_latency=False,
        crop_rect=None,
        max_width=0,
        max_height=0,
        clk_rate=1000)

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

    n_frame = 0
    total_dec_time = 0.0

    while True:
        start_time = datetime.datetime.now()
        packet = demuxer.DemuxFrame()

        n_frame_returned = viddec.DecodeFrame(packet)

        # measure after completing a whole frame
        end_time = datetime.datetime.now()
        time_per_frame = end_time - start_time
        total_dec_time = total_dec_time + time_per_frame.total_seconds()

        # increament frames counter
        n_frame += n_frame_returned

        if (packet.frame_size <= 0):  # EOF: no more to decode
            break

    # beyond the decoding loop
    n_frame += viddec.GetNumOfFlushedFrames()

    p_frames.value = n_frame

    if (n_frame > 0 and total_dec_time > 0):
        time_per_frame = (total_dec_time / n_frame) * 1000
        frame_per_second = n_frame / total_dec_time
        p_fps.value = round(frame_per_second,2)

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
        '-d',
        '--device',
        type=int,
        default=0,
        help='GPU device ID - optional, default 0',
        required=False)
    parser.add_argument(
        '-t',
        '--num_process',
        type=int,
        default=4,
        help='Num of parallel runs - optional, default 4',
        required=False)
    try:
        args = parser.parse_args()
    except BaseException:
        sys.exit()

    # get params
    input_file_path = args.input
    device_id = args.device
    num_process = args.num_process

    # handle params
    if not os.path.exists(input_file_path):  # Input file (must exist)
        print("ERROR: input file doesn't exist.")
        exit()

    total_frames = 0
    total_fps = 0.0
    try:
        multiprocessing.set_start_method('spawn')
    except RuntimeError:
        print('ERROR: Could not create processes')
        exit()
    processes = []
    p_frames = Value('i', 0)
    p_fps = Value('f', 0.0)

    for i in range(0, num_process):
        p = Process(target=DecProc, args=(device_id, input_file_path, p_frames, p_fps))
        p.start()
        processes.append(p)

    for p in processes:
        total_frames += p_frames.value
        total_fps += p_fps.value
        p.join()

    print("info: Total frame decoded: " + str(total_frames))
    print("info: avg frame per second: " + str(total_fps))