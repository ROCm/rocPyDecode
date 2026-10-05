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

# rocPyDecode / rocPyJpegDecode: shipped tests and samples against the SDK media.
#
# Tests run from a scratch copy laid out like the install tree (tests/ copied,
# samples/ linked next to it), because they locate the samples through
# Path(__file__).resolve().parents[1]. Every script gets a fresh working
# directory and TMPDIR under $VP_WORK and a 300 s timeout.
set -uo pipefail
source "${VP_REPO}/build_tools/lib/vp.sh"
vp_init rocpydecode

if ! vp_tier_ge standard; then
  vp_skip "tier::quick" "rocpydecode runs in the standard tier and above"
  vp_finish
  exit 0
fi

CR="${VP_SUITE_DIR}/checked_run.py"
CHK="${VP_SUITE_DIR}/pyd_checks.py"
T_SCRIPT=300
VID="${ROCM_PATH}/share/rocdecode/video"
IMG="${ROCM_PATH}/share/rocjpeg/images"
H264="${VID}/AMD_driving_virtual_20-H264.264"
H265="${VID}/AMD_driving_virtual_20-H265.265"
MP4="${VID}/AMD_driving_virtual_20-H264.mp4"
export TMPDIR="${VP_WORK}/tmp"
mkdir -p "${TMPDIR}"

have_py() { "${VP_PY}" -c "import $1" >/dev/null 2>&1; }

# fresh_dir <name>: an empty working directory for one script.
fresh_dir() {
  local d="${VP_WORK}/cwd/$1"
  rm -rf "${d}"
  mkdir -p "${d}"
  printf '%s' "${d}"
}

# cr <id> [checked_run options] -- cmd...
cr() {
  local id="$1"; shift
  "${VP_PY}" "${CR}" --id "${id}" --timeout "${T_SCRIPT}" "$@"
}

# ---------------------------------------------------------------------------
# Scratch layout: tests/ copied (not linked, so __file__ resolves here) and
# samples/ linked from the prefix.
# ---------------------------------------------------------------------------
LAYOUT="${VP_WORK}/layout"
rm -rf "${LAYOUT}"
for pkg in rocpydecode rocpyjpegdecode; do
  mkdir -p "${LAYOUT}/${pkg}"
  if [[ -d "${ROCM_PATH}/share/${pkg}/tests" ]]; then
    cp -r "${ROCM_PATH}/share/${pkg}/tests" "${LAYOUT}/${pkg}/tests"
    chmod -R u+w "${LAYOUT}/${pkg}/tests"
  fi
  [[ -d "${ROCM_PATH}/share/${pkg}/samples" ]] && ln -s "${ROCM_PATH}/share/${pkg}/samples" "${LAYOUT}/${pkg}/samples"
done
VT="${LAYOUT}/rocpydecode/tests"
VS="${LAYOUT}/rocpydecode/samples/rocdecode"
JT="${LAYOUT}/rocpyjpegdecode/tests"
JS="${LAYOUT}/rocpyjpegdecode/samples/rocjpeg"

# N1: the regression tests call ../samples/...; the tarball layout must provide them.
cr "layout::samples-relative-path" -- "${VP_PY}" "${CHK}" samples-layout \
  "${ROCM_PATH}/share/rocpydecode" "${ROCM_PATH}/share/rocpyjpegdecode"
# The sample READMEs point at ../../README.md, which is not shipped (M3 docs gap).
cr "layout::sample-readme-links" -- "${VP_PY}" "${CHK}" readme-links \
  "${ROCM_PATH}/share/rocpydecode" "${ROCM_PATH}/share/rocpyjpegdecode"

# ---------------------------------------------------------------------------
# Imports and compiled-in API surface
# ---------------------------------------------------------------------------
for m in rocpydecode rocpyjpegdecode pyRocVideoDecode.decoder pyRocVideoDecode.decodercpu \
         pyRocVideoDecode.demuxer pyRocVideoDecode.types pyRocJpegDecode.decoder pyRocJpegDecode.types; do
  vp_run "import::${m}" --timeout 60 -- "${VP_PY}" -c "import ${m}"
done
# H2: built without FFmpeg (no demuxer) and without the rocDecode host backend.
cr "api::ffmpeg-demuxer-bindings" -- "${VP_PY}" "${CHK}" api-ffmpeg
cr "api::host-backend-bindings" -- "${VP_PY}" "${CHK}" api-host

# ---------------------------------------------------------------------------
# Video (upstream CTest set, plus the optional API tests)
# ---------------------------------------------------------------------------
media_ok=1
for f in "${H264}" "${H265}" "${MP4}"; do [[ -f "${f}" ]] || media_ok=0; done

cr "video::types_test" --cwd "$(fresh_dir types_test)" --require "^rocPyDecode types" \
  -- "${VP_PY}" "${VT}/types_test.py"

if [[ "${media_ok}" == 1 ]]; then
  DECODED='info: Decoded [1-9][0-9]* frames'
  cr "video::videodecoderaw.h264" --backend GPU --cwd "$(fresh_dir raw264)" --require "${DECODED}" \
    -- "${VP_PY}" "${VS}/videodecoderaw.py" -i "${H264}"
  cr "video::videodecoderaw.h265" --backend GPU --cwd "$(fresh_dir raw265)" --require "${DECODED}" \
    -- "${VP_PY}" "${VS}/videodecoderaw.py" -i "${H265}" --codec h265
  cr "video::raw_regression_test" --backend GPU --cwd "$(fresh_dir raw_regression)" \
    --require "Raw video frame-limit and empty-input regressions passed" \
    -- "${VP_PY}" "${VT}/raw_regression_test.py" --media-dir "${VID}"
  # Debug/host API test: exit 77 when the debug APIs are not built (upstream SKIP_RETURN_CODE).
  cr "video::decoder_api_test" --backend GPU --skip-rc 77 --cwd "$(fresh_dir decoder_api)" \
    -- "${VP_PY}" "${VT}/decoder_api_test.py" -i "${MP4}"
  # H2: every test that demuxes an MP4 needs FFmpeg; decodercpu also needs the host backend.
  for t in demuxer_test decoder_test decoder_rgb_dlpack_test decodercpu_test; do
    cr "video::${t}" --backend GPU --cwd "$(fresh_dir "${t}")" --why "needs the FFmpeg demuxer (H2)" \
      -- "${VP_PY}" "${VT}/${t}.py" -i "${MP4}"
  done
  # Samples. Demuxer and CPU-backend samples are expected to fail (H2), never skipped.
  for s in videodecode videodecodemem videodecodergb videodecode_cpu_backend; do
    cr "samples::${s}" --backend GPU --cwd "$(fresh_dir "sample_${s}")" --why "needs the FFmpeg demuxer (H2)" \
      -- "${VP_PY}" "${VS}/${s}.py" -i "${MP4}"
  done
  if have_py torch; then
    cr "samples::videodecodetorch_cpu_backend" --cwd "$(fresh_dir sample_torch_cpu)" \
      --why "needs the FFmpeg demuxer and host backend (H2)" \
      -- "${VP_PY}" "${VS}/videodecodetorch_cpu_backend.py" -i "${MP4}"
  else
    vp_blocked "samples::videodecodetorch_cpu_backend" "torch is not installed"
  fi
  for s in videodecodetorch videodecodetorch_yuv videodecodetorch_resnet50; do
    if [[ "${VP_EXTENDED}" == 1 ]] && have_py torch; then
      cr "samples::${s}" --backend GPU --cwd "$(fresh_dir "sample_${s}")" --why "needs the FFmpeg demuxer (H2)" \
        -- "${VP_PY}" "${VS}/${s}.py" -i "${MP4}"
    else
      vp_blocked "samples::${s}" "needs ROCm torch (extended image)"
    fi
  done
  if have_py hip; then
    cr "samples::videodecodeperf" --backend GPU --cwd "$(fresh_dir sample_perf)" --why "needs the FFmpeg demuxer (H2)" \
      -- "${VP_PY}" "${VS}/videodecodeperf.py" -i "${MP4}"
  else
    vp_blocked "samples::videodecodeperf" "hip-python (module hip) is not installed"
  fi
  if have_py torch && have_py mpi4py && have_py av && command -v mpiexec >/dev/null 2>&1; then
    cr "samples::videodecode_mpi" --backend GPU --cwd "$(fresh_dir sample_mpi)" --why "needs the FFmpeg demuxer (H2)" \
      -- mpiexec -n 1 "${VP_PY}" "${VS}/videodecode_mpi.py" -i "${MP4}"
  else
    vp_blocked "samples::videodecode_mpi" "needs torch, mpi4py, av and mpiexec"
  fi
else
  for id in video::videodecoderaw.h264 video::videodecoderaw.h265 video::raw_regression_test \
            video::decoder_api_test video::demuxer_test video::decoder_test video::decoder_rgb_dlpack_test \
            video::decodercpu_test samples::videodecode samples::videodecodemem samples::videodecodergb \
            samples::videodecode_cpu_backend samples::videodecodetorch_cpu_backend samples::videodecodetorch \
            samples::videodecodetorch_yuv samples::videodecodetorch_resnet50 samples::videodecodeperf \
            samples::videodecode_mpi; do
    vp_blocked "${id}" "SDK video media missing under ${VID}"
  done
fi

# ---------------------------------------------------------------------------
# rocPyJpegDecode
# ---------------------------------------------------------------------------
if compgen -G "${IMG}/*.jpg" >/dev/null; then
  for fmt in 3 4; do
    cr "jpeg::jpegdecodebatched.fmt${fmt}" --backend GPU --cwd "$(fresh_dir "jpegbatched${fmt}")" \
      --require 'Total files processed : [1-9]' --require 'Total Bad files found : 0$' \
      -- "${VP_PY}" "${JS}/jpegdecodebatched.py" -i "${IMG}" -fmt "${fmt}"
  done
  cr "jpeg::jpeg_regression_test" --backend GPU --cwd "$(fresh_dir jpeg_regression)" \
    --require "JPEG RGB-layout and batch-error regressions passed" \
    -- "${VP_PY}" "${JT}/jpeg_regression_test.py" --media-dir "${IMG}"
  if vp_require_py "jpeg::jpeg_input_test" numpy; then
    # Runs itself twice: once with NumPy and once in a -S interpreter without it.
    cr "jpeg::jpeg_input_test" --backend GPU --cwd "$(fresh_dir jpeg_input)" \
      --require "NumPy available: False" --require "NumPy available: True" \
      -- "${VP_PY}" "${JT}/jpeg_input_test.py" --media-dir "${IMG}"
  fi
  if [[ "${VP_EXTENDED}" == 1 ]] && have_py torch; then
    cr "jpeg::jpeg_tensor_test" --backend GPU --cwd "$(fresh_dir jpeg_tensor)" --require "Pixel equality passed" \
      -- "${VP_PY}" "${JT}/jpeg_tensor_test.py" --media-dir "${IMG}"
  else
    vp_blocked "jpeg::jpeg_tensor_test" "needs ROCm torch (extended image)"
  fi
else
  for id in jpeg::jpegdecodebatched.fmt3 jpeg::jpegdecodebatched.fmt4 jpeg::jpeg_regression_test \
            jpeg::jpeg_input_test jpeg::jpeg_tensor_test; do
    vp_blocked "${id}" "SDK JPEG media missing under ${IMG}"
  done
fi

vp_finish
exit 0
