#!/usr/bin/env bash
set -euo pipefail

daemon="${1:-./build/daemon/face-unlockd}"
selftest="${2:-./build/daemon/face-unlock-detector-selftest}"
client="${3:-./scripts/test-socket-client.sh}"

echo "[test-detector-backends] Starting detector backend integration test"

if [[ ! -x "$daemon" ]]; then
  echo "ERROR: daemon not executable: $daemon"
  exit 1
fi

if [[ ! -x "$selftest" ]]; then
  echo "ERROR: detector selftest not executable: $selftest"
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

echo
echo "[test-detector-backends] Detector self-test"
selftest_output="$("$selftest")"
echo "$selftest_output"

if [[ "$selftest_output" != *"supported_backend: noop"* ]]; then
  echo "ERROR: noop backend not reported as supported"
  exit 1
fi

start_daemon() {
  local backend="$1"

  "$daemon" --detector "$backend" --serve >"$daemon_log" 2>&1 &
  daemon_pid="$!"

  for _ in $(seq 1 50); do
    if [[ -S "$socket_path" ]]; then
      return 0
    fi
    sleep 0.1
  done

  echo "ERROR: daemon socket was not created for backend: $backend"
  echo "daemon log:"
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
  : > "$daemon_log"
}

echo
echo "[test-detector-backends] noop daemon detector_status"
start_daemon "noop"

response="$("$client" detector_status)"
echo "response: $response"

if [[ "$response" != *'"op":"detector_status"'* ]]; then
  echo "ERROR: detector_status op missing"
  exit 1
fi

if [[ "$response" != *'"detector":"noop"'* ]]; then
  echo "ERROR: noop detector response missing"
  exit 1
fi

stop_daemon

if [[ "$selftest_output" == *"supported_backend: haar"* ]]; then
  echo
  echo "[test-detector-backends] haar daemon detector_status"

  start_daemon "haar"

  response="$("$client" detector_status)"
  echo "response: $response"

  if [[ "$response" != *'"op":"detector_status"'* ]]; then
    echo "ERROR: detector_status op missing for haar"
    exit 1
  fi

  if [[ "$response" != *'"detector":"haar"'* ]]; then
    echo "ERROR: haar detector response missing"
    exit 1
  fi

  if [[ "$response" != *'"detector_ms":'* ]]; then
    echo "ERROR: haar detector_status response missing detector_ms"
    exit 1
  fi

  stop_daemon

  echo "haar_backend_status: ok"
else
  echo
  echo "haar_backend_status: skipped_not_supported"
fi

echo
echo "[test-detector-backends] unsupported backend should fail safely"

set +e
"$daemon" --detector definitely-not-a-detector --serve >"$daemon_log" 2>&1
rc="$?"
set -e

if [[ "$rc" -eq 0 ]]; then
  echo "ERROR: unsupported detector backend unexpectedly succeeded"
  cat "$daemon_log"
  exit 1
fi

if ! grep -q "detector_backend_error" "$daemon_log"; then
  echo "ERROR: unsupported detector did not print detector_backend_error"
  cat "$daemon_log"
  exit 1
fi

echo "unsupported_backend_status: ok"

echo
echo "detector_backend_test_status: ok"
