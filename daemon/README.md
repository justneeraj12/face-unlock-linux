# Daemon

This directory contains the user-level face unlock daemon.

## Current status

- minimal C++17 executable
- links OpenCV
- prints startup/runtime information
- opens webcam
- reads one frame by default
- can run continuously with --loop
- prints loop status once per second
- stops cleanly with Ctrl+C
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

Run one-shot camera probe with default camera index 0:

    ./build/daemon/face-unlockd

Run one-shot probe with an explicit camera index:

    ./build/daemon/face-unlockd --camera 0

Run continuous camera loop:

    ./build/daemon/face-unlockd --camera 0 --loop

Stop loop mode with Ctrl+C.

## Current expected one-shot output

Example output:

    face-unlockd prototype
    version: 0.1.0
    uid: 1000
    runtime_dir: /run/user/1000
    planned_socket: /run/user/1000/face-unlock.sock
    mode: one_shot
    camera_index: 0
    camera_status: opened
    frame_status: ok
    frame_width: 640
    frame_height: 480
    frame_channels: 3
    status: ok

## Current expected loop output

Example output:

    face-unlockd prototype
    version: 0.1.0
    uid: 1000
    runtime_dir: /run/user/1000
    planned_socket: /run/user/1000/face-unlock.sock
    mode: loop
    camera_index: 0
    camera_status: opened
    loop_status: started
    stop_hint: press Ctrl+C to stop
    loop_report: frames_total=30 fps=29.9 width=640 height=480 channels=3

## Privacy

The current daemon reads frames into memory only.

It does not save images, face crops, embeddings, templates, or logs containing biometric data.
