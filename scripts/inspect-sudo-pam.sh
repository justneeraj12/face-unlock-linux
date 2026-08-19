#!/usr/bin/env bash
set -euo pipefail

sudo_pam="/etc/pam.d/sudo"
proposed_line="auth sufficient pam_face_unlock.so timeout_ms=1000"

echo "[inspect-sudo-pam] sudo PAM inspection"
echo
echo "This script is read-only."
echo
echo "It will NOT modify:"
echo "  $sudo_pam"
echo
echo "Planned line:"
echo "  $proposed_line"
echo

if [[ ! -f "$sudo_pam" ]]; then
  echo "ERROR: sudo PAM file not found: $sudo_pam"
  exit 1
fi

echo "Current $sudo_pam:"
echo "------------------------------------------------------------"
nl -ba "$sudo_pam"
echo "------------------------------------------------------------"
echo

if grep -Fq "pam_face_unlock.so" "$sudo_pam"; then
  echo "Existing pam_face_unlock.so line found:"
  grep -nF "pam_face_unlock.so" "$sudo_pam"
  echo
  echo "No duplicate line should be added."
  exit 0
fi

echo "Suggested insertion:"
echo

line_number="$(grep -nE 'pam_unix\.so|common-auth|system-auth' "$sudo_pam" | head -n 1 | cut -d: -f1 || true)"

if [[ -n "$line_number" ]]; then
  echo "Insert before line $line_number:"
  sed -n "${line_number}p" "$sudo_pam" | sed 's/^/  existing: /'
else
  echo "Could not detect pam_unix.so, common-auth, or system-auth."
  echo "A human must review this PAM file before any change."
fi

echo
echo "Proposed line:"
echo "  $proposed_line"
echo
echo "Safety requirements before applying in a future script:"
echo "  - keep a root shell open"
echo "  - create a timestamped backup"
echo "  - print rollback command"
echo "  - require explicit confirmation"
echo "  - verify fake PAM service test first"
echo
echo "This script made no changes."
