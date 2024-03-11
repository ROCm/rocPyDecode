 
import rocPyDecode as rocpydec   # rocpydecode main module
import rocPyDecode.decTypes as roctypes
import ctypes 
import numpy as np
import datetime
import sys
import argparse
import os.path

# testing access API
import amd.rocdecode.types as cty
from   amd.rocdecode.types import TestingImportClass

import amd.rocdecode.decoder as dec
import amd.rocdecode.demuxer as dex


xx = TestingImportClass()                   # test class
print( cty.rocDecVideoCodec_AV1, "\n")      # test types
print( cty.ROCDEC_PKT_ENDOFPICTURE, "\n")   # test types


# empty init
input_file_path = ""
output_file_path = np.array("")
ref_md5_file = ""
 
# init for decoding
b_dump_output_frames = False     # set if "Output File Path" is passed as arg
b_generate_md5 = False           # set if "Input MD5 File Path" is passed as arg
b_md5_check = False              # set if "generate_md5" is passed as arg

# init Params (set based on passed args)
b_extract_sei_messages = False
b_force_zero_latency = False
p_crop_rect = None
device_name =  np.zeros(100,str)
gcn_arch_name = np.zeros(100,str)
pci_bus_id = np.array(1)
pci_domain_id = np.array(1)
pci_device_id = np.array(1)
 
# accept arguments for: input_file_path, output_file_path, gpu_device_id, force_zero_latency_flag, extract_sei_messages_flag, generate_md5_message_digest_flag, input_md5_file_path, crop_rectangle_4_values, and output_surface_memory_type
parser = argparse.ArgumentParser(description='PyRocDecode Video Decode Arguments')
parser.add_argument('-i', '--input', type=str, help='Input File Path - required')
parser.add_argument('-o', '--output', type=str, help='Output File Path - optional')
parser.add_argument('-d', '--device', type=int, default=0, help='GPU device ID - optional, default 0')
parser.add_argument('-z', '--zero_latency', type=str, help='Force zero latency - [optios: yes,no], default: no')
parser.add_argument('-sei', '--extract_sei', type=str, help='Extract SEI messages - [optios: yes,no], default: no')
parser.add_argument('-md5', '--generate_md5',type=str, help='Generate MD5 message digest - [optios: yes,no], default: no')
parser.add_argument('-md5_check', '--input_md5', type=str, help='Input MD5 File Path, optional, if passed  then -md5 set to yes')
parser.add_argument('-crop', '--crop_rect', nargs='+', type=int, help='Crop rectangle (left, top, right, bottom), optional, default: no cropping')
parser.add_argument('-m', '--mem_type', type=int, default=0, help='Output surface memory type, default 0, [options: 0:OUT_SURFACE_MEM_DEV_INTERNAL, 1:OUT_SURFACE_MEM_DEV_COPIED, 2:OUT_SURFACE_MEM_HOST_COPIED]')

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
extract_sei_messages = args.extract_sei
generate_md5 = args.generate_md5
ref_md5_file = args.input_md5
crop_rect = args.crop_rect
mem_type = roctypes.OutputSurfaceMemoryType(args.mem_type)
 
# do we have rect from user
if (args.crop_rect != None):
    if (args.crop_rect != [0,0,0,0]):
        p_crop_rect = rocpydec.Rect()
        p_crop_rect.l = crop_rect[0]
        p_crop_rect.t = crop_rect[1]
        p_crop_rect.r = crop_rect[2]
        p_crop_rect.b = crop_rect[3]
    
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

# md5 file full path, & md5 flag
if (ref_md5_file!=None):
    if os.path.exists(ref_md5_file):
        b_generate_md5 = True
        b_md5_check = True
    else:
        print("Warning: The reference MD5 input file full path name is not valid,\n\t incorrect, or file doesn't exist,\n\t no MD5 check will be performed.\n")

# md5
if (generate_md5 in ('yes','no')):
    if generate_md5 =='yes':
        b_generate_md5 = True
 
# force 0 latency
if (force_zero_latency in ('yes','no')):
    if force_zero_latency =='yes':
        b_force_zero_latency = True

# sei
if (extract_sei_messages in ('yes','no')):
    if extract_sei_messages =='yes':
        b_extract_sei_messages = True

# valid to pass
if (output_file_path==None):
    output_file_path = np.array("")
else:
    output_file_path = np.array(output_file_path)

output_name_ptr = ctypes.c_void_p(output_file_path.ctypes.data) 

# instantiate demuxer instance 
demuxer = dex.demuxer(input_file_path)

# get the used coded id
coded_id = rocpydec.AVCodec2RocDecVideoCodec(demuxer.GetCodec_ID())
 
# instantiate decoder instance 
viddec = dec.decoder( device_id, mem_type, coded_id, b_force_zero_latency, p_crop_rect, b_extract_sei_messages,0,0,0)

# Get GPU device information
viddec.Get_GPU_Info(device_name,gcn_arch_name,pci_bus_id,pci_domain_id,pci_device_id)

#  print some info out  
d = np.string_(device_name).decode("utf-8")      
g = np.string_(gcn_arch_name).decode("utf-8")
print("\ninfo: Input file: " + input_file_path + '\n' +
      "info: Using GPU device " + str(device_id) + " - " + d + "[" + g + "] on PCI bus " + str(pci_bus_id) + ":" + str(pci_domain_id) + "." + str(pci_device_id) )
print("info: decoding started, please wait! \n")
 
# initialize reconfigure params: 
cfg_flush = np.array([0],dtype=int)
cfg_dump = np.array([b_dump_output_frames],dtype=bool)
 
if b_generate_md5:
      viddec.InitMd5()
      
viddec.SetReconfigurationParams( cfg_flush, cfg_dump, output_name_ptr)
 
# -------------------------------------
# prepare params for the decoding loop 
# -------------------------------------
 
pkg_flags = int(0)                              
n_frame_returned = 0
 
b_t = np.ndarray(shape=(1), dtype=np.uint8)

n_frame = int(0)
total_dec_time = float(0.0)

frame_adrs = np.ndarray(shape=(0), dtype=np.uint64) # one uint64 storage (carries address)
frame_size = np.ndarray(shape=(0), dtype=np.int64)  # one int64  storage (carries int value)
frame_pts  = np.ndarray(shape=(0), dtype=np.int64)  # one int64  storage (carries int value)

surface_info_struct = rocpydec.OutputSurfaceInfo() 
surface_info_adrs   = np.ndarray(shape=(0), dtype=np.uint8)
print_surface_info = True

# go until no more to decode
while True:           
    start_time = datetime.datetime.now()
    
    b_ret = demuxer.DemuxFrame(frame_adrs, frame_size, frame_pts)

    # Treat False ret as end of stream indicator
    if (b_ret == False):
        pkg_flags = pkg_flags | int(roctypes.ROCDEC_PKT_ENDOFSTREAM)

    n_frame_returned = viddec.DecodeFrame(frame_adrs[0], frame_size[0], pkg_flags, frame_pts[0])

    # OutputSurfaceInfo **surface_info_adrs  
    b_ret_info = viddec.GetOutputSurfaceInfoAdrs(surface_info_struct, surface_info_adrs) 

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
        print ( "mem_type: \t\t",        surface_info_struct.mem_type)
        print ("\n")			

    if (n_frame==0 and b_ret_info==False):
        print("Error: Failed to get Output Surface Info!\n")
        break
                
    for i in range(n_frame_returned): 
        viddec.GetFrameAddress(frame_pts, frame_adrs)        

        if b_generate_md5:
            viddec.UpdateMd5ForFrame(frame_adrs[0], surface_info_adrs)

        if b_dump_output_frames: 
            viddec.SaveFrameToFile( output_name_ptr, frame_adrs[0], surface_info_adrs )

        # release frame        
        viddec.ReleaseFrame(frame_pts, b_t)

    # measure after completing a whole frame
    end_time = datetime.datetime.now()
    time_per_frame = end_time - start_time
    total_dec_time = total_dec_time + time_per_frame.total_seconds()

    n_frame += n_frame_returned

    if (b_ret==False): # no more to decode?
        break

# beyond the decoding LOOP
n_frame += viddec.GetNumOfFlushedFrames()

print("info: Total frame decoded: " + str(n_frame))

if b_dump_output_frames==False:
    if(n_frame>0):
        print("info: avg decoding time per frame: " + str((total_dec_time / n_frame)*1000) + " ms")
        print("info: avg FPS: " + str(n_frame / total_dec_time) + "\n")
    else:
        print( "info: frame count= ", n_frame )
      
if b_generate_md5:
    digest = np.zeros(16,np.uint8)
    str_digest = ""
    viddec.FinalizeMd5(ctypes.c_void_p(digest.ctypes.data))
    print("MD5 message digest: ",end = " ")
    for i in range(16):
        str_digest = str_digest + str(format('%02x' % int(digest[i])))
    print(str_digest)

    if (b_md5_check):  
        f = open(ref_md5_file) 
        md5_from_file = f.read(16*2) 
        b_match = (md5_from_file == str_digest)
        if(b_match):
            print("MD5 digest matches the reference MD5 digest.\n")
        else:
            print("MD5 digest does not match the reference MD5 digest: ", md5_from_file)

print("\n") # end


# examples of command line :
# (1) python3 ../samples/py_videodecode.py -i /opt/rocm/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
# (2) python3 ../samples/py_videodecode.py -i ../AMP_A_Samsung_4.bit -md5_check ../AMP_A_Samsung_4.md5 -o TEST.raw -m 1
# To debug in vscode, add this [example arg list] to your debugger launch.json:
# "args": ["-i","/opt/rocm/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4","-o","test_frames_out.RAW","-m","1", "-d","0","-z","yes","-sei","yes","-md5","yes","-md5_check","input_md5_file_name.txt","-crop","0","0","100","200"]

