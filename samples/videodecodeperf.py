import pyRocVideoDecode.decoder as dec
import pyRocVideoDecode.demuxer as dmx
import datetime
import sys
import argparse
from threading import Thread
from hip import hip
import os

def HipCheck(call_result):
    err = call_result[0]
    result = call_result[1:]
    if len(result) == 1:
        result = result[0]
    if isinstance(err, hip.hipError_t) and err != hip.hipError_t.hipSuccess:
        raise RuntimeError(str(err))
    return result

def DecProc(viddec, demuxer, frame_ret, fps_ret):
    n_frame = 0
    total_dec_time = 0.0

    start_time = datetime.datetime.now()
    while True:
        packet = demuxer.DemuxFrame()
        if(packet.end_of_stream):
            break
        n_frame_returned = viddec.DecodeFrame(packet)
        n_frame += 1
        if (packet.end_of_stream): # no more to decode?
            break

    # measure decoding time
    end_time = datetime.datetime.now()
    time_per_frame = end_time - start_time
    total_dec_time = time_per_frame.total_seconds()
    fps_ret.append(n_frame / total_dec_time)
    frame_ret.append(n_frame)

if __name__ =="__main__":
    # get passed arguments
    parser = argparse.ArgumentParser(description='PyRocDecode Video Decode Arguments')
    parser.add_argument('-i', '--input', type=str, help='Input File Path - required', required=True)
    parser.add_argument('-t', '--thread', type=int, choices=range(1, 65), default=4, help='Number of threads - optional', required=False)
    parser.add_argument('-d', '--device', type=int, default=0, help='GPU device ID - optional, default 0', required=False)

    try:
        args = parser.parse_args()
    except:
        sys.exit()

    input_file_path = args.input
    num_threads = args.thread
    device_id = args.device
    sd = 0

    # Input file (must exist)
    if (os.path.exists(input_file_path) == False):
        print("ERROR: input file doesn't exist.")
        exit()

    # HIP Python calls to find number of VCNs per device
    props = hip.hipDeviceProp_t()
    HipCheck(hip.hipGetDeviceProperties(props, device_id))
    gcn_arch_name = props.gcnArchName.decode('UTF-8')
    gcn_arch_name = gcn_arch_name.split(':', 1)[0]
    num_devices = HipCheck(hip.hipGetDeviceCount())
    if (num_devices < 1):
        print("ERROR: no GPUs found")
        sys.exit()
    if (gcn_arch_name == 'gfx90a' and num_devices > 1):
        sd = 1

    print("info: number of threads: ", num_threads)
    v_demuxer = []
    v_viddec = []
    v_device_id  = []
    for i in range(num_threads):
        # use correct device
        if (device_id % 2 == 0):
            if (i % 2 == 0):
                v_device_id.append(device_id)
            else:
                v_device_id.append(device_id + sd)
        else:
            if (i % 2 == 0):
                v_device_id.append(device_id - sd)
            else:
                v_device_id.append(device_id)

        # demuxer instance
        v_demuxer.append(dmx.demuxer(input_file_path))
        # get the used coded id
        coded_id = dec.GetRocDecCodecID(v_demuxer[i].GetCodecId())
        # decoder instance
        v_viddec.append(dec.decoder(v_device_id[i], coded_id, False, None))
        # Get GPU device information
        cfg = v_viddec[i].GetGpuInfo()
        #  print some GPU info out
        print("\ninfo: Input file: " + input_file_path + '\n' +"info: Using GPU device " + str(v_device_id[i]) + " - " + cfg.device_name + "[" + cfg.gcn_arch_name + "] on PCI bus " + str(cfg.pci_bus_id) + ":" + str(cfg.pci_domain_id) + "." + str(cfg.pci_device_id))
        print("info: decoding started, please wait!")

    # empty lists for each thread and it's corresponding frames & fps
    thread_list = []
    n_frame = []
    n_fps = []
    for i in range(num_threads):
        curr_thread = Thread(target=DecProc, args=(v_viddec[i], v_demuxer[i], n_frame, n_fps))
        thread_list.append(curr_thread)

    for t in thread_list:
        t.start()

    for t in thread_list:
        t.join()

    total_frame = 0
    total_fps = 0.0
    for i in range(num_threads):
        total_frame += n_frame[i]
        total_fps += n_fps[i]

    print("info: Total frame decoded: " + str(total_frame))
    print("info: avg decoding time per frame: " + "{0:0.2f}".format(round(1000 / total_fps, 2)) + " ms")
    print("info: avg FPS: " + "{0:0.2f}".format(round(total_fps, 2)) + "\n")
