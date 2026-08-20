#!/usr/bin/env bash
set -euo pipefail

daemon="${1:-./build/daemon/face-unlockd}"
key_tool="${2:-./build/daemon/face-unlock-key-tool}"
template_tool="${3:-./build/daemon/face-unlock-template-tool}"
client="${4:-./scripts/test-socket-client.sh}"

echo "[test-daemon-metadata] Starting daemon metadata integration test"

if [[ ! -x "$daemon" ]]; then
  echo "ERROR: daemon not executable: $daemon"
  exit 1
fi

if [[ ! -x "$key_tool" ]]; then
  echo "ERROR: key tool not executable: $key_tool"
  exit 1
fi

if [[ ! -x "$template_tool" ]]; then
  echo "ERROR: template tool not executable: $template_tool"
  exit 1
fi

if [[ ! -x "$client" ]]; then
  echo "ERROR: socket client not executable: $client"
  exit 1
fi

tmp_home="$(mktemp -d)"
tmp_runtime="$(mktemp -d)"
daemon_log="$(mktemp)"

cleanup() {
  if [[ -n "${daemon_pid:-}" ]]; then
    kill "$daemon_pid" 2>/dev/null || true
    wait "$daemon_pid" 2>/dev/null || true
  fi

  rm -rf "$tmp_home"
  rm -rf "$tmp_runtime"
  rm -f "$daemon_log"
}

trap cleanup EXIT

export HOME="$tmp_home"
export USER="face-unlock-test-user"
export XDG_RUNTIME_DIR="$tmp_runtime"

socket_path="$XDG_RUNTIME_DIR/face-unlock.sock"

echo "temp_home: $HOME"
echo "temp_runtime: $XDG_RUNTIME_DIR"

echo
echo "[test-daemon-metadata] Create dev key and placeholder template"
"$key_tool" create-dev-key --i-understand-dev-key-risk --overwrite
"$template_tool" create-placeholder --i-understand-placeholder --overwrite --use-dev-key

echo
echo "[test-daemon-metadata] Start daemon socket-only mode"
"$daemon" --serve >"$daemon_log" 2>&1 &
daemon_pid="$!"

for _ in $(seq 1 50); do
  if [[ -S "$socket_path" ]]; then
    break
  fi
  sleep 0.1
done

if [[ ! -S "$socket_path" ]]; then
  echo "ERROR: daemon socket was not created"
  echo "daemon log:"
  cat "$daemon_log"
  exit 1
fi

echo "socket_status: present"

echo
echo "[test-daemon-metadata] Query camera_status metadata"
response="$("$client" camera_status)"

echo "response: $response"

check_contains() {
  local needle="$1"

  if [[ "$response" != *"$needle"* ]]; then
    echo "ERROR: response missing: $needle"
    echo "$response"
    echo
    echo "daemon log:"
    cat "$daemon_log"
    exit 1
  fi
}

check_contains '"template":"present"'
check_contains '"enrollment":"placeholder"'
check_contains '"key":"present"'
check_contains '"decryptability":"possible_with_dev_key"'
check_contains '"key_storage":"local_development_key_file"'

echo
echo "[test-daemon-metadata] Stop daemon"
kill "$daemon_pid"
wait "$daemon_pid" 2>/dev/null || true
daemon_pid=""

echo
echo "daemon_metadata_test_status: ok"
