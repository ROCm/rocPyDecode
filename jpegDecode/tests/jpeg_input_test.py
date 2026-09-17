# Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

"""Exercise JPEG input conversion with NumPy available and unavailable."""
import argparse
import importlib.util
from pathlib import Path
import subprocess
import sys


def check_inputs(path, without_numpy):
    if without_numpy:
        assert importlib.util.find_spec("numpy") is None, sys.path
        numpy = None
    elif importlib.util.find_spec("numpy") is not None:
        import numpy
    else:
        numpy = None

    import rocpyjpegdecode as native
    import pyRocJpegDecode.decoder as jpeg

    data = path.read_bytes()
    stream = native.CodeStream(data)
    source = native.DecodeSource(stream)
    inputs = {
        "filename": lambda: str(path),
        "bytes": lambda: data,
        "stream": lambda: stream,
        "source": lambda: source,
        "stream-filename": lambda: native.CodeStream(str(path)),
        "stream-path": lambda: native.CodeStream(path),
        "source-filename": lambda: native.DecodeSource(str(path)),
        "source-bytes": lambda: native.DecodeSource(data),
        "batch-filenames": lambda: [str(path), str(path)],
        "batch-mixed": lambda: [str(path), data, stream, source],
        "tuple-batch": lambda: (str(path), str(path)),
    }
    if numpy is not None:
        array = numpy.frombuffer(data, dtype=numpy.uint8)
        padded = numpy.zeros(len(data) * 2, dtype=numpy.uint8)
        padded[::2] = array
        reverse = array[::-1].copy()
        inputs.update({
            "array": lambda: array,
            "strided-array": lambda: padded[::2],
            "negative-stride-array": lambda: reverse[::-1],
            "stream-array": lambda: native.CodeStream(array),
            "source-array": lambda: native.DecodeSource(array),
            "batch-arrays": lambda: [array, padded[::2]],
        })

    count = 0
    for fmt in (3, 4):
        decoder = jpeg.decoder(output_format=fmt)
        _, reference = decoder.decode(source)
        channels = range(3 if fmt == 4 else 1)
        reference_pixels = ([reference.to_numpy(i).tobytes() for i in channels]
                            if numpy is not None else None)
        for method in (decoder.decode, decoder.read):
            for name, make_input in inputs.items():
                _, result = method(make_input())
                images = result if isinstance(result, list) else [result]
                expected_count = 4 if name == "batch-mixed" else (
                    2 if name.startswith("batch-") or name == "tuple-batch" else 1)
                assert len(images) == expected_count, name
                for image in images:
                    assert (image.width, image.height) == (reference.width, reference.height), name
                    assert image.dtype == "|u1", name
                    for i in channels:
                        assert image.ext_buf[i].shape == reference.ext_buf[i].shape, name
                        assert image.ext_buf[i].strides == reference.ext_buf[i].strides, name
                        assert image.ext_buf[i].__dlpack__(None) is not None, name
                    if reference_pixels is not None:
                        assert [image.to_numpy(i).tobytes() for i in channels] == reference_pixels, name
                count += 1
        if numpy is not None:
            # Preserve the existing distinction: direct decode accepts uint8
            # arrays; explicit constructors may convert another numeric dtype.
            try:
                decoder.decode(array.astype(numpy.uint16))
            except TypeError:
                pass
            else:
                raise AssertionError("Implicit array conversion accepted uint16")
            _, image = decoder.decode(native.DecodeSource(array.astype(numpy.uint16)))
            assert [image.to_numpy(i).tobytes() for i in channels] == reference_pixels
        for invalid in (object(), 42, None, [object()]):
            try:
                decoder.decode(invalid)
            except (TypeError, ValueError):
                pass
            else:
                raise AssertionError("Invalid decode input was accepted")

    if without_numpy:
        assert "numpy" not in sys.modules
    print(f"JPEG input conversions passed: {count}; NumPy available: {numpy is not None}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--media-dir", required=True, type=Path)
    parser.add_argument("--without-numpy", action="store_true")
    args = parser.parse_args()
    images = sorted(args.media_dir.glob("*.jpg"))
    assert images, "JPEG input regression requires JPEG media"
    if not args.without_numpy:
        # Keep the real extension and wrappers on PYTHONPATH, but remove site
        # packages from a fresh interpreter so NumPy cannot be imported.
        subprocess.run([sys.executable, "-S", "-B", str(Path(__file__).resolve()),
                        "--media-dir", str(args.media_dir), "--without-numpy"],
                       check=True, timeout=120)
    check_inputs(images[0], args.without_numpy)


if __name__ == "__main__":
    main()
