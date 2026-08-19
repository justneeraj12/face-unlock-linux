#!/usr/bin/env bash
set -euo pipefail

config_dir="$HOME/.config/face-unlock"
config_path="$config_dir/config.json"

echo "[write-default-config] Face Unlock default config writer"
echo
echo "This script will create or overwrite:"
echo "  $config_path"
echo
echo "It will NOT:"
echo "  - modify PAM files"
echo "  - install PAM modules"
echo "  - enable systemd services"
echo "  - change authentication settings"
echo

read -r -p "Write default config? Type YES: " answer

if [[ "$answer" != "YES" ]]; then
  echo "Aborted."
  exit 1
fi

mkdir -p "$config_dir"

cat > "$config_path" <<'JSON_EOF'
{
  "camera_index": 0,
  "max_auth_attempts": 3
}
JSON_EOF

chmod 0600 "$config_path"

echo
echo "Wrote:"
echo "  $config_path"
echo
echo "Contents:"
cat "$config_path"
echo
echo "Test with:"
echo "  ./build/daemon/face-unlockd --daemon"
echo
echo "Override config camera index with:"
echo "  ./build/daemon/face-unlockd --camera 0 --daemon"
