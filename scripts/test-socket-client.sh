#!/usr/bin/env bash
set -euo pipefail

uid_now="$(id -u)"
socket_path="${XDG_RUNTIME_DIR:-/run/user/${uid_now}}/face-unlock.sock"

if [[ ! -S "$socket_path" ]]; then
  echo "socket_not_found: $socket_path"
  echo "start daemon first:"
  echo "  ./build/daemon/face-unlockd --serve"
  exit 1
fi

python3 - "$socket_path" <<'PY'
import socket
import sys

socket_path = sys.argv[1]

client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client.settimeout(2.0)
client.connect(socket_path)
client.sendall(b'{"op":"ping","client":"test-socket-client"}\n')
response = client.recv(4096)
client.close()

print(response.decode("utf-8", errors="replace").strip())
PY
