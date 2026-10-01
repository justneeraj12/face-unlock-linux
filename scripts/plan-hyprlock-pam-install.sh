#!/usr/bin/env bash
set -euo pipefail

hyprlock_pam="${HYPRLOCK_PAM_PATH:-/etc/pam.d/hyprlock}"
module_line="auth sufficient pam_face_unlock.so timeout_ms=1000"
timestamp="$(date +%Y%m%d-%H%M%S)"
backup_path="${hyprlock_pam}.face-unlock-backup.${timestamp}"

echo "[plan-hyprlock-pam-install] Hyprlock PAM dry-run planner"
echo
echo "This script is READ-ONLY."
echo "Target: $hyprlock_pam"
echo "Planned line: $module_line"
echo

if [[ ! -f "$hyprlock_pam" || ! -r "$hyprlock_pam" ]]; then
  echo "ERROR: readable Hyprlock PAM service not found: $hyprlock_pam"
  exit 1
fi

if grep -Fq "pam_face_unlock.so" "$hyprlock_pam"; then
  echo "pam_face_unlock.so already appears in $hyprlock_pam"
  grep -nF "pam_face_unlock.so" "$hyprlock_pam"
  echo "This script made no changes."
  exit 0
fi

insert_line="$(
  grep -nE '^[[:space:]]*auth[[:space:]]+(include|substack)[[:space:]]+' \
    "$hyprlock_pam" | head -n 1 | cut -d: -f1 || true
)"
if [[ -z "$insert_line" ]]; then
  insert_line="$(
    grep -nE '^[[:space:]]*auth[[:space:]]+' "$hyprlock_pam" |
      head -n 1 | cut -d: -f1 || true
  )"
fi
if [[ -z "$insert_line" ]]; then
  echo "ERROR: no safe auth insertion point found; manual review required"
  exit 1
fi

proposed="$(mktemp /tmp/face-unlock-hyprlock-pam-proposed.XXXXXX)"
awk -v insert_line="$insert_line" -v module_line="$module_line" '
  NR == insert_line {
    print "# face-unlock-linux: optional face auth; password fallback remains"
    print module_line
  }
  { print }
' "$hyprlock_pam" > "$proposed"

echo "Current service:"
echo "------------------------------------------------------------"
nl -ba "$hyprlock_pam"
echo "------------------------------------------------------------"
echo
echo "Proposed file: $proposed"
echo "Insertion line: $insert_line"
echo "Diff:"
echo "------------------------------------------------------------"
diff -u "$hyprlock_pam" "$proposed" || true
echo "------------------------------------------------------------"
echo
echo "Backup before any future apply:"
echo "  sudo cp $hyprlock_pam $backup_path"
echo "Rollback:"
echo "  ./scripts/rollback-hyprlock-pam.sh $backup_path"
echo
echo "Important: Hyprlock 0.9 submits PAM after user input/Enter."
echo "This PAM integration alone does not provide automatic face unlock."
echo "This script made no changes."
