#!/usr/bin/env bash
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

# Robustness (gpu_access: all), rocPyDecode checks: exit-code honesty.
#
# rocPyDecode has no unsupported-GPU probe, so this copy enumerates no GPU
# agents and records no unsupported-gpu::* results. Its one check ends at a
# shipped test's argument parsing, before any decoder is created.
set -uo pipefail
source "${VP_REPO}/build_tools/lib/vp.sh"
vp_init robustness

if ! vp_tier_ge comprehensive; then
  vp_skip "tier::below-comprehensive" "robustness runs in the comprehensive tier and above"
  vp_finish
  exit 0
fi

CR="${VP_REPO}/suites/robustness/checked_run.py"
export TMPDIR="${VP_WORK}/tmp"
mkdir -p "${TMPDIR}"

fresh_dir() {
  local d="${VP_WORK}/cwd/$1"
  rm -rf "${d}"
  mkdir -p "${d}"
  printf '%s' "${d}"
}

# cr <id> [checked_run options] -- cmd... ; probes report a crashed child with exit 70.
cr() {
  local id="$1"; shift
  "${VP_PY}" "${CR}" --id "${id}" --timeout 300 --error-rc 70 "$@"
}

# ---------------------------------------------------------------------------
# Exit-code honesty: controlled failing cases that must end with a non-zero status.
# ---------------------------------------------------------------------------
# rocPyDecode's decoder_test.py catches its own argument error and calls exit() (status 0).
cr "exit-code::rocpydecode-decoder-test-bad-args-exit0" --expect-nonzero --require "Error:" \
  --cwd "$(fresh_dir decoder_test_args)" -- "${VP_PY}" "${ROCM_PATH}/share/rocpydecode/tests/decoder_test.py"

vp_finish
exit 0
