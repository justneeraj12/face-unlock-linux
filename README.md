# face-unlock-linux

[![Build](https://github.com/justneeraj12/face-unlock-linux/actions/workflows/build.yml/badge.svg)](https://github.com/justneeraj12/face-unlock-linux/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/justneeraj12/face-unlock-linux?include_prereleases)](https://github.com/justneeraj12/face-unlock-linux/releases)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Ubuntu%2024.04-orange.svg)](docs/development-setup.md)
[![Status](https://img.shields.io/badge/status-v0.2%20development-yellow.svg)](docs/project-status.md)

CPU-first, local-only face recognition infrastructure for Linux.

The product goal is a lightweight add-on that users can install, open, enroll
with a guided head-turn flow, and use without depending on CUDA, discrete GPU
drivers, or cloud services. Intel and AMD laptop CPUs are the primary runtime
target.

> This repository is still a development prototype. Real biometric matching,
> liveness protection, and production PAM integration are not finished. It
> must not be used as the only authentication method.

## Product direction

The intended experience is simple:

1. Install the package.
2. Open the enrollment GUI.
3. Look at the camera and slowly turn left, right, up, and down.
4. Let the local CPU build an encrypted multi-pose face profile.
5. Keep password or PIN fallback available for every login.

Enrollment does not fine-tune a neural network. Pretrained CPU models process
many frames, reject poor or duplicate samples, and build a compact user profile
from normalized embeddings.

The project is being built in small, testable stages. A one-command production
installer is a goal, not a capability of the current release.

## Current status

| Area | Current implementation |
|---|---|
| Platform | Ubuntu 24.04 LTS, x86_64 |
| Daemon | C++17 per-user process |
| Camera | OpenCV one-shot, loop, and worker modes |
| Detection | CPU YuNet in C++; noop and Haar fallbacks |
| Detection output | boxes, confidence, five landmarks, latency |
| Recognition | CPU SFace Python prototype |
| Enrollment | guided, multi-pose, memory-only Python prototype |
| Profile builder | pose coverage, duplicate rejection, normalized centroids |
| IPC | UNIX socket with mode 0600 and peer credential checks |
| PAM | minimal C IPC client with bounded timeout |
| Templates | libsodium placeholder encryption and development key tooling |
| GUI | Qt6 consent, status, pose, quality, and privacy scaffold |
| Authentication | fail-closed; real matcher not connected |
| Liveness | not implemented |
| Packaging | development Debian/CPack skeleton |

The current development phase is moving SFace embedding, profile construction,
and enrollment control into the daemon. See [project status](docs/project-status.md)
and the [roadmap](ROADMAP.md).

## Architecture

The security boundary is intentionally small:

    PAM service
        |
        v
    pam_face_unlock.so       tiny C client; no OpenCV, Qt, Torch, or sodium
        |
        | UNIX socket /run/user/$UID/face-unlock.sock
        v
    face-unlockd             normal desktop user
        |
        +-- camera worker
        +-- CPU YuNet detector
        +-- future CPU SFace matcher
        +-- encrypted per-user profile
        |
        v
    explicit success or fail-closed response

The PAM module never opens the camera or loads a model. Heavy work stays in the
unprivileged daemon. The socket uses mode 0600 and SO_PEERCRED checks.

Read the [architecture](docs/architecture.md) and
[threat model](docs/threat-model.md) before changing authentication behavior.

## CPU model baseline

The development baseline is:

- YuNet 2022mar for face detection
- SFace 2021dec for alignment and embeddings
- OpenCV DNN backend
- OpenCV CPU target
- no required CUDA, TensorRT, OpenCL, or vendor GPU runtime

Download pinned model files and verify their SHA-256 checksums:

    ./scripts/download-cpu-models.sh

The files are placed under models/ and ignored by Git.

Camera-free benchmark on an Intel i5-12500H with Ubuntu 24.04 and OpenCV 4.6:

| Operation | Development result |
|---|---:|
| Python YuNet 320x320 p50 | about 3.8 ms |
| Python SFace 112x112 p50 | about 9.9 ms |
| C++ YuNet blank-frame smoke inference | about 6.6 ms |
| Python benchmark peak RSS | about 277 MB |

These synthetic measurements validate runtime cost only. They do not measure
recognition accuracy, liveness, camera latency, or end-to-end authentication.

Run the benchmark locally:

    python3 python/benchmark_cpu_models.py --iterations 100

## Developer quickstart

Clone and enter the repository:

    git clone https://github.com/justneeraj12/face-unlock-linux.git
    cd face-unlock-linux

Install development dependencies with an explicit confirmation prompt:

    ./setup-dev.sh

Download the CPU models:

    ./scripts/download-cpu-models.sh

Build and test:

    ./scripts/build.sh
    ./scripts/test.sh
    ./scripts/audit-dependencies.sh

Run the C++ YuNet smoke test:

    ./build/daemon/face-unlock-detector-selftest --yunet-model models/face_detection_yunet_2022mar.onnx

Run the daemon with camera and CPU YuNet:

    ./build/daemon/face-unlockd --camera 0 --detector yunet --detector-model models/face_detection_yunet_2022mar.onnx --daemon

In another terminal:

    ./scripts/test-socket-client.sh ping
    ./scripts/test-socket-client.sh camera_status
    ./scripts/test-socket-client.sh detector_status
    ./scripts/test-socket-client.sh auth

The auth request must fail because the real matcher is not connected yet.

## Guided enrollment prototype

Run the current non-persistent enrollment prototype:

    python3 python/prototype_enroll_cpu.py --i-understand-biometric-risk

It guides the user through center, left, right, up, and down poses. It keeps
frames, aligned crops, and embeddings in memory and writes no biometric profile.

The next implementation step is to expose daemon enrollment start, status,
cancel, and commit operations, then connect them to the Qt GUI.

## Enrollment GUI

Build the optional Qt6 GUI:

    ./scripts/build-gui.sh

Run it:

    ./build-gui/gui/face-unlock-enroll

The GUI currently provides consent and privacy information, daemon status,
template status, pose and quality scaffolds, and placeholder-data deletion. It
does not yet perform real enrollment or enable authentication.

## Safety model

Authentication software can lock users out. This project therefore requires:

- password or PIN fallback
- fail-closed behavior on every error
- no raw image or embedding logs
- encrypted per-user templates
- local-only processing
- explicit opt-in for development auth
- explicit review, backup, confirmation, and rollback for PAM changes
- a minimal dependency set inside the PAM module

Development-only switches:

    FACE_UNLOCK_DEV_ALLOW=1
    FACE_UNLOCK_ALLOW_ROOT_AUTH=1

They must never be enabled as production authentication defaults.

Do not edit sudo, common-auth, GDM, SDDM, or LightDM PAM files manually. Use
only the documented fake PAM flow while developing. Read
[PAM safety](docs/pam-safety.md) and
[sudo apply and rollback](docs/sudo-apply-and-rollback.md).

## Testing

Fast local checks:

    ./scripts/check-scripts.sh
    ./scripts/check-docs.sh
    ./scripts/check-json.sh
    ./scripts/build.sh
    ./scripts/test.sh
    ./scripts/audit-dependencies.sh

Full verification and package inspection:

    ./scripts/verify-local.sh

CTest covers crypto, key/template handling, daemon metadata, fail-closed auth
reasons, detector backends, and CPU profile construction. Camera-dependent
quality and accuracy evaluation remain manual work.

The dependency audit ensures pam_face_unlock.so does not link OpenCV, Qt,
Torch, CUDA, TensorRT, Python, or libsodium.

## Repository layout

| Path | Purpose |
|---|---|
| daemon/ | C++ camera, detector, IPC, crypto, and template tools |
| pam/ | minimal C PAM IPC module |
| gui/ | optional Qt6 enrollment application |
| python/ | CPU model, enrollment, and evaluation prototypes |
| models/ | model provenance notes; weights are ignored |
| schemas/ | metadata schemas and examples |
| scripts/ | build, test, model, packaging, and safety helpers |
| packaging/ | systemd and package assets |
| docs/ | architecture, security, development, and release documentation |
| .github/ | CI workflows and contribution templates |

Generated directories such as build/, build-gui/, .venv/, local models, keys,
templates, captures, and evaluation output are excluded by .gitignore.

## Documentation

Start with the [documentation index](docs/README.md).

Most useful references:

- [Current project status](docs/project-status.md)
- [Architecture](docs/architecture.md)
- [CPU face profile](docs/cpu-face-profile.md)
- [C++ detector backends](docs/daemon-detector-scaffold.md)
- [Configuration](docs/configuration.md)
- [Testing](docs/testing.md)
- [Threat model](docs/threat-model.md)
- [PAM safety](docs/pam-safety.md)
- [Key management](docs/key-management.md)
- [Packaging](docs/packaging.md)

## Contributing

Small, reviewable changes are preferred. Security-sensitive changes must
explain failure behavior, privacy impact, test coverage, and rollback.

Read [CONTRIBUTING.md](CONTRIBUTING.md),
[SECURITY.md](SECURITY.md), and
[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

## License

Apache License 2.0. See [LICENSE](LICENSE).
