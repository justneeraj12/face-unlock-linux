# Daemon

This directory contains the user-level face unlock daemon.

## Current status

- minimal C++17 executable
- links OpenCV
- prints startup/runtime information
- opens webcam
- reads one frame
- prints frame dimensions
- does not expose socket yet
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

Run with default camera index 0:

    ./build/daemon/face-unlockd

Run with a different camera index:

    ./build/daemon/face-unlockd --camera 1

## Current expected output

Example output:

    face-unlockd prototype
    version: 0.1.0
    uid: 1000
    runtime_dir: /run/user/1000
    planned_socket: /run/user/1000/face-unlock.sock
    camera_index: 0
    camera_status: opened
    frame_status: ok
    frame_width: 640
    frame_height: 480
    frame_channels: 3
    status: ok

## Privacy

The current daemon reads one frame into memory and exits.

It does not save images, face crops, embeddings, templates, or logs containing biometric data.
