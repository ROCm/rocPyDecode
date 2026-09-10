# Copyright © Advanced Micro Devices, Inc., or its affiliates.
# SPDX-License-Identifier: MIT

"""Check decoded RGB metadata and batch-sample failures without PyTorch."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

import pyRocJpegDecode.decoder as jdec
import rocpyjpegdecode as native


def run(sample, args, expected_error=None, expected_output=None):
    result = subprocess.run(
        [sys.executable, str(sample), *map(str, args)],
        capture_output=True, text=True, timeout=60,
    )
    output = result.stdout + result.stderr
    if expected_error:
        assert result.returncode > 0 and expected_error in output, output
    else:
        assert result.returncode == 0, output
        assert expected_output in output, output


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--media-dir", required=True, type=Path)
    args = parser.parse_args()
    files = sorted(args.media_dir.glob("*.jpg"))
    assert files, "JPEG regression requires SDK JPEG media"
    device_count, ready = jdec.initialize_hip()
    assert ready
    for fmt in (3, 4):
        decoder = jdec.decoder()
        decoder.set_output_image_format(fmt)
        for path in files:
            _, img = decoder.decode(str(path))
            assert img.width > 0 and img.height > 0
            if fmt == 3:
                assert img.shape == (img.height, img.width, 3), img.shape
                assert img.strides == (((img.width * 3 + 255) // 256) * 256, 3, 1), img.strides
            else:
                for buf in img.ext_buf:
                    assert buf.shape == (img.height, img.width), buf.shape
                    assert buf.strides == (((img.width + 255) // 256) * 256, 1), buf.strides
            assert img.dtype == "|u1", img.dtype
    # Converting a CodeStream must not destroy a handle still used by its source.
    stream = native.CodeStream(files[0].read_bytes())
    source = native.DecodeSource(stream)
    borrowed_stream = source.code_stream
    del source
    decoder = jdec.decoder()
    for item in (stream, stream, borrowed_stream):
        _, image = decoder.decode(item)
        assert image.width > 0 and image.height > 0
    _, batch = decoder.decode([stream, stream])
    assert len(batch) == 2
    assert native.CodeStream(b"") is not None
    sample = Path(__file__).resolve().parents[1] / "samples/rocjpeg/jpegdecodebatched.py"
    run(sample, ["-i", args.media_dir, "-d", device_count], expected_error="not found")
    with tempfile.TemporaryDirectory() as directory:
        folder = Path(directory)
        run(sample, ["-i", folder], expected_error="No images decoded")
        (folder / "invalid.jpg").write_bytes(b"not a JPEG")
        run(sample, ["-i", folder], expected_error="No images decoded")
        run(sample, ["-i", folder / "missing"], expected_error="not a directory")
        for batch in (0, -1):
            run(sample, ["-i", folder, "-b", batch], expected_error="positive integer")
        run(sample, [], expected_error="required")
        run(sample, ["--help"], expected_output="usage:")
        # The sample intentionally skips bad files when useful images remain.
        shutil.copyfile(files[0], folder / "valid.jpg")
        run(sample, ["-i", folder], expected_output="Total files processed : 1")
    print("JPEG RGB-layout and batch-error regressions passed")


if __name__ == "__main__":
    main()
