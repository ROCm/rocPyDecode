import pyRocVideoDecode.decoder as dec
import pyRocVideoDecode.demuxer as dmx
import datetime
import sys
import argparse
import os.path
from mpi4py import MPI	
import subprocess
import av


def Decoder(
        input_file_path,
        output_file_path,
        usr_dbg,
        use_ssh
        ):

    # mpi
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()
    temp_frames_folder = 'temp_frames_folder/'

    # if use_ssh
    if use_ssh and rank == 0:
        print('\n','*'*75, "\n * Using -ssh assumes that you have setup your nodes appropriately")
        print(" * to successfully communicate with the base node 0 or with each other.\n", '*'*75,'\n')
        
    # using ssh allows scale across many external nodes (in this example)
    if use_ssh and (rank == 0):
        my_user_name = subprocess.run("whoami", shell=True, capture_output=True, text=True).stdout.strip()
        my_ip_addrs  = subprocess.run("hostname -I | awk '{print $1}'", shell=True, capture_output=True, text=True).stdout.strip()
        node0_folder  = subprocess.run("pwd", shell=True, capture_output=True, text=True).stdout.strip()
        os.makedirs(temp_frames_folder, exist_ok=True)
        destination_ssh_folder = my_user_name+'@'+my_ip_addrs+':'+node0_folder+'/'+temp_frames_folder
        print(f"destination_ssh_folder: {destination_ssh_folder}")
    else:        
        destination_ssh_folder = None

    # all know dest if exist
    if output_file_path is not None and use_ssh:
        destination_ssh_folder = comm.bcast(destination_ssh_folder, root=0)
                
    # TODO (rocDecode):
    # The rocDecode demuxer/parser needs to go through the  
    # entire stream without decoding, to get the count of
    # frames till this API being implemented, use av to ..

    # .. get count of frames
    if rank == 0:        
        container = av.open(input_file_path)               # Open the video file
        frames_total = container.streams.video[0].frames   # Get total number of frames from first video stream
    else:
        frames_total = None

    # demuxer instance
    demuxer = dmx.demuxer(input_file_path)
    # get the used coded id
    codec_id = dec.GetRocDecCodecID(demuxer.GetCodecId())
    # decoder instance
    viddec = dec.decoder(codec_id)
    # check if codec is supported
    if (viddec.IsCodecSupported(0, codec_id, demuxer.GetBitDepth()) == False):
        print("ERROR: Codec is not supported on this GPU " + viddec.GetGpuInfo().device_name)
        exit()

    # ----
    # MPI
    # ----
 
    # all ranks to know what is 'frames_total'
    frames_total = comm.bcast(frames_total, root=0)
 
     # calc for all
    frames_remain = frames_total % size
    each_proc_frame_count = int((frames_total-frames_remain)/size)
    this_rank_start = rank * each_proc_frame_count
    this_rank_end  = this_rank_start + each_proc_frame_count - 1 

    # add 'frames_remain' to last rank
    if (rank == (size-1)):
        this_rank_end += frames_remain # last rank takes care of the residue

    # no need for file operations if user do not want to save frames, save time, performance
    save_output_file_path = None
    if rank == 0:
        if output_file_path is not None:
            save_output_file_path = output_file_path

    # serialize the output frames files (in case we used it)
    output_file_path = f"output_frames_{rank}_2048x1024_8bit_420.yuv" 
    flush_mode = 1
    # set reconfiguration params based on user arguments
    viddec.SetReconfigParams(flush_mode, output_file_path)

    # all ranks to know if we saved is temp filename
    save_decoded_section = False
    if rank == 0:        
        if save_output_file_path is not None:
            save_decoded_section = True
    save_decoded_section = comm.bcast(save_decoded_section, root=0)

    # -----------------
    # The decoding loop
    # -----------------

    frame_index = 0
    total_dec_time = 0.0

    while True:

        start_time = datetime.datetime.now() # whole frame
        packet = demuxer.DemuxFrame()
        n_of_frames_returned = viddec.DecodeFrame(packet)

        for i in range(n_of_frames_returned):

            viddec.GetFrameYuv(packet)

            # only save when requested, and only save the assigned frames to this rank
            if (save_decoded_section and (output_file_path is not None)):
                if ((frame_index >= this_rank_start) and (frame_index <= this_rank_end)): # only the assigned section of frames
                    viddec.SaveFrameToFile(output_file_path, packet.frame_adrs)

            # release frame
            viddec.ReleaseFrame(packet)

        # measure time after completing a whole frame
        end_time = datetime.datetime.now()
        time_per_frame = end_time - start_time
        total_dec_time = total_dec_time + time_per_frame.total_seconds()

        # increament frames counter
        frame_index += n_of_frames_returned

        # stop if no more to demux/decode
        if (packet.bitstream_size <= 0):  # EOF: no more to decode
            break

    # beyond the decoding loop
    frame_index += viddec.GetNumOfFlushedFrames()        

    # collect total elapsed time in all processes
    total_time_elapsed = comm.gather(total_dec_time, root=0)

    # collect session overhead from all processes
    session_overhead = comm.gather(viddec.GetDecoderSessionOverHead(rank), root=0)

    # copy resultled file from local node to main node 0, then delete it, only if ssh used
    if (save_decoded_section and use_ssh):
        cmmnd = 'scp '+output_file_path+' '+destination_ssh_folder+output_file_path
        subprocess.run(cmmnd, shell=True, check=True)

    # make sure all finished their share of the job
    comm.barrier() # synchronize all processes

    # if user used output file then save all in one
    if rank == 0:
        if save_output_file_path is not None:
            files_list = ''
            for i in range(size):
                files_list += f"output_frames_{i}_2048x1024_8bit_420.yuv "
            
            cat_cmd = 'cat '+files_list+' > '+save_output_file_path
            rm_cmd = 'rm '+files_list                        
            
            print("\nConstructing YUV file from nodes results and removing temp files ..")
            
            if use_ssh is not True:
                subprocess.run(cat_cmd, shell=True, check=True)
                subprocess.run(rm_cmd, shell=True, check=True)
            else: # this is happening here locally at node 0
                chg_dir = " \"cd "+node0_folder+'/'+temp_frames_folder
                subprocess.run(chg_dir +" && "+ cat_cmd + "\"", shell=True, check=True)
                # you may need to keep the temp files, if so comment next line
                subprocess.run(chg_dir + " && " + rm_cmd + "\"", shell=True, check=True)
        else:
            print("No output-file was specified..\n")

        # report time
        frame_index = frame_index * size # we decoded 'size' times, depends on the -n mpirun/mpiexec param
        total_dec_time = sum(total_time_elapsed)/size # average of all processes time
        time_per_frame = (total_dec_time / frames_total) * 1000 # we decoded ALL anyway
        time_per_frame -= (sum(session_overhead) / frame_index) # remove the overhead (if any)
        frame_per_second = frame_index / total_dec_time
        print("info: avg decoding time per frame: " +"{0:0.2f}".format(round(time_per_frame, 2)) + " ms")
        print("info: avg frame per second: " +"{0:0.2f}".format(round(frame_per_second,2)) +"\n")

        # for user dbg
        if usr_dbg:
            print(f"\nElapsed time reported by all processes: {total_time_elapsed}")
            print(f"Highest elapsed time (actual decoding time): {max(total_time_elapsed)}")
            print("Total processors: ", size)
            print("Frames per proc:  ", each_proc_frame_count)
            print("Frames Remaining: ", frames_remain)

    # done using MPI
    MPI.Finalize


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
        '-dbg',
        '--debug',
        type=str,
        default='no',
        choices=['yes', 'no'],
        help='Print out some debug information')
    parser.add_argument(
        '-ssh',
        '--use_ssh',
        type=str,
        default='no',
        choices=['yes', 'no'],
        help='Use ssh scp to copy decoded frames, this example way of consumption'
        )

    try:
        args = parser.parse_args()
    except BaseException:
        sys.exit()

    # get params
    input_file_path = args.input
    output_file_path = args.output
    usr_dbg = args.debug.upper()
    use_ssh = args.use_ssh.upper()
    
    # handle params
    usr_dbg = True if usr_dbg == 'YES' else False
    use_ssh = True if use_ssh == 'YES' else False
    if not os.path.exists(input_file_path):  # Input file (must exist)
        print("ERROR: input file doesn't exist.")
        exit()

    # call main routine
    Decoder(
        input_file_path,
        output_file_path,
        usr_dbg,
        use_ssh
    )