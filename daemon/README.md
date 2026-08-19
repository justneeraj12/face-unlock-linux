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
- supports simple socket operations: ping, camera_status, auth
- auth operation currently fails closed
- stops cleanly with Ctrl+C
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
- auth is not implemented

## Build

From the repository root, build with:

    ./scripts/build.sh

Run daemon mode with camera worker and socket server:

    ./build/daemon/face-unlockd --camera 0 --daemon

In another terminal, test ping:

    ./scripts/test-socket-client.sh ping

Test camera status:

    ./scripts/test-socket-client.sh camera_status

Test auth fail-closed behavior:

    ./scripts/test-socket-client.sh auth

Stop daemon mode with Ctrl+C.

## Current expected socket responses

Ping:

    {"status":"ok","op":"ping","reason":"daemon_alive","camera":"ready","frames_total":30,"frame_width":640,"frame_height":480,"frame_channels":3}

Camera status:

    {"status":"ok","op":"camera_status","camera":"ready","frames_total":30,"frame_width":640,"frame_height":480,"frame_channels":3}

Auth currently fails closed:

    {"status":"fail","op":"auth","reason":"auth_not_implemented","camera":"ready","frames_total":30,"frame_width":640,"frame_height":480,"frame_channels":3}

## Security notes

The socket server currently uses SO_PEERCRED to inspect the connected peer process.

Current prototype policy:

- same UID is allowed
- other users are rejected

Future PAM integration may require carefully reviewed handling for privileged PAM clients.

## Privacy

The current daemon reads frames into memory only.

Socket mode currently replies with operation status and frame readiness metadata.

It does not save images, face crops, embeddings, templates, or logs containing biometric data.
