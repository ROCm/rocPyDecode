# rocPyDecode main module
import rocPyDecode as pR
import ctypes 
import numpy as np
import datetime
import sys
import os.path

# empty init
input_file_path = ""
output_file_path = np.array("")
ref_md5_file = ""
 
# init for decoding
b_dump_output_frames = False     # can be passed as arg
b_generate_md5 = False           # can be passed as arg
b_md5_check = False              # can be passed as arg

# init Params (can be passed as args)
device_id = 0
mem_type = pR.OUT_SURFACE_MEM_DEV_INTERNAL
b_force_zero_latency = False
p_crop_rect = None
b_extract_sei_messages = False
b_flushing = True
device_name =  np.zeros(100,str)
gcn_arch_name = np.zeros(100,str)
pci_bus_id = np.array(1)
pci_domain_id = np.array(1)
pci_device_id = np.array(1)

def ShowHelpAndExit():
    print("\nCommand Line Options:"
    ,"\n-i             Input File Path - required"
    ,"\n-o             Output File Path - dumps output if requested; optional"
    ,"\n-d             GPU device ID (0 for the first device, 1 for the second, etc.);" 
    ,"\n               optional; default: 0"
    ,"\n-z             force_zero_latency (force_zero_latency, Decoded frames will be"
    ,"\n               flushed out for display immediately); optional;"
    ,"\n-sei           extract SEI messages; optional;"
    ,"\n-md5           generate MD5 message digest on the decoded YUV image sequence; optional;"
    ,"\n-md5_check     Input MD5 File Path - generate MD5 message digest on the decoded YUV image" 
    ,"\n               sequence and compare to the reference MD5 string in a file; optional;"
    ,"\n-crop          l t r b rectangle for output, l=left, t=top, r=right, b=bottom,"
    ,"\n               output crop rectangle must have width and height of even numbers,"
    ,"\n               (not used when using interopped decoded frame); optional; default: [original size]"
    ,"\n-m             output_surface_memory_type - decoded surface memory; optional; default - 0"
    ,"\n               [0 : OUT_SURFACE_MEM_DEV_INTERNAL, 1 : OUT_SURFACE_MEM_DEV_COPIED, 2 : OUT_SURFACE_MEM_HOST_COPIED]"
    ,"\n-h             Display this help information\n\n")
    sys.exit()

# parser for the params:
# ----------------------
args_count = len(sys.argv)

if args_count<=1:    
    ShowHelpAndExit()

crop_rect = np.zeros(4,int) # left, top, right, bottom, all zeros now

for i in range(args_count):

    # help 
    if (sys.argv[i] == "-h"):
        ShowHelpAndExit()
        continue

    # input file name-path
    if (sys.argv[i] == "-i"):
        if ((i+1) == args_count):
            ShowHelpAndExit()
        i += 1
        input_file_path = str(sys.argv[i])
        if (input_file_path==""):
            ShowHelpAndExit()
        if(os.path.isfile(input_file_path) == False):
            print("File name:", input_file_path, " doesn't exist, please check params .." )
            ShowHelpAndExit()
        continue

    # output file name, enable b_dump_output_frames
    if (sys.argv[i] == "-o"):
        if ((i+1) == args_count):
            ShowHelpAndExit()
        i += 1
        output_file_path = np.array(sys.argv[i])
        if (output_file_path==""):
            print("Output File Name is empty, please check params .." )
            ShowHelpAndExit()
             
        b_dump_output_frames = True
        continue
    
    # device id
    if (sys.argv[i] == "-d"):
        if ((i+1) == args_count):
            ShowHelpAndExit()
        i += 1
        device_id = int(sys.argv[i])
        continue
  
    # b_force_zero_latency
    if (sys.argv[i] == "-z"):       
        b_force_zero_latency = True
        continue

    # b_extract_sei_messages
    if (sys.argv[i] == "-sei"):    
        b_extract_sei_messages = True
        continue      

    # b_generate_md5
    if (sys.argv[i] == "-md5"):
        b_generate_md5 = True
        continue
 
    # b_generate_md5, b_md5_check, ref_md5_file
    if (sys.argv[i] == "-md5_check"):
        if ((i+1) == args_count):
            ShowHelpAndExit()        
        i += 1
        ref_md5_file = str(sys.argv[i])
        if(os.path.isfile(ref_md5_file) == False):
            print("File name:", ref_md5_file, " doesn't exist, please check params .." )
            ShowHelpAndExit()        
        b_generate_md5 = True 
        b_md5_check = True
        continue

    # crop rectangle
    if (sys.argv[i] == "-crop"):
        if ((i+4) >= args_count):
            ShowHelpAndExit()
        for s in range(4):
            crop_rect[s] = int(sys.argv[i+(s+1)])
        print("Crop: ", crop_rect)
        i += 3 # for loop will add another 1
        continue

    # mem_type, memory type
    if (sys.argv[i] == "-m"):
        if ((i+1) == args_count):
            ShowHelpAndExit()
        i += 1    
        mem_type = pR.OutputSurfaceMemoryType(int(sys.argv[i]))
        continue

# did user pass crop rect?
if(crop_rect[0]!=crop_rect[2] and crop_rect[1]!=crop_rect[3]):
    p_crop_rect = pR.Rect()
    p_crop_rect.l = crop_rect[0]
    p_crop_rect.t = crop_rect[1]
    p_crop_rect.r = crop_rect[2]
    p_crop_rect.b = crop_rect[3]

output_name_ptr = ctypes.c_void_p(output_file_path.ctypes.data) 

# instantiate decode object 
viddec = pR.pyRocVideoDecoder(input_file_path, device_id, mem_type, b_force_zero_latency, p_crop_rect, b_extract_sei_messages,0,0,0)
 
viddec.GetDeviceinfo(ctypes.c_void_p(device_name.ctypes.data),
                     ctypes.c_void_p(gcn_arch_name.ctypes.data),
                     ctypes.c_void_p(pci_bus_id.ctypes.data),
                     ctypes.c_void_p(pci_domain_id.ctypes.data),
                     ctypes.c_void_p(pci_device_id.ctypes.data))

#  print some info out  
d = np.string_(device_name).decode("utf-8")      
g = np.string_(gcn_arch_name).decode("utf-8")
id = str(pci_bus_id)
dm = str(pci_domain_id)
pc = str(pci_device_id) 
print("\ninfo: Input file: " + input_file_path + '\n' +
      "info: Using GPU device " + str(device_id) + " - " + d + "[" + g + "] on PCI bus " + id + ":" + dm + "." + pc )
print("info: decoding started, please wait! \n")
 
# initialize reconfigure params: 
cfg_flush = np.array([0],dtype=int)
cfg_dump = np.array([b_dump_output_frames],dtype=bool)
 
if b_generate_md5:
      viddec.InitMd5()
      
viddec.SetReconfigParams(ctypes.c_void_p(cfg_flush.ctypes.data),
                         ctypes.c_void_p(cfg_dump.ctypes.data),
                         output_name_ptr)
 
# -------------------------------------
# prepare params for the decoding loop 
# -------------------------------------
 
pkg_flags = np.array([0])                               
n_frame_returned = 0
pts = np.zeros(1,int)
b_t = np.zeros(1,bool)
n_frame = np.zeros(1,int)
total_dec_time = float(0.0)

# go until no more to decode
while True:           
    start_time = datetime.datetime.now()
    
    b_ret = viddec.demuxer.Demux()

    # Treat False ret as end of stream indicator
    if (b_ret == False):
        pkg_flags = pkg_flags | [int(pR.ROCDEC_PKT_ENDOFSTREAM)]

    n_frame_returned = viddec.DecodeFrame(ctypes.c_void_p(pkg_flags.ctypes.data))

    end_time = datetime.datetime.now()
    time_per_frame = end_time - start_time
    total_dec_time = total_dec_time + time_per_frame.total_seconds()
    
    b_ret_info = viddec.GetOutputSurfaceInfo() 

    if (n_frame==0 and b_ret_info==False):
        print("Error: Failed to get Output Surface Info!\n")
        break
                
    for i in range(n_frame_returned): 
        viddec.GetFrame(ctypes.c_void_p(pts.ctypes.data))        

        if b_generate_md5:
            viddec.UpdateMd5ForFrame()

        if b_dump_output_frames: 
            viddec.SaveFrameToFile( output_name_ptr )

        # release frame        
        viddec.ReleaseFrame(ctypes.c_void_p(pts.ctypes.data), 
                            ctypes.c_void_p(b_t.ctypes.data))

    n_frame += n_frame_returned

    if (b_ret==False): # no more to decode?
        break

# beyond the decoding LOOP
n_frame += viddec.GetNumOfFlushedFrames()

print("info: Total frame decoded: " + str(n_frame))

if b_dump_output_frames==False:
    print("info: avg decoding time per frame: " + str((total_dec_time / n_frame)*1000) + " ms")
    print("info: avg FPS: " + str(n_frame / total_dec_time) + "\n")

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
# (1)
# python3 ../examples/py_videodecode.py -i /opt/rocm/share/rocdecode/video/AMD_driving_virtual_20-H265.mp4
# (2)
# python3 ../examples/py_videodecode.py -i ../AMP_A_Samsung_4.bit -md5_check ../AMP_A_Samsung_4.md5 -o TEST.raw -m 1
