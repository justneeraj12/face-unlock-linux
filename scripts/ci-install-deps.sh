#!/usr/bin/env bash
set -euo pipefail

mode="${1:-core}"

usage() {
  echo "Usage:"
  echo "  ./scripts/ci-install-deps.sh core"
  echo "  ./scripts/ci-install-deps.sh gui"
}

if [[ "$mode" == "--help" || "$mode" == "-h" ]]; then
  usage
  exit 0
fi

if [[ "$mode" != "core" && "$mode" != "gui" ]]; then
  echo "ERROR: mode must be core or gui"
  usage
  exit 1
fi

echo "[ci-install-deps] Installing CI dependencies"
echo "mode: $mode"

echo "[ci-install-deps] Runner release:"
grep -E '^(NAME|VERSION|VERSION_CODENAME)=' /etc/os-release || true

sudo apt-get -o Acquire::Retries=5 update

packages=(
  build-essential
  cmake
  ninja-build
  ccache
  pkg-config
  file
  python3-numpy
  python3-opencv
  libopencv-core-dev
  libopencv-dnn-dev
  libopencv-videoio-dev
  libopencv-objdetect-dev
  libopencv-imgproc-dev
  libpam0g-dev
  libsodium-dev
)

if [[ "$mode" == "gui" ]]; then
  packages+=(
    qt6-base-dev
  )
fi

sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends "${packages[@]}"

echo "[ci-install-deps] Installed packages:"
printf '  %s\n' "${packages[@]}"
