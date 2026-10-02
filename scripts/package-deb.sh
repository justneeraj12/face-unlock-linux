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
./scripts/check-json.sh
./scripts/check-scripts.sh

package_build_dir="${PACKAGE_BUILD_DIR:-build-gui}"
GUI_BUILD_DIR="$package_build_dir" ./scripts/build-gui.sh
ctest --test-dir "$package_build_dir" --output-on-failure

cmake --build "$package_build_dir" --target package

AUDIT_BUILD_DIR="$package_build_dir" \
AUDIT_GUI_BUILD_DIR="$package_build_dir" \
AUDIT_PACKAGE_DIR="$package_build_dir" \
  ./scripts/audit-dependencies.sh

echo
echo "[package-deb] Package artifacts:"
find "$package_build_dir" -maxdepth 1 -type f -name "*.deb" \
  -print -exec ls -lh {} \;
echo
echo "Inspect package contents with:"
echo "  dpkg-deb -c $package_build_dir/*.deb"
echo
echo "Install manually only if you understand the package contents:"
echo "  sudo apt install ./$package_build_dir/<package-name>.deb"
echo
echo "Installing the package still does not modify PAM service files."
