#!/usr/bin/env bash
set -euo pipefail

key_tool="${1:-./build/daemon/face-unlock-key-tool}"
template_tool="${2:-./build/daemon/face-unlock-template-tool}"

echo "[test-key-template-flow] Starting key/template integration self-test"
echo
echo "key_tool: $key_tool"
echo "template_tool: $template_tool"

if [[ ! -x "$key_tool" ]]; then
  echo "ERROR: key tool not executable: $key_tool"
  exit 1
fi

if [[ ! -x "$template_tool" ]]; then
  echo "ERROR: template tool not executable: $template_tool"
  exit 1
fi

tmp_home="$(mktemp -d)"
trap 'rm -rf "$tmp_home"' EXIT

export HOME="$tmp_home"
export USER="face-unlock-test-user"

template_path="$HOME/.local/share/face-unlock/template.enc"
manifest_path="$HOME/.local/share/face-unlock/enrollment.json"
key_path="$HOME/.local/share/face-unlock/template.key"

echo
echo "temp_home: $HOME"

echo
echo "[test-key-template-flow] Initial status"
"$key_tool" status
"$template_tool" status

echo
echo "[test-key-template-flow] Create development key"
"$key_tool" create-dev-key --i-understand-dev-key-risk --overwrite

if [[ ! -f "$key_path" ]]; then
  echo "ERROR: expected key file missing: $key_path"
  exit 1
fi

key_mode="$(stat -c '%a' "$key_path")"
key_size="$(stat -c '%s' "$key_path")"

echo "key_mode: $key_mode"
echo "key_size: $key_size"

if [[ "$key_mode" != "600" ]]; then
  echo "ERROR: key file mode is not 600"
  exit 1
fi

if [[ "$key_size" != "32" ]]; then
  echo "ERROR: key file size is not 32 bytes"
  exit 1
fi

echo
echo "[test-key-template-flow] Create placeholder template with development key"
"$template_tool" create-placeholder --i-understand-placeholder --overwrite --use-dev-key

if [[ ! -f "$template_path" ]]; then
  echo "ERROR: expected template file missing: $template_path"
  exit 1
fi

if [[ ! -f "$manifest_path" ]]; then
  echo "ERROR: expected manifest file missing: $manifest_path"
  exit 1
fi

template_mode="$(stat -c '%a' "$template_path")"
manifest_mode="$(stat -c '%a' "$manifest_path")"

echo "template_mode: $template_mode"
echo "manifest_mode: $manifest_mode"

if [[ "$template_mode" != "600" ]]; then
  echo "ERROR: template file mode is not 600"
  exit 1
fi

if [[ "$manifest_mode" != "600" ]]; then
  echo "ERROR: manifest file mode is not 600"
  exit 1
fi

echo
echo "[test-key-template-flow] Verify decrypt"
"$template_tool" verify-decrypt --use-dev-key

echo
echo "[test-key-template-flow] Validate manifest JSON parses"
python3 -m json.tool "$manifest_path" >/tmp/face-unlock-test-manifest.json

if ! grep -q '"placeholder_only": true' /tmp/face-unlock-test-manifest.json; then
  echo "ERROR: manifest does not indicate placeholder_only true"
  exit 1
fi

if ! grep -q '"key_storage": "local_development_key_file"' /tmp/face-unlock-test-manifest.json; then
  echo "ERROR: manifest does not record local_development_key_file"
  exit 1
fi

rm -f /tmp/face-unlock-test-manifest.json

echo
echo "[test-key-template-flow] Delete template and manifest"
"$template_tool" delete --yes

if [[ -e "$template_path" ]]; then
  echo "ERROR: template file still exists after delete"
  exit 1
fi

if [[ -e "$manifest_path" ]]; then
  echo "ERROR: manifest file still exists after delete"
  exit 1
fi

echo
echo "[test-key-template-flow] Delete key"
"$key_tool" delete --yes

if [[ -e "$key_path" ]]; then
  echo "ERROR: key file still exists after delete"
  exit 1
fi

echo
echo "key_template_flow_status: ok"
