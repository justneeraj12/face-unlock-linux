#!/usr/bin/env bash
set -euo pipefail

camera_index="0"
iterations="20"
if [[ "${1:-}" == "--camera" && -n "${2:-}" ]]; then
  camera_index="$2"
  shift 2
fi
if [[ "${1:-}" == "--iterations" && -n "${2:-}" ]]; then
  iterations="$2"
  shift 2
fi
if [[ $# -ne 0 || ! "$camera_index" =~ ^[0-9]+$ || \
      ! "$iterations" =~ ^[1-9][0-9]*$ ]]; then
  echo "Usage: $0 [--camera INDEX] [--iterations N]"
  exit 2
fi

yunet_model="models/face_detection_yunet.onnx"
sface_model="models/face_recognition_sface_2021dec.onnx"
binary="./build/daemon/face-unlock-native-verification-benchmark"

for required in "$binary" "$yunet_model" "$sface_model"; do
  if [[ ! -f "$required" ]]; then
    echo "ERROR: required file missing: $required"
    echo "Run ./scripts/build.sh and ./scripts/download-cpu-models.sh first."
    exit 1
  fi
done

"$binary" \
  --yunet-model "$yunet_model" \
  --sface-model "$sface_model" \
  --camera "$camera_index" \
  --iterations "$iterations"
