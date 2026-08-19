#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

daemon_src="$repo_root/build/daemon/face-unlockd"
daemon_dst="$HOME/.local/bin/face-unlockd"

service_src="$repo_root/packaging/systemd/face-unlockd.service"
service_dst="$HOME/.config/systemd/user/face-unlockd.service"

echo "[install-user-service] Face Unlock user service installer"
echo
echo "This script will:"
echo "  - copy daemon binary to:"
echo "      $daemon_dst"
echo "  - install systemd user service to:"
echo "      $service_dst"
echo "  - run:"
echo "      systemctl --user daemon-reload"
echo "      systemctl --user enable --now face-unlockd.service"
echo
echo "This script will NOT modify:"
echo "  - /etc/pam.d/sudo"
echo "  - /etc/pam.d/gdm-password"
echo "  - /etc/pam.d/sddm"
echo "  - /etc/pam.d/lightdm"
echo "  - /etc/pam.d/common-auth"
echo
echo "The service runs with:"
echo "  FACE_UNLOCK_DEV_ALLOW=0"
echo
echo "So auth remains fail-closed by default."
echo
echo "Rollback:"
echo "  ./scripts/remove-user-service.sh"
echo

if [[ ! -x "$daemon_src" ]]; then
  echo "ERROR: daemon not found or not executable:"
  echo "  $daemon_src"
  echo
  echo "Build first:"
  echo "  ./scripts/build.sh"
  exit 1
fi

if [[ ! -f "$service_src" ]]; then
  echo "ERROR: service file not found:"
  echo "  $service_src"
  exit 1
fi

read -r -p "Continue installing and starting user service? Type YES: " answer

if [[ "$answer" != "YES" ]]; then
  echo "Aborted."
  exit 1
fi

mkdir -p "$HOME/.local/bin"
mkdir -p "$HOME/.config/systemd/user"
mkdir -p "$HOME/.local/share/face-unlock"

install -m 0755 "$daemon_src" "$daemon_dst"
install -m 0644 "$service_src" "$service_dst"

systemctl --user daemon-reload
systemctl --user enable --now face-unlockd.service

echo
echo "[install-user-service] Installed and started."
echo
echo "Status:"
systemctl --user --no-pager status face-unlockd.service || true
echo
echo "Socket check:"
if [[ -S "${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/face-unlock.sock" ]]; then
  ls -l "${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/face-unlock.sock"
else
  echo "Socket not found yet. Check service logs:"
  echo "  journalctl --user -u face-unlockd.service -n 100 --no-pager"
fi
echo
echo "Test:"
echo "  ./scripts/test-socket-client.sh ping"
echo "  ./scripts/test-socket-client.sh auth"
echo
echo "Expected auth response should fail closed:"
echo "  status fail, reason auth_not_implemented"
