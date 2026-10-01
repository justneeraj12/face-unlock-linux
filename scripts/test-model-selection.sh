#!/usr/bin/env bash
set -euo pipefail

downloader="${1:-./scripts/download-cpu-models.sh}"

check_selection() {
  local version="$1"
  local expected_variant="$2"
  local expected_name="$3"
  local output
  output="$(FACE_UNLOCK_OPENCV_VERSION="$version" "$downloader" --select-only)"
  echo "$output"
  grep -Fq "yunet_variant: $expected_variant" <<<"$output"
  grep -Fq "yunet_model_name: $expected_name" <<<"$output"
  grep -Fq "model_selection_status: ok" <<<"$output"
}

check_selection 4.6.0 2022mar face_detection_yunet_2022mar.onnx
check_selection 4.10.0 2023mar face_detection_yunet_2023mar.onnx

forced="$(
  FACE_UNLOCK_OPENCV_VERSION=4.10.0 \
  FACE_UNLOCK_YUNET_VARIANT=2022mar \
  "$downloader" --select-only
)"
grep -Fq "yunet_variant: 2022mar" <<<"$forced"

if FACE_UNLOCK_YUNET_VARIANT=invalid "$downloader" --select-only \
    >/dev/null 2>&1; then
  echo "model_selection_invalid_override_status: failed"
  exit 1
fi

echo "model_selection_opencv_46_status: ok"
echo "model_selection_opencv_410_status: ok"
echo "model_selection_override_status: ok"
echo "model_selection_invalid_override_status: ok"
echo "model_selection_test_status: ok"
