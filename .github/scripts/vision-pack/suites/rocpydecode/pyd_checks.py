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

"""Static checks of the installed rocPyDecode / rocPyJpegDecode trees. Exit 0 = pass, 1 = fail.

    pyd_checks.py samples-layout <share_dir>...   every "samples/..." file a shipped test runs exists
    pyd_checks.py readme-links   <share_dir>...   every relative link in the shipped READMEs resolves
    pyd_checks.py api-ffmpeg                      FFmpeg-backed demuxer bindings are compiled in (H2)
    pyd_checks.py api-host                        the rocDecode host (CPU) decoder binding is compiled in (H2)
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

SAMPLE_REF = re.compile(r"""samples/[A-Za-z0-9_./-]+\.py""")
MD_LINK = re.compile(r"\]\(([^)#\s]+)(?:#[^)]*)?\)")


def samples_layout(shares: list[str]) -> int:
    refs, missing = 0, []
    for share in map(Path, shares):
        for test in sorted((share / "tests").glob("*.py")):
            for ref in sorted(set(SAMPLE_REF.findall(test.read_text(errors="replace")))):
                refs += 1
                # The tests resolve the sample as Path(__file__).resolve().parents[1] / ref.
                target = share / ref
                state = "ok" if target.is_file() else "MISSING"
                print(f"{test.relative_to(share.parent)} -> ../{ref}: {state}")
                if state != "ok":
                    missing.append(f"{share.name}/{ref}")
    if refs == 0:
        print("no samples/... references found in the shipped tests (layout check has nothing to verify)")
        return 1
    if missing:
        print(f"FAIL: {len(missing)} of {refs} referenced samples are absent from the prefix: {', '.join(missing)}")
        return 1
    print(f"PASS: all {refs} sample references resolve")
    return 0


def readme_links(shares: list[str]) -> int:
    bad, total = [], 0
    for share in map(Path, shares):
        for md in sorted(share.rglob("*.md")):
            for link in MD_LINK.findall(md.read_text(errors="replace")):
                if "://" in link or link.startswith("mailto:"):
                    continue
                total += 1
                if not (md.parent / link).exists():
                    bad.append(f"{md.relative_to(share.parent)} -> {link}")
    for b in bad:
        print(f"broken link: {b}")
    print(f"{'FAIL' if bad else 'PASS'}: {len(bad)} of {total} relative README links do not resolve")
    return 1 if bad else 0


def api(names: list[str], what: str) -> int:
    import rocpydecode

    exported = sorted(n for n in dir(rocpydecode) if not n.startswith("_"))
    print(f"rocpydecode exports {len(exported)} names: {exported}")
    absent = [n for n in names if not hasattr(rocpydecode, n)]
    if absent:
        print(f"FAIL: {what} bindings absent: {', '.join(absent)}")
        return 1
    print(f"PASS: {what} bindings present")
    return 0


def main(argv: list[str]) -> int:
    if not argv:
        print(__doc__)
        return 2
    cmd, args = argv[0], argv[1:]
    if cmd == "samples-layout":
        return samples_layout(args)
    if cmd == "readme-links":
        return readme_links(args)
    if cmd == "api-ffmpeg":
        return api(["PyVideoDemuxer", "PyFileStreamProvider", "AVCodecString2RocDecVideoCodec",
                    "AVCodec2RocDecVideoCodec"], "FFmpeg demuxer")
    if cmd == "api-host":
        return api(["PyRocVideoDecoderCpu"], "rocDecode host backend")
    print(f"unknown command {cmd}")
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
