#!/usr/bin/env bash
set -euo pipefail

daemon="${FACE_UNLOCK_DAEMON:-./build/daemon/face-unlockd}"
client="${FACE_UNLOCK_CLIENT:-./scripts/test-socket-client.sh}"
camera_index="0"

if [[ "${1:-}" == "--camera" && -n "${2:-}" ]]; then
  camera_index="$2"
elif [[ $# -ne 0 ]]; then
  echo "Usage: $0 [--camera INDEX]"
  exit 2
fi

if [[ ! "$camera_index" =~ ^[0-9]+$ ]]; then
  echo "ERROR: camera index must be a non-negative integer"
  exit 2
fi

benchmark_home="$(mktemp -d)"
benchmark_runtime="$(mktemp -d)"
daemon_log="$(mktemp)"
daemon_pid=""

cleanup() {
  if [[ -n "$daemon_pid" ]]; then
    kill "$daemon_pid" 2>/dev/null || true
    wait "$daemon_pid" 2>/dev/null || true
  fi
  rm -rf "$benchmark_home" "$benchmark_runtime"
  rm -f "$daemon_log"
}
trap cleanup EXIT

HOME="$benchmark_home" XDG_RUNTIME_DIR="$benchmark_runtime" \
  "$daemon" --camera "$camera_index" --daemon >"$daemon_log" 2>&1 &
daemon_pid="$!"
socket_path="$benchmark_runtime/face-unlock.sock"

for _ in $(seq 1 100); do
  [[ -S "$socket_path" ]] && break
  sleep 0.02
done

if [[ ! -S "$socket_path" ]]; then
  echo "ERROR: daemon socket was not created"
  cat "$daemon_log"
  exit 1
fi

query() {
  HOME="$benchmark_home" XDG_RUNTIME_DIR="$benchmark_runtime" "$client" "$1"
}

started="$(query lockscreen_start)"
echo "start_response: $started"

ready=""
for _ in $(seq 1 150); do
  response="$(query camera_status)"
  if [[ "$response" == *'"camera":"ready"'* ]]; then
    ready="$response"
    break
  fi
  if [[ "$response" == *'"camera":"error"'* || \
        "$response" == *'"camera_release_reason":"lease_deadline"'* ]]; then
    echo "final_response: $response"
    echo "camera_lease_benchmark_status: unavailable"
    exit 1
  fi
  sleep 0.02
done

if [[ -z "$ready" ]]; then
  echo "ERROR: no usable frame arrived before the benchmark deadline"
  cat "$daemon_log"
  exit 1
fi

echo "ready_response: $ready"
cancelled="$(query lockscreen_cancel)"
echo "cancel_response: $cancelled"

if [[ "$cancelled" != *'"released":true'* || \
      "$cancelled" != *'"camera_open":false'* ]]; then
  echo "ERROR: camera was not released after benchmark"
  exit 1
fi

echo "privacy_status: frames_memory_only"
echo "camera_lease_benchmark_status: ok"
