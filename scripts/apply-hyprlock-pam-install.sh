#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
hyprlock_pam="${HYPRLOCK_PAM_PATH:-/etc/pam.d/hyprlock}"
module_path="$($script_dir/pam-module-path.sh)"
module_line="auth sufficient pam_face_unlock.so timeout_ms=1000"
timestamp="$(date +%Y%m%d-%H%M%S)"
backup_path="${hyprlock_pam}.face-unlock-backup.${timestamp}"
apply=false

usage() {
  echo "Usage:"
  echo "  ./scripts/apply-hyprlock-pam-install.sh"
  echo "  ./scripts/apply-hyprlock-pam-install.sh --apply"
  echo
  echo "Default mode is dry-run only."
}

case "${1:-}" in
  "") ;;
  --apply) apply=true ;;
  --help|-h) usage; exit 0 ;;
  *) usage; exit 1 ;;
esac

echo "[apply-hyprlock-pam-install] guarded Hyprlock PAM installer"
echo "Mode: $([[ "$apply" == true ]] && echo APPLY || echo 'DRY RUN')"
echo "Target: $hyprlock_pam"
echo "Module: $module_path"
echo "Planned line: $module_line"
echo
echo "This script never modifies common-auth, sudo, login, greetd, or a display manager."
echo

if [[ ! -f "$hyprlock_pam" || ! -r "$hyprlock_pam" ]]; then
  echo "ERROR: readable Hyprlock PAM service not found: $hyprlock_pam"
  exit 1
fi
if grep -Fq "pam_face_unlock.so" "$hyprlock_pam"; then
  echo "ERROR: refusing to add a duplicate pam_face_unlock.so line"
  exit 1
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

echo "Diff:"
echo "------------------------------------------------------------"
diff -u "$hyprlock_pam" "$proposed" || true
echo "------------------------------------------------------------"
echo "Backup: $backup_path"
echo "Rollback: ./scripts/rollback-hyprlock-pam.sh $backup_path"
echo

if [[ "$apply" != true ]]; then
  echo "DRY RUN ONLY. No changes made."
  exit 0
fi

if [[ "$hyprlock_pam" != "/etc/pam.d/hyprlock" ]]; then
  echo "ERROR: apply mode only permits /etc/pam.d/hyprlock"
  exit 1
fi
if [[ ! -f "$module_path" ]]; then
  echo "ERROR: PAM module is not installed: $module_path"
  exit 1
fi

socket_path="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/face-unlock.sock"
if [[ ! -S "$socket_path" ]]; then
  echo "ERROR: daemon socket not found: $socket_path"
  exit 1
fi

echo "Checking current fail-closed daemon authentication..."
auth_output="$(
  FACE_UNLOCK_SOCKET_PATH="$socket_path" \
    "$script_dir/test-socket-client.sh" auth 2>&1 || true
)"
echo "$auth_output"
if ! grep -Fq '"status":"ok"' <<<"$auth_output"; then
  echo "ERROR: daemon did not return explicit auth success."
  echo "Refusing to modify Hyprlock PAM while biometric authentication is disabled."
  exit 1
fi
if grep -Fq '"reason":"dev_allow_camera_ready"' <<<"$auth_output"; then
  echo "ERROR: development-only authentication success is not acceptable."
  echo "Refusing to modify Hyprlock PAM with FACE_UNLOCK_DEV_ALLOW enabled."
  exit 1
fi

if ! grep -Eq '^[[:space:]]*auth[[:space:]]+(include|substack|.*pam_unix\.so)' \
    "$proposed"; then
  echo "ERROR: proposed service has no recognizable password fallback"
  exit 1
fi

echo "WARNING: Hyprlock 0.9 normally invokes PAM after input/Enter."
echo "This change does not add automatic background submission or illumination."
read -r -p "Type PASSWORD_FALLBACK_CONFIRMED: " fallback_answer
if [[ "$fallback_answer" != "PASSWORD_FALLBACK_CONFIRMED" ]]; then
  echo "Aborted."
  exit 1
fi
read -r -p "Type APPLY_HYPRLOCK_PAM_CHANGE: " apply_answer
if [[ "$apply_answer" != "APPLY_HYPRLOCK_PAM_CHANGE" ]]; then
  echo "Aborted."
  exit 1
fi

sudo cp "$hyprlock_pam" "$backup_path"
sudo install -m 0644 -o root -g root "$proposed" "$hyprlock_pam"

if ! grep -Fq "pam_face_unlock.so" "$hyprlock_pam"; then
  echo "ERROR: installed line missing; rollback immediately with:"
  echo "  ./scripts/rollback-hyprlock-pam.sh $backup_path"
  exit 1
fi

echo "Hyprlock PAM change applied."
echo "Rollback: ./scripts/rollback-hyprlock-pam.sh $backup_path"
