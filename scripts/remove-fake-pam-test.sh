#!/usr/bin/env bash
set -euo pipefail

module_dst="/usr/lib/x86_64-linux-gnu/security/pam_face_unlock.so"
service_path="/etc/pam.d/face-unlock-test"

echo "[remove-fake-pam-test] Removing fake PAM test artifacts"
echo
echo "This script removes:"
echo "  - $service_path"
echo "  - $module_dst"
echo
echo "This script does NOT modify:"
echo "  - /etc/pam.d/sudo"
echo "  - /etc/pam.d/gdm-password"
echo "  - /etc/pam.d/sddm"
echo "  - /etc/pam.d/lightdm"
echo "  - /etc/pam.d/common-auth"
echo

sudo rm -f "$service_path"
sudo rm -f "$module_dst"

echo "Verifying removal..."

if [[ -e "$service_path" ]]; then
  echo "ERROR: still exists: $service_path"
  exit 1
fi

if [[ -e "$module_dst" ]]; then
  echo "ERROR: still exists: $module_dst"
  exit 1
fi

echo "fake PAM service removed"
echo "PAM module removed"
