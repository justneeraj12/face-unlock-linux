#!/usr/bin/env bash
set -euo pipefail

echo "[setup-dev] face-unlock-linux development setup"
echo
echo "This script installs development packages only."
echo
echo "It will NOT:"
echo "  - modify /etc/pam.d/*"
echo "  - install pam_face_unlock.so"
echo "  - enable any systemd service"
echo "  - change sudo/login/lock-screen authentication"
echo

if [[ "${EUID}" -eq 0 ]]; then
  echo "ERROR: do not run this script as root."
  echo "Run it as your normal user. It will use sudo for apt."
  exit 1
fi

read -r -p "Continue installing development packages? Type YES: " answer

if [[ "$answer" != "YES" ]]; then
  echo "Aborted."
  exit 1
fi

sudo apt update

sudo apt install -y \
  build-essential \
  cmake \
  pkg-config \
  git \
  libopencv-dev \
  python3-opencv \
  libpam0g-dev \
  libsodium-dev \
  pamtester \
  v4l-utils

echo
echo "[setup-dev] Installed packages."
echo

echo "[setup-dev] Versions:"
echo -n "  cmake: "
cmake --version | head -n 1 || true

echo -n "  opencv4 pkg-config: "
pkg-config --modversion opencv4 || true

echo -n "  python cv2: "
python3 -c 'import cv2; print(cv2.__version__)' || true

echo
echo "[setup-dev] Camera devices:"
v4l2-ctl --list-devices || true

echo
echo "[setup-dev] Next commands:"
echo "  ./scripts/check-docs.sh"
echo "  ./scripts/build.sh"
echo "  ./build/daemon/face-unlockd --camera 0"
echo
echo "[setup-dev] Done."
