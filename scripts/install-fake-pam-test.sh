#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

module_src="$repo_root/build/pam/pam_face_unlock.so"
module_dst="/usr/lib/x86_64-linux-gnu/security/pam_face_unlock.so"
service_path="/etc/pam.d/face-unlock-test"

echo "[install-fake-pam-test] Safe fake PAM test installer"
echo
echo "This script will:"
echo "  - install pam_face_unlock.so to:"
echo "      $module_dst"
echo "  - create fake PAM service:"
echo "      $service_path"
echo
echo "This script will NOT modify:"
echo "  - /etc/pam.d/sudo"
echo "  - /etc/pam.d/gdm-password"
echo "  - /etc/pam.d/sddm"
echo "  - /etc/pam.d/lightdm"
echo "  - /etc/pam.d/common-auth"
echo
echo "Rollback:"
echo "  sudo rm -f $service_path"
echo "  sudo rm -f $module_dst"
echo
echo "Or run:"
echo "  ./scripts/remove-fake-pam-test.sh"
echo

if [[ ! -f "$module_src" ]]; then
  echo "ERROR: module not found: $module_src"
  echo "Build first:"
  echo "  ./scripts/build.sh"
  exit 1
fi

echo "Dependency audit for PAM module:"
ldd "$module_src"
echo

if ldd "$module_src" | grep -Ei 'opencv|torch|cuda|cudart|Qt|tensorflow'; then
  echo "ERROR: PAM module links to a heavy/disallowed dependency."
  echo "Refusing to install."
  exit 1
fi

echo "Dependency audit passed."
echo

read -r -p "Continue installing fake PAM test service? Type YES: " answer

if [[ "$answer" != "YES" ]]; then
  echo "Aborted."
  exit 1
fi

sudo install -m 0644 "$module_src" "$module_dst"

sudo tee "$service_path" >/dev/null <<'PAM_EOF'
auth sufficient pam_face_unlock.so timeout_ms=1000 debug
auth required pam_unix.so
account required pam_unix.so
PAM_EOF

echo
echo "[install-fake-pam-test] Installed fake PAM service."
echo
echo "Installed module:"
ls -l "$module_dst"
echo
echo "Fake service:"
cat "$service_path"
echo
echo "Test fail-closed behavior:"
echo "  ./build/daemon/face-unlockd --camera 0 --daemon"
echo "  pamtester face-unlock-test \"\$USER\" authenticate"
echo
echo "Test dev-only success:"
echo "  FACE_UNLOCK_DEV_ALLOW=1 ./build/daemon/face-unlockd --camera 0 --daemon"
echo "  pamtester face-unlock-test \"\$USER\" authenticate"
echo
echo "Rollback:"
echo "  ./scripts/remove-fake-pam-test.sh"
