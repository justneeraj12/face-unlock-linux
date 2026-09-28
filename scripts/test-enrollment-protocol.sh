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
  "$daemon" --camera 9999 --detector noop --daemon >"$daemon_log" 2>&1 &
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

status="$(query enrollment_status)"
[[ "$status" == *"\"status\":\"ok\""* ]]
[[ "$status" == *"\"op\":\"enrollment_status\""* ]]
[[ "$status" == *"\"enrollment_state\":\"idle\""* ]]
[[ "$status" == *"\"progress_percent\":0"* ]]

started="$(query enrollment_start)"
[[ "$started" == *"\"status\":\"fail\""* ]]
[[ "$started" == *"\"reason\":\"landmark_detector_required\""* ]]
[[ "$started" == *"\"enrollment_state\":\"idle\""* ]]

captured="$(query enrollment_capture)"
[[ "$captured" == *"\"reason\":\"enrollment_not_collecting\""* ]]

committed="$(query enrollment_commit)"
[[ "$committed" == *"\"reason\":\"enrollment_not_ready\""* ]]

cancelled="$(query enrollment_cancel)"
[[ "$cancelled" == *"\"status\":\"ok\""* ]]
[[ "$cancelled" == *"\"enrollment_state\":\"cancelled\""* ]]

if [[ -e "$test_home/.local/share/face-unlock/template.enc" ||
      -e "$test_home/.local/share/face-unlock/template.key" ||
      -e "$test_home/.local/share/face-unlock/enrollment.json" ]]; then
  echo "ERROR: failed enrollment wrote profile data"
  exit 1
fi

echo "enrollment_protocol_fail_closed_status: ok"
