#!/usr/bin/env bash
set -euo pipefail

daemon="${1:-./build/daemon/face-unlockd}"
key_tool="${2:-./build/daemon/face-unlock-key-tool}"
template_tool="${3:-./build/daemon/face-unlock-template-tool}"
client="${4:-./scripts/test-socket-client.sh}"

echo "[test-auth-reasons] Starting auth reason integration test"

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

start_daemon() {
  "$daemon" --serve >"$daemon_log" 2>&1 &
  daemon_pid="$!"

  for _ in $(seq 1 50); do
    if [[ -S "$socket_path" ]]; then
      return 0
    fi
    sleep 0.1
  done

  echo "ERROR: daemon socket was not created"
  cat "$daemon_log"
  exit 1
}

stop_daemon() {
  if [[ -n "${daemon_pid:-}" ]]; then
    kill "$daemon_pid" 2>/dev/null || true
    wait "$daemon_pid" 2>/dev/null || true
    daemon_pid=""
  fi

  rm -f "$socket_path"
}

expect_auth_reason() {
  local expected="$1"

  response="$("$client" auth)"
  echo "response: $response"

  if [[ "$response" != *"\"reason\":\"$expected\""* ]]; then
    echo "ERROR: expected reason: $expected"
    echo "daemon log:"
    cat "$daemon_log"
    exit 1
  fi

  echo "auth_reason_status: ok expected=$expected"
}

echo "temp_home: $HOME"
echo "temp_runtime: $XDG_RUNTIME_DIR"

echo
echo "[test-auth-reasons] Case 1: missing template"
start_daemon
expect_auth_reason "template_missing"
stop_daemon

echo
echo "[test-auth-reasons] Case 2: discarded key template"
"$template_tool" delete --yes >/dev/null || true
"$key_tool" delete --yes >/dev/null || true
"$template_tool" create-placeholder --i-understand-placeholder --overwrite
start_daemon
expect_auth_reason "template_not_decryptable"
stop_daemon

echo
echo "[test-auth-reasons] Case 3: dev-key template but key deleted"
"$template_tool" delete --yes >/dev/null || true
"$key_tool" delete --yes >/dev/null || true
"$key_tool" create-dev-key --i-understand-dev-key-risk --overwrite
"$template_tool" create-placeholder --i-understand-placeholder --overwrite --use-dev-key
"$key_tool" delete --yes
start_daemon
expect_auth_reason "key_missing"
stop_daemon

echo
echo "[test-auth-reasons] Case 4: dev-key template decrypts but matcher missing"
"$template_tool" delete --yes >/dev/null || true
"$key_tool" delete --yes >/dev/null || true
"$key_tool" create-dev-key --i-understand-dev-key-risk --overwrite
"$template_tool" create-placeholder --i-understand-placeholder --overwrite --use-dev-key
start_daemon
expect_auth_reason "matcher_not_implemented"
stop_daemon

echo
echo "auth_reason_test_status: ok"
