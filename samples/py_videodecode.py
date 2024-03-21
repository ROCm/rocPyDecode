import amd.rocdecode.decoder as dec
import amd.rocdecode.demuxer as dex
import numpy as np
import datetime
import sys
import argparse
import os.path

# init for decoding
b_dump_output_frames = False     # set if "Output File Path" is passed as arg

# init Params (set based on passed args)
b_force_zero_latency = False
p_crop_rect = None

# get passed arguments for: input_file_path, output_file_path, gpu_device_id, force_zero_latency_flag, crop_rectangle_4_values
parser = argparse.ArgumentParser(description='PyRocDecode Video Decode Arguments')
parser.add_argument('-i', '--input', type=str, help='Input File Path - required')
parser.add_argument('-o', '--output', type=str, help='Output File Path - optional')
parser.add_argument('-d', '--device', type=int, default=0, help='GPU device ID - optional, default 0')
parser.add_argument('-z', '--zero_latency', type=str, help='Force zero latency - [optios: yes,no], default: no')
parser.add_argument('-crop', '--crop_rect', nargs='+', type=int, help='Crop rectangle (left, top, right, bottom), optional, default: no cropping')

try:
    args = parser.parse_args()  
except:
    help_ = False
    if(len(sys.argv[0])>=2):
        if(sys.argv[1]=="--help") or (sys.argv[1]=="-h"):
            help_ = True    
    if(help_==False):
        print("ERROR: Incorrect arguments were passed.\n")
    print("\n")
    sys.exit()    

input_file_path = args.input
output_file_path = args.output
device_id = args.device
force_zero_latency = args.zero_latency
crop_rect = args.crop_rect
 
# do we have rect from user
if (args.crop_rect != None):
    if (args.crop_rect != [0,0,0,0]):
        p_crop_rect = dec.GetRectangle()
        p_crop_rect.left = crop_rect[0]
        p_crop_rect.top = crop_rect[1]
        p_crop_rect.right = crop_rect[2]
        p_crop_rect.bottom = crop_rect[3]
    
# Input file name (must)
valid_input = False
if (input_file_path!=None):
    if os.path.exists(input_file_path):
        valid_input= True
if (valid_input==False):
    print("ERROR: Must provide valid input file full path name")
    exit()

# Output file name (optional, & flag to dump frames out)  
if (output_file_path!=None ):
    b_dump_output_frames = True

# force 0 latency
if (force_zero_latency in ('yes','no')):
    if force_zero_latency =='yes':
        b_force_zero_latency = True

# valid to pass
if (output_file_path==None):
    output_file_path = np.array("")
else:
    output_file_path = np.array(output_file_path)

# instantiate demuxer instance 
demuxer = dex.demuxer(input_file_path)

# get the used coded id
avCodecID = demuxer.GetCodec_ID()
coded_id = dec.GetRocDecCodecID( avCodecID )
 
# instantiate decoder instance 
viddec = dec.decoder(device_id, coded_id, b_force_zero_latency, p_crop_rect, 0, 0, 0)

# Get GPU device information
[device_name,gcn_arch_name,pci_bus_id,pci_domain_id,pci_device_id] = viddec.Get_GPU_Info()

#  print some info out  
d = np.string_(device_name).decode("utf-8")      
g = np.string_(gcn_arch_name).decode("utf-8")
print("\ninfo: Input file: " + input_file_path + '\n' +
      "info: Using GPU device " + 
      str(device_id) + " - " + d + "[" + g + "] on PCI bus " + str(pci_bus_id) + ":" + 
      str(pci_domain_id) + "." + str(pci_device_id) )
print("info: decoding started, please wait! \n")
  
# -------------------------------------
# prepare params for the decoding loop 
# -------------------------------------
 
pkg_flags = int(0)                              
n_frame = int(0)
total_dec_time = float(0.0)
surface_info_struct = dec.GetOutputSurfaceInfo() 
print_surface_info = True

# Do until no more to decode
while True:           
    start_time = datetime.datetime.now()
    
    [b_ret, frame_adrs, frame_size, frame_pts] = demuxer.DemuxFrame()

    # Treat False ret as end of stream indicator
    if (b_ret == False):
        pkg_flags = dec.EndOfStream(pkg_flags)

    n_frame_returned = viddec.DecodeFrame(frame_adrs, frame_size, pkg_flags, frame_pts)

    # OutputSurfaceInfo **surface_info_adrs  
    [b_ret_info, surface_info_adrs] = viddec.GetOutputSurfaceInfoAdrs(surface_info_struct) 

    # print ONE time only
    if print_surface_info:
        print_surface_info = False
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

    if (n_frame==0 and b_ret_info==False):
        print("Error: Failed to get Output Surface Info!\n")
        break
                
    for i in range(n_frame_returned): 
        viddec.GetFrameAddress(frame_pts, frame_adrs)        

        if b_dump_output_frames: 
            viddec.SaveFrameToFile( output_file_path, frame_adrs, surface_info_adrs )

        # release frame        
        viddec.ReleaseFrame(frame_pts, False)

    # measure after completing a whole frame
    end_time = datetime.datetime.now()
    time_per_frame = end_time - start_time
    total_dec_time = total_dec_time + time_per_frame.total_seconds()

    # increament frames counter
    n_frame += n_frame_returned

    if (b_ret==False): # no more to decode?
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


# examples of command line args:
# python3 ../samples/py_videodecode.py -i /opt/rocm/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
# python3 ../samples/py_videodecode.py -i ../AMP_A_Samsung_4.bit -o TEST.raw

