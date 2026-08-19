# Daemon

This directory contains the user-level face unlock daemon.

## Current status

- minimal C++17 executable
- links OpenCV
- prints startup/runtime information
- opens webcam
- reads one frame by default
- can run continuously with --loop
- can run a local UNIX socket server with --serve
- can run camera worker plus socket server with --daemon
- socket path is /run/user/$UID/face-unlock.sock
- socket permissions are set to 0600
- logs UNIX socket peer credentials with SO_PEERCRED
- currently allows same-UID socket clients only
- stops cleanly with Ctrl+C
- does not authenticate yet
- does not save images

## Planned responsibilities

The daemon is planned to be responsible for:

- opening the camera
- maintaining a recent frame buffer
- loading the face recognition model
- reading encrypted templates
- performing face matching
- exposing a local UNIX domain socket
- enforcing authentication rate limits

The daemon should run as the normal desktop user, not as root.

## Fail-closed behavior

The daemon must fail closed if:

- the camera is unavailable
- no template exists
- the model is unavailable
- the IPC peer is not trusted
- authentication times out

## Build

From the repository root, build with:

    ./scripts/build.sh

Run one-shot camera probe with default camera index 0:

    ./build/daemon/face-unlockd

Run continuous camera loop:

    ./build/daemon/face-unlockd --camera 0 --loop

Run socket server only:

    ./build/daemon/face-unlockd --serve

Run daemon mode with camera worker and socket server:

    ./build/daemon/face-unlockd --camera 0 --daemon

In another terminal, test the socket:

    ./scripts/test-socket-client.sh

Stop loop, server, or daemon mode with Ctrl+C.

## Current expected daemon socket output

Daemon terminal:

    face-unlockd prototype
    version: 0.1.0
    uid: 1000
    runtime_dir: /run/user/1000
    planned_socket: /run/user/1000/face-unlock.sock
    mode: daemon
    daemon_status: starting
    socket_path: /run/user/1000/face-unlock.sock
    socket_status: listening
    socket_mode: 0600
    server_status: started
    camera_index: 0
    camera_status: opened
    camera_worker_status: started
    peer_credentials: pid=12345 uid=1000 gid=1000
    peer_status: allowed
    client_request: {"op":"ping","client":"test-socket-client"}

Client terminal:

    {"status":"ok","reason":"daemon_alive","camera":"ready","frames_total":30,"frame_width":640,"frame_height":480,"frame_channels":3}

## Security notes

The socket server currently uses SO_PEERCRED to inspect the connected peer process.

Current prototype policy:

- same UID is allowed
- other users are rejected

Future PAM integration may require carefully reviewed handling for privileged PAM clients.

## Privacy

The current daemon reads frames into memory only.

Socket mode currently only replies to a local test request and frame readiness status.

It does not save images, face crops, embeddings, templates, or logs containing biometric data.
