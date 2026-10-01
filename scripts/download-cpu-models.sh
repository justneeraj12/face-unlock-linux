#!/usr/bin/env bash
set -euo pipefail

model_dir="${MODEL_DIR:-models}"
mode="${1:-download}"

sface_name="face_recognition_sface_2021dec.onnx"
sface_sha256="0ba9fbfa01b5270c96627c4ef784da859931e02f04419c829e83484087c34e79"
sface_revision="47534e27c9851bb1128ccc0102f1145e27f23f98"
sface_url="https://github.com/opencv/opencv_zoo/raw/${sface_revision}/models/face_recognition_sface/${sface_name}"

yunet_2022_name="face_detection_yunet_2022mar.onnx"
yunet_2022_sha256="50ef07f702a31741ca46a4c0d947773b64143b9362780237bf0d427d6c79bab7"
yunet_2022_revision="c97242ce7f2a554e288b50eabd9f5df957e78801"

yunet_2023_name="face_detection_yunet_2023mar.onnx"
yunet_2023_sha256="8f2383e4dd3cfbb4553ea8718107fc0423210dc964f9f4280604804ed2552fa4"
yunet_2023_revision="47534e27c9851bb1128ccc0102f1145e27f23f98"

usage() {
  echo "Usage:"
  echo "  ./scripts/download-cpu-models.sh"
  echo "  ./scripts/download-cpu-models.sh --check"
  echo "  ./scripts/download-cpu-models.sh --select-only"
  echo
  echo "Optional:"
  echo "  MODEL_DIR=/path/to/models"
  echo "  FACE_UNLOCK_OPENCV_VERSION=4.10.0"
  echo "  FACE_UNLOCK_YUNET_VARIANT=2022mar|2023mar"
}

if [[ "$mode" == "--help" || "$mode" == "-h" ]]; then
  usage
  exit 0
fi

if [[ "$mode" != "download" && "$mode" != "--check" &&
      "$mode" != "--select-only" ]]; then
  usage
  exit 1
fi

opencv_version="${FACE_UNLOCK_OPENCV_VERSION:-}"
if [[ -z "$opencv_version" ]] && command -v pkg-config >/dev/null 2>&1; then
  opencv_version="$(pkg-config --modversion opencv4 2>/dev/null || true)"
fi
if [[ -z "$opencv_version" ]] && command -v python3 >/dev/null 2>&1; then
  opencv_version="$(python3 -c 'import cv2; print(cv2.__version__)' 2>/dev/null || true)"
fi

yunet_variant="${FACE_UNLOCK_YUNET_VARIANT:-}"
if [[ -z "$yunet_variant" ]]; then
  if [[ "$opencv_version" =~ ^4\.([0-9]+) ]]; then
    opencv_minor="${BASH_REMATCH[1]}"
    if (( opencv_minor <= 6 )); then
      yunet_variant="2022mar"
    else
      yunet_variant="2023mar"
    fi
  else
    yunet_variant="2023mar"
  fi
fi

case "$yunet_variant" in
  2022mar)
    yunet_name="$yunet_2022_name"
    yunet_sha256="$yunet_2022_sha256"
    yunet_revision="$yunet_2022_revision"
    ;;
  2023mar)
    yunet_name="$yunet_2023_name"
    yunet_sha256="$yunet_2023_sha256"
    yunet_revision="$yunet_2023_revision"
    ;;
  *)
    echo "ERROR: FACE_UNLOCK_YUNET_VARIANT must be 2022mar or 2023mar"
    exit 1
    ;;
esac

yunet_url="https://github.com/opencv/opencv_zoo/raw/${yunet_revision}/models/face_detection_yunet/${yunet_name}"

if [[ "$mode" == "--select-only" ]]; then
  echo "opencv_version: ${opencv_version:-unknown}"
  echo "yunet_variant: $yunet_variant"
  echo "yunet_model_name: $yunet_name"
  echo "yunet_revision: $yunet_revision"
  echo "model_selection_status: ok"
  exit 0
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
    echo "expected: $expected"
    echo "actual:   $actual"
    return 1
  fi

  install -m 0644 "$temporary" "$path"
  echo "${label}_model_status: downloaded_verified"
}

mkdir -p "$model_dir"
yunet_versioned_path="$model_dir/$yunet_name"
yunet_path="$model_dir/face_detection_yunet.onnx"
sface_path="$model_dir/$sface_name"

if [[ "$mode" == "download" ]]; then
  if ! verify_model "$yunet_versioned_path" "$yunet_sha256" "yunet" >/dev/null 2>&1; then
    download_model "$yunet_url" "$yunet_versioned_path" "$yunet_sha256" "yunet"
  fi

  if ! verify_model "$sface_path" "$sface_sha256" "sface" >/dev/null 2>&1; then
    download_model "$sface_url" "$sface_path" "$sface_sha256" "sface"
  fi

  ln -sfn "$yunet_name" "$yunet_path"
fi

if [[ ! -L "$yunet_path" || "$(readlink "$yunet_path")" != "$yunet_name" ]]; then
  echo "ERROR: compatible YuNet link missing or stale: $yunet_path"
  echo "Run: ./scripts/download-cpu-models.sh"
  exit 1
fi

verify_model "$yunet_path" "$yunet_sha256" "yunet"
verify_model "$sface_path" "$sface_sha256" "sface"

echo "opencv_version: ${opencv_version:-unknown}"
echo "yunet_variant: $yunet_variant"
echo "yunet_revision: $yunet_revision"
echo "cpu_model_status: ok"
echo "license_note: YuNet MIT; SFace directory Apache-2.0; complete provenance review before release bundling"
