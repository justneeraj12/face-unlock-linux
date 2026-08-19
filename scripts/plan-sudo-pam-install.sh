#!/usr/bin/env bash
set -euo pipefail

sudo_pam="/etc/pam.d/sudo"
module_line="auth sufficient pam_face_unlock.so timeout_ms=1000"
timestamp="$(date +%Y%m%d-%H%M%S)"
backup_path="/etc/pam.d/sudo.face-unlock-backup.${timestamp}"

echo "[plan-sudo-pam-install] sudo PAM dry-run planner"
echo
echo "This script is READ-ONLY."
echo
echo "It will NOT modify:"
echo "  $sudo_pam"
echo
echo "It will:"
echo "  - inspect $sudo_pam"
echo "  - create a proposed modified copy in /tmp"
echo "  - print a diff"
echo "  - print backup and rollback commands"
echo
echo "Planned PAM line:"
echo "  $module_line"
echo

if [[ ! -f "$sudo_pam" ]]; then
  echo "ERROR: sudo PAM file not found: $sudo_pam"
  exit 1
fi

if [[ ! -r "$sudo_pam" ]]; then
  echo "ERROR: sudo PAM file is not readable: $sudo_pam"
  exit 1
fi

echo "Current sudo PAM file:"
echo "------------------------------------------------------------"
nl -ba "$sudo_pam"
echo "------------------------------------------------------------"
echo

if grep -Fq "pam_face_unlock.so" "$sudo_pam"; then
  echo "pam_face_unlock.so already appears in $sudo_pam:"
  grep -nF "pam_face_unlock.so" "$sudo_pam"
  echo
  echo "No new line should be inserted."
  exit 0
fi

insert_line=""

common_auth_line="$(grep -nE '^[[:space:]]*@include[[:space:]]+common-auth' "$sudo_pam" | head -n 1 | cut -d: -f1 || true)"
pam_unix_line="$(grep -nE 'pam_unix\.so' "$sudo_pam" | head -n 1 | cut -d: -f1 || true)"
auth_line="$(grep -nE '^[[:space:]]*auth[[:space:]]+' "$sudo_pam" | head -n 1 | cut -d: -f1 || true)"

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
  echo
  echo "Expected one of:"
  echo "  @include common-auth"
  echo "  pam_unix.so"
  echo "  an auth line"
  echo
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
echo "Proposed modified file:"
echo "  $proposed"
echo

echo "Diff:"
echo "------------------------------------------------------------"
diff -u "$sudo_pam" "$proposed" || true
echo "------------------------------------------------------------"
echo

echo "If a future apply script is used, it must first run:"
echo "  sudo cp $sudo_pam $backup_path"
echo
echo "Rollback command would be:"
echo "  sudo cp $backup_path $sudo_pam"
echo
echo "Additional safety requirements before applying:"
echo "  - keep a root shell open"
echo "  - verify fake PAM service test works"
echo "  - verify daemon user service is running"
echo "  - verify password fallback works"
echo "  - confirm the exact diff above"
echo "  - require explicit YES confirmation"
echo
echo "This script made no changes."
