#!/usr/bin/env bash
set -euo pipefail
selftest="${1:-./build/daemon/face-unlock-recognizer-selftest}"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
model_path="${2:-$repo_root/models/face_recognition_sface_2021dec.onnx}"

echo "[test-cpu-recognizer] C++ SFace CPU test"
[[ -x "$selftest" ]] || { echo "ERROR: missing self-test: $selftest"; exit 1; }
base_output="$("$selftest")"
echo "$base_output"
[[ "$base_output" == *"supported_recognizer: sface"* ]] || exit 1
[[ "$base_output" == *"cosine_validation_status: ok"* ]] || exit 1
[[ "$base_output" == *"status: ok"* ]] || exit 1

if [[ ! -f "$model_path" ]]; then
  echo "sface_cpp_status: skipped_model_missing"
  echo "status: ok"
  exit 0
fi

model_output="$("$selftest" --sface-model "$model_path" --warmup 2 --iterations 10)"
echo "$model_output"
[[ "$model_output" == *"sface_embedding_dim: 128"* ]] || exit 1
[[ "$model_output" == *"sface_alignment_status: ok"* ]] || exit 1
[[ "$model_output" == *"sface_status: ok"* ]] || exit 1
[[ "$model_output" == *"sface_p50_ms:"* ]] || exit 1
[[ "$model_output" == *"sface_p95_ms:"* ]] || exit 1
echo "sface_cpp_status: ok"
echo "status: ok"
