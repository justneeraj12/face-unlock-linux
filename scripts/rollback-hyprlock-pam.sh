#!/usr/bin/env bash
set -euo pipefail

hyprlock_pam="/etc/pam.d/hyprlock"
backup="${1:-}"

usage() {
  echo "Usage:"
  echo "  ./scripts/rollback-hyprlock-pam.sh /etc/pam.d/hyprlock.face-unlock-backup.TIMESTAMP"
  echo "  ./scripts/rollback-hyprlock-pam.sh --latest"
}

if [[ -z "$backup" || "$backup" == "--help" || "$backup" == "-h" ]]; then
  usage
  exit 1
fi
if [[ "$backup" == "--latest" ]]; then
  backup="$(
    sudo find /etc/pam.d -maxdepth 1 -type f \
      -name 'hyprlock.face-unlock-backup.*' -printf '%T@ %p\n' 2>/dev/null |
      sort -nr | head -n 1 | cut -d' ' -f2- || true
  )"
fi
if [[ -z "$backup" || ! -f "$backup" ]]; then
  echo "ERROR: Hyprlock backup not found: ${backup:-none}"
  exit 1
fi
case "$backup" in
  /etc/pam.d/hyprlock.face-unlock-backup.*) ;;
  *) echo "ERROR: refusing unrelated backup path: $backup"; exit 1 ;;
esac

echo "Restore $backup to $hyprlock_pam"
sudo nl -ba "$backup"
read -r -p "Type ROLLBACK_HYPRLOCK_PAM: " answer
if [[ "$answer" != "ROLLBACK_HYPRLOCK_PAM" ]]; then
  echo "Aborted."
  exit 1
fi
sudo install -m 0644 -o root -g root "$backup" "$hyprlock_pam"
echo "Hyprlock PAM rollback applied."
