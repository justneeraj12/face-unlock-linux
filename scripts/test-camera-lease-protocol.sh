#!/usr/bin/env bash
set -euo pipefail

daemon="${1:-./build/daemon/face-unlockd}"
client="${2:-./scripts/test-socket-client.sh}"

test_home="$(mktemp -d)"
test_runtime="$(mktemp -d)"
daemon_log="$(mktemp)"
daemon_pid=""

cleanup() {
  if [[ -n "$daemon_pid" ]]; then
    kill "$daemon_pid" 2>/dev/null || true
    wait "$daemon_pid" 2>/dev/null || true
  fi
  rm -rf "$test_home" "$test_runtime"
  rm -f "$daemon_log"
}
trap cleanup EXIT

HOME="$test_home" XDG_RUNTIME_DIR="$test_runtime" \
  "$daemon" --camera 9999 --daemon >"$daemon_log" 2>&1 &
daemon_pid="$!"
socket_path="$test_runtime/face-unlock.sock"

for _ in $(seq 1 50); do
  [[ -S "$socket_path" ]] && break
  sleep 0.05
done

if [[ ! -S "$socket_path" ]]; then
  echo "ERROR: daemon socket was not created"
  cat "$daemon_log"
  exit 1
fi

query() {
  HOME="$test_home" XDG_RUNTIME_DIR="$test_runtime" "$client" "$1"
}

initial="$(query camera_status)"
[[ "$initial" == *'"camera":"idle"'* ]]
[[ "$initial" == *'"camera_open":false'* ]]
[[ "$initial" == *'"camera_lease_generation":0'* ]]

started="$(query lockscreen_start)"
[[ "$started" == *'"status":"ok"'* ]]
[[ "$started" == *'"op":"lockscreen_start"'* ]]
[[ "$started" == *'"implementation_status":"camera_lease_only"'* ]]
[[ "$started" == *'"camera_lease_generation":1'* ]]
[[ "$started" == *'"camera_open_timeout_ms":1500'* ]]
[[ "$started" == *'"camera_first_frame_timeout_ms":1500'* ]]
[[ "$started" == *'"recognition_window_ms":1000'* ]]

final=""
for _ in $(seq 1 50); do
  final="$(query camera_status)"
  [[ "$final" == *'"camera_lease_state":"failed"'* ]] && break
  sleep 0.05
done

if [[ "$final" != *'"camera_lease_state":"failed"'* ]]; then
  echo "ERROR: invalid camera did not fail closed"
  echo "$final"
  cat "$daemon_log"
  exit 1
fi

[[ "$final" == *'"camera":"error"'* ]]
[[ "$final" == *'"camera_open":false'* ]]
[[ "$final" == *'"camera_release_reason":"open_failed"'* ]]

cancelled="$(query lockscreen_password_started)"
[[ "$cancelled" == *'"status":"ok"'* ]]
[[ "$cancelled" == *'"op":"lockscreen_password_started"'* ]]
[[ "$cancelled" == *'"released":true'* ]]

query ping >/dev/null

echo "camera_lease_protocol_status: ok"
