#!/usr/bin/env bash
set -euo pipefail

op="${1:-ping}"

uid_now="$(id -u)"
socket_path="${XDG_RUNTIME_DIR:-/run/user/${uid_now}}/face-unlock.sock"

if [[ ! -S "$socket_path" ]]; then
  echo "socket_not_found: $socket_path"
  echo "start daemon first:"
  echo "  ./build/daemon/face-unlockd --daemon"
  exit 1
fi

python3 - "$socket_path" "$op" <<'PY'
import json
import socket
import sys

socket_path = sys.argv[1]
op = sys.argv[2]

request = {
    "op": op,
    "client": "test-socket-client",
}

client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client.settimeout(2.0)
client.connect(socket_path)
client.sendall((json.dumps(request, separators=(",", ":")) + "\n").encode("utf-8"))
response = client.recv(4096)
client.close()

print(response.decode("utf-8", errors="replace").strip())
PY
