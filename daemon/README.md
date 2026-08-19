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
- auth operation fails closed by default
- development-only auth can be enabled with FACE_UNLOCK_DEV_ALLOW=1
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
- development auth is not explicitly enabled

## Build

From the repository root, build with:

    ./scripts/build.sh

Run daemon mode with camera worker and socket server:

    ./build/daemon/face-unlockd --camera 0 --daemon

Test auth fail-closed behavior in another terminal:

    ./scripts/test-socket-client.sh auth

Expected default auth response:

    {"status":"fail","op":"auth","reason":"auth_not_implemented","camera":"ready","frames_total":30,"frame_width":640,"frame_height":480,"frame_channels":3}

Run daemon mode with development-only auth enabled:

    FACE_UNLOCK_DEV_ALLOW=1 ./build/daemon/face-unlockd --camera 0 --daemon

Then test auth:

    ./scripts/test-socket-client.sh auth

Expected development-only auth response:

    {"status":"ok","op":"auth","reason":"dev_allow_camera_ready","camera":"ready","frames_total":30,"frame_width":640,"frame_height":480,"frame_channels":3}

Stop daemon mode with Ctrl+C.

## Socket operations

Test ping:

    ./scripts/test-socket-client.sh ping

Test camera status:

    ./scripts/test-socket-client.sh camera_status

Test auth:

    ./scripts/test-socket-client.sh auth

## Security notes

The socket server currently uses SO_PEERCRED to inspect the connected peer process.

Current prototype policy:

- same UID is allowed
- other users are rejected

Auth behavior:

- default auth fails closed
- FACE_UNLOCK_DEV_ALLOW=1 is for development testing only
- FACE_UNLOCK_DEV_ALLOW=1 must never be used as real authentication

Future PAM integration may require carefully reviewed handling for privileged PAM clients.

## Privacy

The current daemon reads frames into memory only.

Socket mode currently replies with operation status and frame readiness metadata.

It does not save images, face crops, embeddings, templates, or logs containing biometric data.
