#!/usr/bin/env bash
set -euo pipefail

sudo_pam="/etc/pam.d/sudo"

usage() {
  echo "Usage:"
  echo "  ./scripts/rollback-sudo-pam.sh /etc/pam.d/sudo.face-unlock-backup.TIMESTAMP"
  echo "  ./scripts/rollback-sudo-pam.sh --latest"
}

backup="${1:-}"

if [[ -z "$backup" || "$backup" == "--help" || "$backup" == "-h" ]]; then
  usage
  exit 1
fi

if [[ "$backup" == "--latest" ]]; then
  backup="$(sudo find /etc/pam.d -maxdepth 1 -type f -name 'sudo.face-unlock-backup.*' -printf '%T@ %p\n' 2>/dev/null | sort -nr | head -n 1 | cut -d' ' -f2- || true)"

  if [[ -z "$backup" ]]; then
    echo "ERROR: no sudo.face-unlock-backup.* files found in /etc/pam.d"
    exit 1
  fi
fi

echo "[rollback-sudo-pam] sudo PAM rollback"
echo
echo "This script will restore:"
echo "  $backup"
echo
echo "to:"
echo "  $sudo_pam"
echo
echo "It will NOT modify other PAM files."
echo

if [[ ! -f "$backup" ]]; then
  echo "ERROR: backup file not found:"
  echo "  $backup"
  exit 1
fi

echo "Backup preview:"
echo "------------------------------------------------------------"
sudo nl -ba "$backup"
echo "------------------------------------------------------------"
echo

read -r -p "Type ROLLBACK_SUDO_PAM to restore this backup: " answer

if [[ "$answer" != "ROLLBACK_SUDO_PAM" ]]; then
  echo "Aborted."
  exit 1
fi

sudo install -m 0644 -o root -g root "$backup" "$sudo_pam"

echo
echo "Rollback applied."
echo
echo "Current sudo PAM file:"
echo "------------------------------------------------------------"
sudo nl -ba "$sudo_pam"
echo "------------------------------------------------------------"
echo
echo "Test sudo carefully:"
echo "  sudo -k"
echo "  sudo true"
