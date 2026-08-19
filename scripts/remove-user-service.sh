#!/usr/bin/env bash
set -euo pipefail

daemon_dst="$HOME/.local/bin/face-unlockd"
service_dst="$HOME/.config/systemd/user/face-unlockd.service"
socket_path="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/face-unlock.sock"

echo "[remove-user-service] Removing Face Unlock user service"
echo
echo "This script will:"
echo "  - stop and disable face-unlockd.service"
echo "  - remove:"
echo "      $service_dst"
echo "      $daemon_dst"
echo
echo "This script will NOT modify any PAM files."
echo

systemctl --user disable --now face-unlockd.service 2>/dev/null || true
systemctl --user daemon-reload

rm -f "$service_dst"
rm -f "$daemon_dst"
rm -f "$socket_path"

systemctl --user daemon-reload

echo "Verifying removal..."

if systemctl --user is-active --quiet face-unlockd.service; then
  echo "ERROR: service is still active"
  exit 1
fi

if [[ -e "$service_dst" ]]; then
  echo "ERROR: service file still exists: $service_dst"
  exit 1
fi

if [[ -e "$daemon_dst" ]]; then
  echo "ERROR: daemon binary still exists: $daemon_dst"
  exit 1
fi

echo "user service removed"
echo "daemon binary removed"
echo "socket removed if it existed"
