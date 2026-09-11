# Copyright © Advanced Micro Devices, Inc., or its affiliates.
# SPDX-License-Identifier: MIT

"""Optional pixel checks; requires NumPy and ROCm-compatible PyTorch."""
import argparse
import gc
from pathlib import Path

import numpy as np
import torch
import pyRocJpegDecode.decoder as jdec


def pixels(image, fmt):
    if fmt == 3:
        tensor = torch.from_dlpack(image).cpu().numpy()
        array = image.to_numpy().copy()
    else:
        tensor = np.stack([torch.from_dlpack(b).cpu().numpy() for b in image.ext_buf], axis=-1)
        array = np.stack([image.to_numpy(i).copy() for i in range(3)], axis=-1)
    np.testing.assert_array_equal(tensor, array)
    return tensor


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--media-dir", required=True, type=Path)
    args = parser.parse_args()
    files = sorted(args.media_dir.glob("*.jpg"))
    assert files, "No JPEG fixtures"
    _, ready = jdec.initialize_hip()
    assert ready
    for path in files:
        layouts = []
        for fmt in (3, 4):
            decoder = jdec.decoder()
            decoder.set_output_image_format(fmt)
            _, image = decoder.decode(str(path))
            reference = pixels(image, fmt)
            data = path.read_bytes()
            for source in (data, np.frombuffer(data, dtype=np.uint8)):
                _, image = decoder.decode(source)
                np.testing.assert_array_equal(pixels(image, fmt), reference)
            _, batch = decoder.decode([str(path), str(path)])
            assert len(batch) == 2
            for image in batch:
                np.testing.assert_array_equal(pixels(image, fmt), reference)
            # Exported tensors retain storage after image/decoder deletion.
            _, owned_image = decoder.decode(str(path))
            view = torch.from_dlpack(owned_image.ext_buf[0])
            expected = view.clone()
            host = owned_image.to_numpy()
            del owned_image, decoder
            gc.collect()
            assert torch.equal(view, expected)
            np.testing.assert_array_equal(host, expected.cpu().numpy())
            layouts.append(reference)
        np.testing.assert_array_equal(*layouts)
        print(f"Pixel equality passed: {path.name}")


if __name__ == "__main__":
    main()
