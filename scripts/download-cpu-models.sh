#!/usr/bin/env bash
set -euo pipefail

model_dir="${MODEL_DIR:-models}"
mode="${1:-download}"

yunet_name="face_detection_yunet_2022mar.onnx"
sface_name="face_recognition_sface_2021dec.onnx"
yunet_sha256="50ef07f702a31741ca46a4c0d947773b64143b9362780237bf0d427d6c79bab7"
sface_sha256="0ba9fbfa01b5270c96627c4ef784da859931e02f04419c829e83484087c34e79"
yunet_url="https://github.com/opencv/opencv_zoo/raw/c97242ce7f2a554e288b50eabd9f5df957e78801/models/face_detection_yunet/${yunet_name}"
sface_url="https://github.com/opencv/opencv_zoo/raw/47534e27c9851bb1128ccc0102f1145e27f23f98/models/face_recognition_sface/${sface_name}"

usage() {
  echo "Usage:"
  echo "  ./scripts/download-cpu-models.sh"
  echo "  ./scripts/download-cpu-models.sh --check"
  echo
  echo "Optional: MODEL_DIR=/path/to/models"
}

if [[ "$mode" == "--help" || "$mode" == "-h" ]]; then
  usage
  exit 0
fi

if [[ "$mode" != "download" && "$mode" != "--check" ]]; then
  usage
  exit 1
fi

verify_model() {
  local path="$1"
  local expected="$2"
  local label="$3"

  if [[ ! -f "$path" ]]; then
    echo "ERROR: ${label} model missing: $path"
    return 1
  fi

  local actual
  actual="$(sha256sum "$path" | awk '{print $1}')"

  if [[ "$actual" != "$expected" ]]; then
    echo "ERROR: ${label} checksum mismatch: $path"
    echo "expected: $expected"
    echo "actual:   $actual"
    return 1
  fi

  echo "${label}_model_status: verified"
  echo "${label}_model_path: $path"
}

download_model() {
  local url="$1"
  local path="$2"
  local expected="$3"
  local label="$4"
  local temporary
  temporary="$(mktemp "${TMPDIR:-/tmp}/face-unlock-${label}.XXXXXX")"

  cleanup_download() {
    rm -f "$temporary"
  }
  trap cleanup_download RETURN

  echo "Downloading pinned ${label} CPU model"
  curl --fail --location --retry 3 --output "$temporary" "$url"

  local actual
  actual="$(sha256sum "$temporary" | awk '{print $1}')"

  if [[ "$actual" != "$expected" ]]; then
    echo "ERROR: downloaded ${label} checksum mismatch"
    return 1
  fi

  install -m 0644 "$temporary" "$path"
  echo "${label}_model_status: downloaded_verified"
}

mkdir -p "$model_dir"
yunet_path="$model_dir/$yunet_name"
sface_path="$model_dir/$sface_name"

if [[ "$mode" == "download" ]]; then
  if ! verify_model "$yunet_path" "$yunet_sha256" "yunet" >/dev/null 2>&1; then
    download_model "$yunet_url" "$yunet_path" "$yunet_sha256" "yunet"
  fi

  if ! verify_model "$sface_path" "$sface_sha256" "sface" >/dev/null 2>&1; then
    download_model "$sface_url" "$sface_path" "$sface_sha256" "sface"
  fi
fi

verify_model "$yunet_path" "$yunet_sha256" "yunet"
verify_model "$sface_path" "$sface_sha256" "sface"

echo "cpu_model_status: ok"
echo "license_note: YuNet MIT; SFace directory Apache-2.0; complete provenance review before release bundling"
