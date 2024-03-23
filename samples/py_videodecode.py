import rocCodec.decoder as dec
import rocCodec.demuxer as dex
import numpy as np
import datetime
import sys
import argparse
import os.path

# init 
b_dump_output_frames = False     
b_force_zero_latency = False

# get passed arguments  
parser = argparse.ArgumentParser(description='PyRocDecode Video Decode Arguments')
parser.add_argument('-i', '--input', type=str, help='Input File Path - required', required=True)
parser.add_argument('-o', '--output', type=str, help='Output File Path - optional', required=False)
parser.add_argument('-d', '--device', type=int, default=0, help='GPU device ID - optional, default 0', required=False)
parser.add_argument('-z', '--zero_latency', type=str, help='Force zero latency - [optios: yes,no], default: no', required=False)
parser.add_argument('-crop', '--crop_rect', nargs=4, type=int, help='Crop rectangle (left, top, right, bottom), optional, default: no cropping', required=False)

try:
    args = parser.parse_args()  
except:
    print("ERROR: Incorrect arguments were passed.\n")
    sys.exit()    

input_file_path = args.input
output_file_path = args.output
device_id = args.device
force_zero_latency = args.zero_latency
crop_rect = args.crop_rect
 
# rect from user
p_crop_rect = dec.GetRectangle(crop_rect)
 
# Input file name (must exist)
if (os.path.exists(input_file_path) == False):
    print("ERROR: input file doesn't exist.")
    exit()

# Output file name (optional flag to dump out)  
if (output_file_path!=None):
    b_dump_output_frames = True

# force 0 latency
if force_zero_latency =='yes':
    b_force_zero_latency = True

# instantiate demuxer instance 
demuxer = dex.demuxer(input_file_path)

# get the used coded id
coded_id = dec.GetRocDecCodecID( demuxer.GetCodec_ID() )
 
# instantiate decoder instance 
viddec = dec.decoder(device_id, coded_id, b_force_zero_latency, p_crop_rect, 0, 0, 0)

# Get GPU device information
cfg = viddec.Get_GPU_Info()

#  print some info out  
#d = np.string_(cfg.device_name).decode("utf-8")      
print("\ninfo: Input file: " + input_file_path + '\n' +"info: Using GPU device " + str(device_id) + " - " + cfg.device_name + "[" + cfg.gcn_arch_name + "] on PCI bus " + str(cfg.pci_bus_id) + ":" + str(cfg.pci_domain_id) + "." + str(cfg.pci_device_id) )
print("info: decoding started, please wait! \n")
  
# -------------------------------------
# the decoding loop 
# -------------------------------------                         
n_frame = int(0)
total_dec_time = float(0.0)
surface_info_struct = dec.GetOutputSurfaceInfo() 
print_surface_info = True

# Do until no more to decode
while True:           
    start_time = datetime.datetime.now()
    
    packet = demuxer.DemuxFrame()
    
    if(packet.end_of_stream): # packet.frame_size<=0):
        break

    n_frame_returned = viddec.DecodeFrame(packet)
    
    # print ONE time only
    if print_surface_info:
        [b_ret_info, surface_info_adrs] = viddec.GetOutputSurfaceInfoAdrs(surface_info_struct) 
        print_surface_info = False
        if (n_frame==0 and b_ret_info==False):
            print("Error: Failed to get Output Surface Info!\n")
        else:
            print ("Surface Info:\n--------------")
            print ( "output_width: \t\t",    surface_info_struct.output_width)
            print ( "output_height: \t\t",   surface_info_struct.output_height)
            print ( "output_pitch: \t\t",    surface_info_struct.output_pitch)
            print ( "vertical stride: \t",   surface_info_struct.output_vstride)	   
            print ( "bytes_per_pixel: \t",   surface_info_struct.bytes_per_pixel)
            print ( "bit_depth: \t\t",       surface_info_struct.bit_depth)	               
            print ( "num_chroma_planes: \t", surface_info_struct.num_chroma_planes)	
            print ( "out surface bytes: \t", surface_info_struct.output_surface_size_in_bytes)
            print ( "surface_format: \t",    surface_info_struct.surface_format)			
            print ("\n")			

    for i in range(n_frame_returned): 
        viddec.GetFrame(packet)        
        
        if b_dump_output_frames: 
            viddec.SaveFrameToFile( output_file_path, packet.frame_adrs, surface_info_adrs )
            
        # release frame        
        viddec.ReleaseFrame(packet, False)
        
    # measure after completing a whole frame
    end_time = datetime.datetime.now()
    time_per_frame = end_time - start_time
    total_dec_time = total_dec_time + time_per_frame.total_seconds()

    # increament frames counter
    n_frame += 1

    if (packet.end_of_stream): # no more to decode?
        break

# beyond the decoding LOOP
n_frame += viddec.GetNumOfFlushedFrames()

print("info: Total frame decoded: " + str(n_frame))

if b_dump_output_frames==False:
    if(n_frame>0 and total_dec_time>0):
        print("info: avg decoding time per frame: " + str((total_dec_time / n_frame)*1000) + " ms")
        print("info: avg FPS: " + str(n_frame / total_dec_time) + "\n")
    else:
        print( "info: frame count= ", n_frame )

print("\n") # end