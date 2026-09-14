# Copyright © Advanced Micro Devices, Inc., or its affiliates.
# SPDX-License-Identifier: MIT

"""Exercise raw sample frame limits and failure exit codes using SDK media."""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def run(sample, args, expected_frames=None, error=None):
    result = subprocess.run(
        [sys.executable, str(sample), *map(str, args)],
        capture_output=True, text=True, timeout=60,
    )
    output = result.stdout + result.stderr
    if error is not None:
        assert result.returncode > 0 and error in output, output
    else:
        assert result.returncode == 0, output
        counts = re.findall(r"Decoded (\d+) frames", output)
        assert counts == [str(expected_frames)], output


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--media-dir", required=True, type=Path)
    args = parser.parse_args()
    sample = Path(__file__).resolve().parents[1] / "samples/rocdecode/videodecoderaw.py"
    for codec in ("264", "265"):
        media = args.media_dir / f"AMD_driving_virtual_20-H{codec}.{codec}"
        for count in (1, 2, 10):
            run(sample, ["-i", media, "-f", count], expected_frames=count)
    with tempfile.TemporaryDirectory() as directory:
        for codec in ("264", "265"):
            empty = Path(directory) / f"empty.{codec}"
            empty.touch()
            run(sample, ["-i", empty], error="No frames decoded")
            for count in (0, -2):
                run(sample, ["-i", empty, "-f", count], error="--frames must be")
    print("Raw video frame-limit and empty-input regressions passed")


if __name__ == "__main__":
    main()
