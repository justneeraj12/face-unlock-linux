# face-unlock-linux

[![build](https://github.com/justneeraj12/face-unlock-linux/actions/workflows/build.yml/badge.svg)](https://github.com/justneeraj12/face-unlock-linux/actions/workflows/build.yml)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![Ubuntu](https://img.shields.io/badge/Ubuntu-24.04-orange.svg)](docs/development-setup.md)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](daemon/)
[![PAM](https://img.shields.io/badge/PAM-minimal%20IPC-red.svg)](pam/)
[![Security](https://img.shields.io/badge/security-fail--closed-brightgreen.svg)](SECURITY.md)
[![Status](https://img.shields.io/badge/status-v0.1.0--alpha%20prototype-yellow.svg)](docs/releases/v0.1.0-alpha.md)

Experimental, safety-first face unlock infrastructure for Ubuntu Linux.

face-unlock-linux is building a local, user-controlled face unlock system for Ubuntu 24.04 using a user daemon, UNIX socket IPC, a minimal PAM bridge, OpenCV, encrypted template storage scaffolding, and optional TorchScript model loading.

This repository is currently an infrastructure prototype.

It is not real biometric authentication yet.

## Why this project exists

Linux desktop face unlock projects often become risky because heavy code gets too close to PAM, or installers modify authentication files too early.

This project takes the opposite approach:

- keep PAM tiny
- run camera/model code as the user
- fail closed by default
- never modify real PAM service files without explicit consent
- keep password fallback available
- document rollback before installation
- avoid storing biometric data unless explicitly requested
- build every step slowly and audibly

## Current status

Current target milestone:

    v0.1.0-alpha

What works today:

- documented open-source repository
- CMake build
- GitHub Actions CI on Ubuntu 24.04
- CTest support
- Debian package generation with CPack
- C++17 user daemon
- OpenCV camera one-shot probe
- OpenCV camera loop mode
- camera worker thread
- UNIX domain socket server
- socket permissions set to 0600
- SO_PEERCRED peer credential logging
- same-UID socket peer policy
- JSON-ish socket operations
- fail-closed auth operation
- max auth attempt enforcement
- development-only auth gate
- minimal PAM IPC module
- fake PAM service testing
- fake PAM install/remove scripts
- systemd user service install/remove scripts
- sudo PAM inspection script
- libsodium encrypted template storage scaffold
- crypto self-test
- Python safe capture prototype
- TorchScript export stub
- optional LibTorch loader scaffold

What does not work yet:

- real face recognition authentication
- real enrollment
- encrypted template matching
- calibrated thresholds
- liveness/spoof resistance
- Qt enrollment GUI
- production sudo integration
- lock-screen integration
- GDM, SDDM, or LightDM greeter integration

See:

    docs/project-status.md
    docs/releases/v0.1.0-alpha.md
    ROADMAP.md

## Architecture

High-level design:

<pre>
┌──────────────────────────────────────────────────────────────┐
│                        Desktop User                          │
└──────────────────────────────────────────────────────────────┘

        Camera frames                    Local IPC
            │                               │
            ▼                               ▼
┌──────────────────────┐        ┌──────────────────────────────┐
│ OpenCV Camera Worker │        │ UNIX Socket Server           │
│                      │        │                              │
│ - opens webcam       │        │ - /run/user/$UID/...sock     │
│ - stores latest frame│◄──────►│ - mode 0600                  │
│ - no image saving    │        │ - SO_PEERCRED peer logging   │
└──────────────────────┘        └───────────────┬──────────────┘
                                                 │
                                                 │ auth request
                                                 ▼
                                      ┌─────────────────────┐
                                      │ PAM Module          │
                                      │ pam_face_unlock.so  │
                                      │                     │
                                      │ - tiny C IPC client │
                                      │ - no OpenCV         │
                                      │ - no Torch          │
                                      │ - no Qt             │
                                      │ - bounded timeout   │
                                      └─────────────────────┘

Future planned path:

┌──────────────────────┐
│ Qt Enrollment GUI    │
│                      │
│ - consent            │
│ - multi-angle poses  │
│ - quality checks     │
│ - brightness assist  │
│ - encrypted template │
└──────────────────────┘

┌──────────────────────┐
│ TorchScript Models   │
│                      │
│ - detector           │
│ - alignment          │
│ - embeddings         │
│ - matching           │
└──────────────────────┘
</pre>

## Security model

Core rules:

- default auth fails closed
- password fallback must remain available
- PAM module stays minimal
- heavy dependencies stay outside PAM
- daemon runs as the user
- IPC uses a UNIX domain socket
- socket permissions are 0600
- peer credentials are checked with SO_PEERCRED
- templates must be encrypted at rest
- no raw image telemetry
- no silent biometric storage
- no automatic edits to real PAM service files

The PAM module must not link to:

- OpenCV
- LibTorch
- Qt
- CUDA
- TensorRT
- Python
- libsodium

Audit locally:

    ldd build/pam/pam_face_unlock.so

See:

    SECURITY.md
    docs/threat-model.md
    docs/pam-safety.md

## Quickstart for developers

Target platform:

- Ubuntu 24.04 LTS
- x86_64
- internal or USB webcam

Install development dependencies:

    ./setup-dev.sh

Build:

    ./scripts/build.sh

Run automated tests:

    ./scripts/test.sh

Run full local verification:

    ./scripts/verify-local.sh

## Run daemon manually

One-shot camera probe:

    ./build/daemon/face-unlockd --camera 0

Expected important output:

    camera_status: opened
    frame_status: ok
    status: ok

Run daemon mode:

    ./build/daemon/face-unlockd --camera 0 --daemon

In another terminal:

    ./scripts/test-socket-client.sh ping
    ./scripts/test-socket-client.sh camera_status
    ./scripts/test-socket-client.sh auth

Default auth should fail closed:

    status fail
    reason auth_not_implemented

## Development-only auth

Development auth is disabled by default.

Manual dev-only test:

    FACE_UNLOCK_DEV_ALLOW=1 ./build/daemon/face-unlockd --camera 0 --daemon

Then:

    ./scripts/test-socket-client.sh auth

Expected dev-only response:

    status ok
    reason dev_allow_camera_ready

Warning:

FACE_UNLOCK_DEV_ALLOW=1 is only for development testing.

Never use it as real authentication.

## PAM testing

Use only the fake PAM service flow during development.

Install fake test service:

    ./scripts/install-fake-pam-test.sh

Run fake PAM test:

    pamtester face-unlock-test "$USER" authenticate

Remove fake test service:

    ./scripts/remove-fake-pam-test.sh

Documentation:

    docs/pam-fake-service-test.md

This fake test does not modify sudo, login, lock-screen, GDM, SDDM, LightDM, or common-auth PAM files.

## systemd user service

Install daemon as a user service:

    ./scripts/install-user-service.sh

Check status:

    systemctl --user status face-unlockd.service

Test IPC:

    ./scripts/test-socket-client.sh ping
    ./scripts/test-socket-client.sh auth

Remove service:

    ./scripts/remove-user-service.sh

Documentation:

    docs/systemd-user-service.md

The user service runs with:

    FACE_UNLOCK_DEV_ALLOW=0

So auth remains fail-closed by default.

## sudo integration status

sudo integration is not installed yet.

Read-only inspection:

    ./scripts/inspect-sudo-pam.sh

Documentation:

    docs/sudo-integration-plan.md

Do not manually edit:

    /etc/pam.d/sudo
    /etc/pam.d/common-auth
    /etc/pam.d/gdm-password
    /etc/pam.d/sddm
    /etc/pam.d/lightdm

## Python prototypes

Safe default capture test:

    python3 python/prototype_capture.py --camera 0 --duration-seconds 10

By default, this saves nothing.

Saving crops requires explicit privacy flags:

    --save-crops
    --i-understand-privacy-risk

Documentation:

    docs/python-prototypes.md

## TorchScript model stub

Export dummy TorchScript embedding stub:

    python3 python/export_torchscript_stub.py

Generated model:

    models/embedding_stub.pt

Model files are ignored by Git.

Documentation:

    docs/model-export.md
    docs/libtorch-loader.md

## Encrypted template storage scaffold

Run crypto self-test:

    ./build/daemon/face-unlock-crypto-selftest

Documentation:

    docs/template-storage.md

Current crypto scaffold uses placeholder bytes only.

It does not store real face templates or images.

## Configuration

Optional user config:

    ~/.config/face-unlock/config.json

Write default config:

    ./scripts/write-default-config.sh

Documentation:

    docs/configuration.md

Current supported fields:

- camera_index
- max_auth_attempts

## Debian package

Build package:

    ./scripts/package-deb.sh

Inspect package:

    dpkg-deb -c build/*.deb
    dpkg-deb -I build/*.deb

Documentation:

    docs/packaging.md

The package does not automatically modify PAM files or enable authentication integration.

## CI

GitHub Actions workflow:

    .github/workflows/build.yml

CI checks:

- documentation
- build
- tests
- daemon CLI smoke test
- PAM dependency audit
- Debian package build
- artifact upload

Documentation:

    docs/ci.md

## Release process

Prepare release:

    ./scripts/prepare-release.sh v0.1.0-alpha

Release docs:

    docs/release-process.md
    docs/releases/v0.1.0-alpha.md

The prepare script does not tag or publish automatically.

## Repository layout

<pre>
daemon/       C++ daemon, crypto scaffold, CMake targets
pam/          minimal C PAM IPC module
python/       prototype capture and TorchScript export scripts
models/       local model artifact docs; generated models ignored
packaging/    systemd user service template
scripts/      build, test, packaging, setup, verification helpers
docs/         architecture, security, testing, packaging, release docs
.github/      issue templates and GitHub Actions workflows
</pre>

## Roadmap

Near-term:

- v0.1.0-alpha release checklist
- safer sudo installer planning
- encrypted template CLI scaffold
- real embedding loader experimentation
- enrollment data format design

Later:

- real face detector and alignment
- encrypted enrollment templates
- matching thresholds
- Qt enrollment GUI
- lock-screen integration
- greeter integration
- liveness/spoof resistance
- CUDA/TensorRT optimization

See:

    ROADMAP.md

## Contributing

Read:

    CONTRIBUTING.md
    CODE_OF_CONDUCT.md
    SECURITY.md

Security-sensitive changes require extra care, especially changes involving:

- PAM
- IPC
- template encryption
- installer behavior
- sudo/login/lock-screen integration

## Safety warning

Authentication software can lock you out of your system if configured incorrectly.

During development:

- do not modify real PAM service files manually
- keep a root shell open when testing PAM changes
- test with fake PAM service first
- keep password authentication as fallback
- never make face unlock the only auth method
- never use FACE_UNLOCK_DEV_ALLOW=1 as real authentication

## License

Apache License 2.0.

See:

    LICENSE
