# face-unlock-linux

[![build](https://github.com/justneeraj12/face-unlock-linux/actions/workflows/build.yml/badge.svg)](https://github.com/justneeraj12/face-unlock-linux/actions/workflows/build.yml)

Experimental open-source face recognition unlock utility for Ubuntu Linux.

## Current status

This project is an early infrastructure prototype.

It is not real biometric authentication yet.

What works today:

- C++ user daemon
- OpenCV camera prototype
- UNIX domain socket IPC
- SO_PEERCRED peer logging
- fail-closed auth operation
- max auth attempt enforcement
- minimal PAM IPC module
- fake PAM service testing
- systemd user service helper scripts
- encrypted template storage scaffold
- Python capture prototype
- TorchScript export/load scaffolds
- Debian package generation
- GitHub Actions CI

What does not work yet:

- real face recognition authentication
- real enrollment
- real template matching
- Qt enrollment GUI
- production sudo integration
- lock-screen integration
- greeter/login integration
- liveness detection

Safety note:

- default auth fails closed
- development auth requires FACE_UNLOCK_DEV_ALLOW=1
- no real PAM service files are modified automatically
- do not use this as your only authentication method

See:

    docs/project-status.md
    docs/releases/v0.1.0-alpha.md


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

## PAM fake service test

The PAM module has a safe fake-service test flow documented in:

    docs/pam-fake-service-test.md

This test uses:

    /etc/pam.d/face-unlock-test

It does not modify sudo, login, lock-screen, GDM, SDDM, or LightDM PAM files.

## Fake PAM helper scripts

For safe fake-service testing only:

    ./scripts/install-fake-pam-test.sh
    ./scripts/remove-fake-pam-test.sh

These scripts only manage:

    /etc/pam.d/face-unlock-test
    /usr/lib/x86_64-linux-gnu/security/pam_face_unlock.so

They do not modify sudo, login, lock-screen, GDM, SDDM, LightDM, or common-auth PAM files.

## systemd user service

The daemon can be installed as a normal user service.

Documentation:

    docs/systemd-user-service.md

Install and start:

    ./scripts/install-user-service.sh

Check status:

    systemctl --user status face-unlockd.service

Test IPC:

    ./scripts/test-socket-client.sh ping
    ./scripts/test-socket-client.sh auth

Remove:

    ./scripts/remove-user-service.sh

The user service does not modify PAM files and runs with FACE_UNLOCK_DEV_ALLOW=0 by default.

## sudo integration plan

sudo integration is not installed yet.

Planning documentation:

    docs/sudo-integration-plan.md

Read-only sudo PAM inspection:

    ./scripts/inspect-sudo-pam.sh

The inspection script does not modify /etc/pam.d/sudo.

## Development setup

For Ubuntu 24.04 development setup:

    ./setup-dev.sh

Documentation:

    docs/development-setup.md

The setup script installs development packages only. It does not modify PAM files or enable authentication integration.

## Continuous integration

GitHub Actions CI is configured in:

    .github/workflows/build.yml

Documentation:

    docs/ci.md

CI builds the daemon and PAM module on Ubuntu 24.04, runs documentation checks, audits PAM module dependencies, and uploads build artifacts.

## Configuration

Optional per-user config file:

    ~/.config/face-unlock/config.json

Write a default config:

    ./scripts/write-default-config.sh

Documentation:

    docs/configuration.md

The config file currently supports camera_index and max_auth_attempts. Authentication still fails closed by default.

## Encrypted template storage scaffold

The project includes a libsodium encrypted template storage self-test.

Documentation:

    docs/template-storage.md

Run:

    ./build/daemon/face-unlock-crypto-selftest

This currently encrypts placeholder bytes only. It does not store real face templates or images.

## Testing

Run automated tests:

    ./scripts/test.sh

Testing documentation:

    docs/testing.md

Current automated test coverage includes the encrypted template storage self-test.

## Python prototypes

Python prototype scripts live in:

    python/

Documentation:

    docs/python-prototypes.md

Safe default capture test:

    python3 python/prototype_capture.py --camera 0 --duration-seconds 10

By default, the script saves nothing.

Saving face crops requires explicit privacy consent flags.

## TorchScript model export stub

A tiny dummy TorchScript embedding model can be exported for future loader tests.

Documentation:

    docs/model-export.md

Install Torch dependencies:

    pip install -r python/requirements-torch.txt

Export stub:

    python3 python/export_torchscript_stub.py

Generated model files are ignored by Git.

## Optional LibTorch loader scaffold

The daemon can optionally be built with LibTorch/TorchScript support.

Documentation:

    docs/libtorch-loader.md

Default builds do not require LibTorch.

Model test on default build:

    ./build/daemon/face-unlockd --model-test

Expected default result:

    torch_status: disabled
    status: torch_not_enabled

## Debian package skeleton

A basic .deb package can be built with CPack.

Documentation:

    docs/packaging.md

Build package:

    ./scripts/package-deb.sh

Inspect before installing:

    dpkg-deb -c build/*.deb

The package does not automatically modify PAM files or enable authentication integration.

## Project status and alpha release

Current status:

    docs/project-status.md

v0.1.0-alpha checklist:

    docs/releases/v0.1.0-alpha.md

The first alpha release is intended to be an infrastructure prototype, not real biometric authentication.

## Local verification

Run the full local verification flow:

    ./scripts/verify-local.sh

Documentation:

    docs/local-verification.md

This verifies docs, build, tests, PAM dependency audit, and Debian package generation. It does not modify PAM files or authentication settings.

## Release process

Release documentation:

    docs/release-process.md

Prepare release locally:

    ./scripts/prepare-release.sh v0.1.0-alpha

The prepare script verifies the tree and prints manual tag/release commands. It does not publish automatically.
