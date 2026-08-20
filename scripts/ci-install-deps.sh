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

sudo tee /etc/apt/sources.list.d/ubuntu.sources >/dev/null <<'APT_EOF'
Types: deb
URIs: http://archive.ubuntu.com/ubuntu/
Suites: noble noble-updates noble-backports
Components: main restricted universe multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg

Types: deb
URIs: http://security.ubuntu.com/ubuntu/
Suites: noble-security
Components: main restricted universe multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
APT_EOF

sudo apt-get -o Acquire::Retries=5 update

packages=(
  build-essential
  cmake
  ninja-build
  ccache
  pkg-config
  file
  libopencv-core-dev
  libopencv-videoio-dev
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
