import pyRocVideoDecode.decoder as dec
import pyRocVideoDecode.demuxer as dmx
import datetime
import sys
import argparse

# get passed arguments  
parser = argparse.ArgumentParser(description='PyRocDecode Video Decode Arguments')
parser.add_argument('-i', '--input', type=str, help='Input File Path - required', required=True)
parser.add_argument('-o', '--output', type=str, help='Output File Path - optional', required=False)
parser.add_argument('-d', '--device', type=int, default=0, help='GPU device ID - optional, default 0', required=False)
parser.add_argument('-z', '--zero_latency', type=str, default=False, help='Force zero latency - [options: yes,no], default: no', required=False)
parser.add_argument('-crop', '--crop_rect', nargs=4, type=int, default=None, help='Crop rectangle (left, top, right, bottom), optional, default: no cropping', required=False)

try:
    args = parser.parse_args()
except:
    sys.exit()

input_file_path = args.input
output_file_path = args.output
device_id = args.device
b_force_zero_latency = args.zero_latency
crop_rect = args.crop_rect
 
# rect from user
p_crop_rect = dec.GetRectangle(crop_rect)

# demuxer instance
demuxer = dmx.demuxer(input_file_path)

# get the used coded id
coded_id = dec.GetRocDecCodecID(demuxer.GetCodecId())
 
# decoder instance
viddec = dec.decoder(device_id, coded_id, b_force_zero_latency, p_crop_rect, 0, 0, 0)

# Get GPU device information
cfg = viddec.GetGpuInfo()

#  print some GPU info out
print("\ninfo: Input file: " + input_file_path + '\n' +"info: Using GPU device " + str(device_id) + " - " + cfg.device_name + "[" + cfg.gcn_arch_name + "] on PCI bus " + str(cfg.pci_bus_id) + ":" + str(cfg.pci_domain_id) + "." + str(cfg.pci_device_id))
print("info: decoding started, please wait! \n")
  
# -----------------
# The decoding loop 
# -----------------
n_frame = 0
total_dec_time = 0.0

while True:
    start_time = datetime.datetime.now()

    packet = demuxer.DemuxFrame()

    if(packet.end_of_stream):
        break

    n_frame_returned = viddec.DecodeFrame(packet)

    for i in range(n_frame_returned):
        viddec.GetFrame(packet)

        if (output_file_path != None):
            surface_info = viddec.GetOutputSurfaceInfo() 
            viddec.SaveFrameToFile(output_file_path, packet.frame_adrs, surface_info)

        # release frame
        viddec.ReleaseFrame(packet, False)

    # measure after completing a whole frame
    end_time = datetime.datetime.now()
    time_per_frame = end_time - start_time
    total_dec_time += time_per_frame.total_seconds()

    # increament frames counter
    n_frame += 1

    if (packet.end_of_stream): # no more to decode?
        break

# beyond the decoding loop
n_frame += viddec.GetNumOfFlushedFrames()

print("info: Total frame decoded: " + str(n_frame))

if (output_file_path == None):
    if(n_frame > 0 and total_dec_time > 0):
        TPF = (total_dec_time / n_frame) * 1000
        FPS = n_frame / total_dec_time
        print("info: avg decoding time per frame: " + "{0:0.2f}".format(round(TPF, 2)) + " ms")
        print("info: avg FPS: " + "{0:0.2f}".format(round(FPS, 2)))
    else:
        print("info: frame count= ", n_frame)
