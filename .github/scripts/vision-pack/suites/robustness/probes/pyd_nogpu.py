#!/usr/bin/env python3
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

"""rocPyDecode / rocPyJpegDecode decoder creation with no GPU visible must fail cleanly (see verdict.py).

    pyd_nogpu.py video|jpeg

The attempt runs in a child interpreter: native code that terminates the process (exit() from C++)
cannot be caught in Python, and is reported as outcome "process_exit", which no mode accepts.
"""
from __future__ import annotations

import subprocess
import sys

from verdict import crashed, finish

MARK = "PYD-NOGPU-OUTCOME"


def inner(what: str) -> None:
    try:
        if what == "video":
            import pyRocVideoDecode.decoder as dec
            import rocpydecode.decTypes as dectypes
            d = dec.decoder(codec=dectypes.rocDecVideoCodec_AVC, device_id=0)
            print(f"{MARK} correct decoder created on {getattr(d.GetGpuInfo(), 'device_name', '?')}", flush=True)
        else:
            import pyRocJpegDecode.decoder as jdec
            count, ready = jdec.initialize_hip()
            print(f"initialize_hip -> devices {count}, ready {ready}", flush=True)
            if not ready:
                print(f"{MARK} clean_error initialize_hip reported no usable device", flush=True)
                return
            jdec.decoder()
            print(f"{MARK} correct decoder created without a GPU", flush=True)
    except Exception as e:  # noqa: BLE001 - the library reported an error
        print(f"{MARK} clean_error {type(e).__name__}: {e}", flush=True)


def main() -> None:
    what = sys.argv[1]
    if len(sys.argv) > 2 and sys.argv[2] == "--inner":
        inner(what)
        return
    p = subprocess.run([sys.executable, __file__, what, "--inner"], capture_output=True, text=True, timeout=240)
    print(p.stdout, p.stderr, sep="\n", flush=True)
    if p.returncode < 0:
        crashed(f"child killed by signal {-p.returncode}")
    for line in p.stdout.splitlines():
        if line.startswith(MARK):
            outcome, _, detail = line[len(MARK) + 1:].partition(" ")
            finish("error", outcome, detail)
    finish("error", "process_exit", f"the library terminated the interpreter with exit {p.returncode} instead of "
                                    "raising or returning an error")


if __name__ == "__main__":
    main()
