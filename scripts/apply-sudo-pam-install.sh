#!/usr/bin/env bash
set -euo pipefail

sudo_pam="/etc/pam.d/sudo"
module_path="/usr/lib/x86_64-linux-gnu/security/pam_face_unlock.so"
module_line="auth sufficient pam_face_unlock.so timeout_ms=1000"
timestamp="$(date +%Y%m%d-%H%M%S)"
backup_path="/etc/pam.d/sudo.face-unlock-backup.${timestamp}"
apply="false"

usage() {
  echo "Usage:"
  echo "  ./scripts/apply-sudo-pam-install.sh"
  echo "  ./scripts/apply-sudo-pam-install.sh --apply"
  echo
  echo "Default mode is dry-run only."
}

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  usage
  exit 0
fi

if [[ "${1:-}" == "--apply" ]]; then
  apply="true"
elif [[ $# -gt 0 ]]; then
  usage
  exit 1
fi

echo "[apply-sudo-pam-install] guarded sudo PAM installer"
echo
echo "Mode:"
if [[ "$apply" == "true" ]]; then
  echo "  APPLY"
else
  echo "  DRY RUN"
fi
echo
echo "This script targets only:"
echo "  $sudo_pam"
echo
echo "It will NOT modify:"
echo "  /etc/pam.d/gdm-password"
echo "  /etc/pam.d/sddm"
echo "  /etc/pam.d/lightdm"
echo "  /etc/pam.d/common-auth"
echo
echo "Planned PAM line:"
echo "  $module_line"
echo

if [[ ! -f "$sudo_pam" ]]; then
  echo "ERROR: sudo PAM file not found: $sudo_pam"
  exit 1
fi

if grep -Fq "pam_face_unlock.so" "$sudo_pam"; then
  echo "pam_face_unlock.so already appears in $sudo_pam:"
  grep -nF "pam_face_unlock.so" "$sudo_pam"
  echo
  echo "Refusing to add a duplicate line."
  exit 1
fi

common_auth_line="$(grep -nE '^[[:space:]]*@include[[:space:]]+common-auth' "$sudo_pam" | head -n 1 | cut -d: -f1 || true)"
pam_unix_line="$(grep -nE 'pam_unix\.so' "$sudo_pam" | head -n 1 | cut -d: -f1 || true)"
auth_line="$(grep -nE '^[[:space:]]*auth[[:space:]]+' "$sudo_pam" | head -n 1 | cut -d: -f1 || true)"

insert_line=""
reason=""

if [[ -n "$common_auth_line" ]]; then
  insert_line="$common_auth_line"
  reason="before @include common-auth"
elif [[ -n "$pam_unix_line" ]]; then
  insert_line="$pam_unix_line"
  reason="before pam_unix.so"
elif [[ -n "$auth_line" ]]; then
  insert_line="$auth_line"
  reason="before first auth line"
else
  echo "ERROR: could not find a safe insertion point."
  echo "Manual review required."
  exit 1
fi

proposed="$(mktemp /tmp/face-unlock-sudo-pam-proposed.XXXXXX)"

awk -v insert_line="$insert_line" -v module_line="$module_line" '
  NR == insert_line {
    print "# face-unlock-linux: optional face auth, password fallback remains available"
    print module_line
  }
  { print }
' "$sudo_pam" > "$proposed"

echo "Suggested insertion point:"
echo "  line: $insert_line"
echo "  reason: $reason"
echo
echo "Proposed file:"
echo "  $proposed"
echo

echo "Diff:"
echo "------------------------------------------------------------"
diff -u "$sudo_pam" "$proposed" || true
echo "------------------------------------------------------------"
echo

echo "Backup command that will be run before applying:"
echo "  sudo cp $sudo_pam $backup_path"
echo
echo "Rollback command:"
echo "  sudo cp $backup_path $sudo_pam"
echo
echo "Or use:"
echo "  ./scripts/rollback-sudo-pam.sh $backup_path"
echo

if [[ "$apply" != "true" ]]; then
  echo "DRY RUN ONLY. No changes made."
  echo
  echo "To apply, rerun with:"
  echo "  ./scripts/apply-sudo-pam-install.sh --apply"
  exit 0
fi

echo "Preflight checks for APPLY mode..."
echo

if [[ ! -f "$module_path" ]]; then
  echo "ERROR: PAM module is not installed:"
  echo "  $module_path"
  echo
  echo "Install package or install module before applying sudo integration."
  echo "For development fake test only, see:"
  echo "  ./scripts/install-fake-pam-test.sh"
  exit 1
fi

if [[ ! -S "${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/face-unlock.sock" ]]; then
  echo "ERROR: daemon socket not found:"
  echo "  ${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/face-unlock.sock"
  echo
  echo "Start the user daemon first:"
  echo "  ./scripts/install-user-service.sh"
  echo
  echo "or manually:"
  echo "  ./build/daemon/face-unlockd --camera 0 --daemon"
  exit 1
fi

echo "WARNING:"
echo "  Incorrect sudo PAM configuration can lock you out of sudo."
echo
echo "Before continuing:"
echo "  1. Open a separate terminal."
echo "  2. Run: sudo -v"
echo "  3. Keep that root-authenticated terminal open."
echo "  4. Confirm fake PAM service testing has already passed."
echo
echo "Also note:"
echo "  Current sudo face-auth success may require future daemon peer-policy support"
echo "  for root-owned PAM clients. If not supported, sudo should fall back to password."
echo

read -r -p "Type I_HAVE_A_ROOT_SHELL to confirm recovery shell is open: " root_answer

if [[ "$root_answer" != "I_HAVE_A_ROOT_SHELL" ]]; then
  echo "Aborted."
  exit 1
fi

read -r -p "Type FAKE_PAM_TEST_PASSED to confirm fake PAM test passed: " fake_answer

if [[ "$fake_answer" != "FAKE_PAM_TEST_PASSED" ]]; then
  echo "Aborted."
  exit 1
fi

read -r -p "Type APPLY_SUDO_PAM_CHANGE to modify /etc/pam.d/sudo: " apply_answer

if [[ "$apply_answer" != "APPLY_SUDO_PAM_CHANGE" ]]; then
  echo "Aborted."
  exit 1
fi

echo
echo "Creating backup:"
echo "  $backup_path"
sudo cp "$sudo_pam" "$backup_path"

echo "Installing proposed sudo PAM file..."
sudo install -m 0644 -o root -g root "$proposed" "$sudo_pam"

echo
echo "Verifying installed line..."
if ! grep -Fq "pam_face_unlock.so" "$sudo_pam"; then
  echo "ERROR: pam_face_unlock.so line not found after install."
  echo "Attempt rollback:"
  echo "  sudo cp $backup_path $sudo_pam"
  exit 1
fi

echo
echo "sudo PAM change applied."
echo
echo "Rollback command:"
echo "  sudo cp $backup_path $sudo_pam"
echo
echo "or:"
echo "  ./scripts/rollback-sudo-pam.sh $backup_path"
echo
echo "Test sudo carefully in a NEW terminal while keeping your recovery shell open:"
echo "  sudo -k"
echo "  sudo true"
echo
echo "If anything looks wrong, rollback immediately."
