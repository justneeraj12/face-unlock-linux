#!/usr/bin/env bash
set -euo pipefail

echo "[package-deb] Building Debian package"
echo
echo "This script builds a .deb package using CPack."
echo
echo "It does NOT:"
echo "  - install the package"
echo "  - modify PAM files"
echo "  - enable systemd services"
echo "  - change sudo/login/lock-screen authentication"
echo

./scripts/check-docs.sh
./scripts/build.sh
./scripts/test.sh

cmake --build build --target package

echo
echo "[package-deb] Package artifacts:"
find build -maxdepth 1 -type f -name "*.deb" -print -exec ls -lh {} \;
echo
echo "Inspect package contents with:"
echo "  dpkg-deb -c build/*.deb"
echo
echo "Install manually only if you understand the package contents:"
echo "  sudo apt install ./build/<package-name>.deb"
echo
echo "Installing the package still does not modify PAM service files."
