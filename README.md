# face-unlock-linux

Experimental open-source face recognition unlock utility for Ubuntu Linux.

face-unlock-linux aims to provide a secure, low-latency, user-controlled face unlock system for Ubuntu 24.04 and compatible Linux desktops.

The long-term goal is to support:

- lock-screen authentication
- sudo authentication
- desktop session unlock
- optional greeter/login support
- X11 and Wayland environments
- encrypted local face templates
- user consent, enrollment, and deletion controls

This project is currently in early development.

## Project status

Status: design and prototype phase.

Do not use this project yet as your only authentication method.

Current implemented prototype:

- documented open-source repository foundation
- minimal C++ daemon executable
- CMake build
- OpenCV camera frame probe
- no PAM integration yet
- no system authentication changes

## Target platform

Primary target:

- Ubuntu 24.04 LTS
- Intel x86_64 laptops/desktops
- internal webcam or USB webcam
- GNOME, KDE Plasma, or LightDM-based desktops

Development target machine:

- MSI GF66
- Intel i5-12500H
- NVIDIA RTX 3050 Ti
- 16 GB RAM
- Ubuntu 24.04 with OEM kernel
- working NVIDIA drivers and CUDA

## Planned architecture

The project is split into small components:

    +--------------------+
    | PAM module         |
    | tiny C IPC client  |
    +---------+----------+
              |
              | UNIX socket
              v
    +--------------------+
    | user daemon        |
    | C++17              |
    | OpenCV + Torch     |
    +---------+----------+
              |
              v
    +--------------------+
    | camera + model     |
    | local only         |
    +--------------------+

    +--------------------+
    | enrollment GUI     |
    | Qt6                |
    +--------------------+

The PAM module should stay minimal and auditable.

It should not link to:

- OpenCV
- Qt
- LibTorch
- CUDA
- TensorRT

The daemon performs the heavier work and runs as the user.

## Repository layout

Current/planned layout:

    daemon/       C++17 user daemon
    pam/          tiny C PAM module
    gui/          Qt6 enrollment GUI
    python/       prototype scripts
    models/       local model placeholders and export docs
    packaging/    deb/systemd/installer assets
    docs/         architecture, threat model, setup guides
    scripts/      helper scripts
    tests/        unit and integration tests

## Build current prototype

Install dependencies:

    sudo apt update
    sudo apt install build-essential cmake libopencv-dev pkg-config

Build:

    ./scripts/build.sh

Run the daemon camera probe:

    ./build/daemon/face-unlockd --camera 0

Expected important output:

    camera_status: opened
    frame_status: ok
    status: ok

The current prototype reads one camera frame into memory and exits.

It does not save images, face crops, embeddings, templates, or logs containing biometric data.

## Security goals

This project is designed around the following principles:

- fail closed by default
- never store raw images unless explicitly requested for development
- encrypted face templates
- local-only processing
- minimal PAM module
- strict UNIX socket permissions
- peer credential checks
- safe fallback to password/PIN
- explicit consent for enrollment and brightness changes
- easy rollback for all system modifications

See:

- SECURITY.md
- docs/threat-model.md
- docs/pam-safety.md

## Non-goals

This project does not aim to:

- replace passwords entirely
- provide remote/cloud biometric authentication
- upload face images or embeddings
- bypass Linux desktop security models
- modify PAM automatically without explicit user consent

## Development checks

Run:

    ./scripts/check-docs.sh
    ./scripts/build.sh

## Safety warning

Authentication software can lock you out of your system if configured incorrectly.

During development:

- do not modify /etc/pam.d/* manually
- keep a root shell open when testing PAM changes
- test with a fake PAM service before touching sudo, GDM, SDDM, or LightDM
- always keep password authentication as fallback

## License

Apache License 2.0.

See LICENSE.
