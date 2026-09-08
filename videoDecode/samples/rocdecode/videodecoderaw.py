# Copyright © Advanced Micro Devices, Inc., or its affiliates.

# SPDX-License-Identifier: MIT License

import pyRocVideoDecode.decoder as dec
import pyRocVideoDecode.types as dectypes
import argparse
import os
import time
from pathlib import Path


def _resolve_codec(codec_arg, input_path):
    """Map user choice or filename hint to rocDecVideoCodec enum."""
    if codec_arg:
        name = codec_arg.lower()
    else:
        name = Path(input_path).suffix.lower().lstrip(".")
    if name in ("h264", "264", "avc"):
        return dectypes.rocDecVideoCodec_AVC, "H.264/AVC"
    if name in ("h265", "265", "hevc"):
        return dectypes.rocDecVideoCodec_HEVC, "H.265/HEVC"
    raise ValueError(
        "Unable to infer codec. Pass --codec {h264|h265} or use a .h264/.h265 filename."
    )


def _normalize_output_path(output_arg, input_path):
    if not output_arg:
        return None
    output_path = Path(output_arg)
    if output_path.is_dir():
        output_path = output_path / (Path(input_path).stem + ".yuv")
    return output_path


def _surface_format_tag(surface_format: int) -> str:
    if surface_format == dectypes.rocDecVideoSurfaceFormat_NV12 or surface_format == dectypes.rocDecVideoSurfaceFormat_P016:
        return "YUV420"
    if surface_format == dectypes.rocDecVideoSurfaceFormat_YUV444:
        return "YUV444"
    if surface_format == dectypes.rocDecVideoSurfaceFormat_YUV444_16Bit:
        return "YUV444_16"
    return "YUV"


def _format_output_path(base_path: Path, input_path: str, width: int, height: int, bit_depth: int, fmt_tag: str) -> Path:
    # Append resolution/bitdepth/format to filename before extension.
    p = base_path
    if p.is_dir():
        p = p / Path(input_path).stem
    stem = p.stem
    suffix = p.suffix if p.suffix else ".yuv"
    return p.with_name(f"{stem}_{width}x{height}_{bit_depth}Bit_{fmt_tag}{suffix}")


def _annexb_slices(data: memoryview):
    # memoryview slices for each Annex-B NAL unit (includes start code).
    starts = []
    i = 0
    n = len(data)
    while True:
        pos = data[i:].tobytes().find(b"\x00\x00\x01")
        if pos == -1:
            break
        pos += i
        # include 4-byte start code if present
        if pos > 0 and data[pos - 1] == 0:
            pos -= 1
        starts.append(pos)
        i = pos + 3
    starts.append(n)
    for s, e in zip(starts[:-1], starts[1:]):
        yield data[s:e]


def decode_raw(
    input_path,
    output_path,
    device_id,
    mem_type,
    zero_latency,
    crop_rect,
    max_frames,
    codec,
    codec_label,
):
    decoder = dec.decoder(
        codec=codec,
        device_id=device_id,
        mem_type=mem_type,
        b_force_zero_latency=zero_latency,
        crop_rect=crop_rect,
        max_width=0,
        max_height=0,
        clk_rate=1000,
    )
    gpu_info = decoder.GetGpuInfo()
    print(
        f"info: Input={input_path}\n"
        f"info: Using GPU {device_id} - {gpu_info.device_name} [{gpu_info.gcn_arch_name}] "
        f"PCI {gpu_info.pci_bus_id}:{gpu_info.pci_domain_id}.{gpu_info.pci_device_id}\n"
        f"info: Codec={codec_label}, zero-latency={'on' if zero_latency else 'off'}"
    )

    frame_count = 0
    start_time = time.time()
    frame_index = 0
    output_final_path = None
    flush_mode = 1 if output_path else 0

    def feed_packet(buf, is_eos=False):
        nonlocal frame_count, frame_index, output_final_path
        packet = dec.GetRocPyDecPacket(frame_index, len(buf), buf)
        packet.pkt_flags = 0
        packet.end_of_stream = False
        if is_eos:
            packet.pkt_flags |= int(dectypes.ROCDEC_PKT_ENDOFSTREAM)
        decoded_now = decoder.DecodeFrame(packet)
        for _ in range(decoded_now):
            decoder.GetFrameYuv(packet, False)
            if output_path and output_final_path is None:
                width = decoder.GetWidth()
                height = decoder.GetHeight()
                bit_depth = decoder.GetBitDepth()
                fmt_tag = "YUV"  # default; surface format not exposed in this wrapper
                output_final = _format_output_path(Path(output_path), input_path, width, height, bit_depth, fmt_tag)
                decoder.SetReconfigParams(flush_mode, str(output_final))
                output_final_path = output_final
            if output_path:
                target = output_final_path if output_final_path else output_path
                decoder.SaveFrameToFile(str(target), packet.frame_adrs)
            decoder.ReleaseFrame(packet)
            frame_count += 1
            frame_index += 1
            if 0 < max_frames <= frame_count:
                return True
        return False

    data = memoryview(Path(input_path).read_bytes())
    sps = pps = None
    for nal in _annexb_slices(data):
        # nal header byte after start code
        sc_len = 4 if nal[:4] == b"\x00\x00\x00\x01" else 3
        nal_type = nal[sc_len] & 0x1F
        if nal_type == 7:  # SPS
            sps = bytes(nal)
            continue
        if nal_type == 8:  # PPS
            pps = bytes(nal)
            continue
        if nal_type in (1, 5):  # non-IDR / IDR slice -> frame
            # prepend latest SPS/PPS so each frame is self-contained
            parts = []
            if sps:
                parts.append(sps)
            if pps:
                parts.append(pps)
            parts.append(bytes(nal))
            au = b"".join(parts)
            if feed_packet(memoryview(au)):
                break
    # EOS
    feed_packet(memoryview(b""), is_eos=True)
    # Account for any frames still buffered in decoder
    frame_count += decoder.GetNumOfFlushedFrames()

    elapsed = time.time() - start_time
    if frame_count and elapsed > 0:
        fps = frame_count / elapsed
        print(
            f"info: Decoded {frame_count} frames in {elapsed:.2f}s "
            f"({fps:.2f} fps)."
        )
    else:
        print("info: No frames decoded.")


def main():
    parser = argparse.ArgumentParser(description="Decode a raw bitstream with rocPyDecode (no FFmpeg demux).")
    parser.add_argument(
        "-i", 
        "--input", 
        required=True, 
        help="Raw input bitstream file."
    )
    parser.add_argument(
        "-o",
        "--output",
        help=("Output file to save decoded YUV frames (optional). Provide a filename or full path."),
    )
    parser.add_argument(
        "-d",
        "--device",
        type=int,
        default=0,
        help="GPU device ID (default: 0).",
    )
    parser.add_argument(
        "-m",
        "--mem_type",
        type=int,
        default=dectypes.OUT_SURFACE_MEM_DEV_COPIED,
        help="Output surface memory type [0:internal, 1:dev_copied, 2:host_copied, 3:not_mapped] (default: 1).",
    )
    parser.add_argument(
        "-f",
        "--frames",
        type=int,
        default=-1,
        help="Number of frames to decode (-1 means all).",
    )
    parser.add_argument(
        "-z",
        "--zero_latency",
        type=str,
        default="no",
        choices=["yes", "no"],
        help="Force zero latency (flush frames ASAP).",
    )
    parser.add_argument(
        "-crop",
        "--crop_rect",
        nargs=4,
        type=int,
        metavar=("LEFT", "TOP", "RIGHT", "BOTTOM"),
        help="Crop rectangle (default: no crop).",
    )
    parser.add_argument(
        "-c",
        "--codec",
        choices=["h264", "h265"],
        help="Codec of the raw bitstream (guessed from extension if omitted).",
    )

    args = parser.parse_args()

    if not os.path.exists(args.input):
        raise FileNotFoundError(f"Input file not found: {args.input}")

    mem_type = max(
        dectypes.OUT_SURFACE_MEM_DEV_INTERNAL,
        min(args.mem_type, dectypes.OUT_SURFACE_MEM_NOT_MAPPED),
    )

    codec_enum, codec_label = _resolve_codec(args.codec, args.input)
    output_path = _normalize_output_path(args.output, args.input)
    zero_latency = True if args.zero_latency.upper() == "YES" else False

    decode_raw(
        input_path=args.input,
        output_path=output_path,
        device_id=args.device,
        mem_type=mem_type,
        zero_latency=zero_latency,
        crop_rect=args.crop_rect,
        max_frames=args.frames,
        codec=codec_enum,
        codec_label=codec_label,
    )


if __name__ == "__main__":
    main()
