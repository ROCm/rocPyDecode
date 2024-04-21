import datetime
import sys
import argparse
import os.path
import torch
import torchvision 
import numpy as np
import pyRocVideoDecode.decoder as dec
import pyRocVideoDecode.demuxer as dmx
import pyRocVideoDecode.catlabels as cat


def Decoder(input_file_path, device_id):

    # Init resnet
    model = torchvision.models.resnet50(weights=torchvision.models.ResNet50_Weights.DEFAULT)
    model.eval()
    model.to("cuda")

    # Resnet expects images to be 3 channel planar RGB of 224x244 size at least.
    target_w, target_h = 224, 224
   
    # demuxer instance
    demuxer = dmx.demuxer(input_file_path)

    # get the used coded id
    coded_id = dec.GetRocDecCodecID(demuxer.GetCodecId())

    # decoder instance
    viddec = dec.decoder(device_id, coded_id, False, None, 0, 0, 0)

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

    # -----------------
    # The decoding loop
    # -----------------
    n_frame = 0
    total_dec_time = 0.0

    while True:
        start_time = datetime.datetime.now()

        packet = demuxer.DemuxFrame()

        if (packet.end_of_stream):
            break

        n_frame_returned = viddec.DecodeFrame(packet)

        for i in range(n_frame_returned):
            viddec.GetFrame(packet)

            # using torch tensor
            img_tensor = torch.from_dlpack(packet.extBuf.__dlpack__(packet))
            
            img_tensor.resize_(3, target_h, target_w)
            img_tensor = img_tensor.type(dtype=torch.cuda.FloatTensor)
            img_tensor = torch.divide(img_tensor, 255.0)

            data_transforms = torchvision.transforms.Normalize(
                mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]
            )
            surface_tensor = data_transforms(img_tensor)
            input_batch = surface_tensor.unsqueeze(0).to("cuda")

            # Run inference.
            with torch.no_grad():
                output = model(input_batch)

            probabilities = torch.nn.functional.softmax(output[0], dim=0)

            top5_prob, top5_catid = torch.topk(probabilities, 5)
            for i in range(top5_prob.size(0)):
                print(cat.categories[top5_catid[i]], top5_prob[i].item())
            print()

            # release frame
            viddec.ReleaseFrame(packet)

        # measure after completing a whole frame
        end_time = datetime.datetime.now()
        time_per_frame = end_time - start_time
        total_dec_time = total_dec_time + time_per_frame.total_seconds()

        # increament frames counter
        n_frame += n_frame_returned

        if (packet.end_of_stream):  # no more to decode?
            break

    # beyond the decoding loop
    n_frame += viddec.GetNumOfFlushedFrames()

    print("info: Total frame decoded: " + str(n_frame))
    print("info: frame count= ", n_frame)
    print()
 


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

    try:
        args = parser.parse_args()
    except BaseException:
        sys.exit()

    input_file_path = args.input
    device_id = args.device

    # clr.PrintTest(1)

    # Input file (must exist)
    if not os.path.exists(input_file_path):
        print("ERROR: input file doesn't exist.")
        exit()

    print("\nPyTorch Using: ", torch.cuda.get_device_name(0))

    Decoder(input_file_path, device_id)
