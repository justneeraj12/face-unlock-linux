# Dependency Audit

This document describes dependency expectations for face-unlock-linux.

## Goals

Dependency rules are important because this project touches authentication.

Core goals:

- keep PAM module minimal
- keep heavy dependencies out of PAM
- keep camera/model logic in user daemon
- make optional dependencies explicit
- make audits reproducible

## PAM module dependency policy

The PAM module must remain tiny.

Allowed PAM dependencies:

- libc
- libpam
- libaudit if pulled by libpam
- libcap-ng if pulled by libaudit
- dynamic loader and system runtime libraries

Disallowed PAM dependencies:

- OpenCV
- LibTorch
- CUDA
- TensorRT
- Qt
- Python
- libsodium
- TensorFlow
- networking/cloud SDKs

Audit command:

    ldd build/pam/pam_face_unlock.so

The output should not contain:

    opencv
    torch
    cuda
    cudart
    Qt
    tensorflow
    sodium

## Daemon dependencies

The daemon may use:

- libc/libstdc++
- OpenCV core
- OpenCV videoio
- pthread/system threading
- optional LibTorch when WITH_TORCH=ON

Current daemon CMake locates minimal OpenCV libraries:

- opencv2/core.hpp
- libopencv_core
- libopencv_videoio

## Crypto dependencies

Template crypto scaffold uses:

- libsodium

libsodium is linked into crypto helper targets and template tooling.

libsodium must not be linked into the PAM module.

## GUI dependencies

The Qt GUI is optional.

Build option:

    -DBUILD_GUI=ON

GUI dependency:

    qt6-base-dev

The main CI workflow does not build the GUI by default.

The optional GUI workflow builds it separately.

## Torch dependencies

TorchScript support is optional.

Build option:

    -DWITH_TORCH=ON

Default:

    OFF

Reasons:

- LibTorch is large
- CI should stay lightweight
- real model integration is not complete

## Python dependencies

Python prototype dependencies are split:

Lightweight capture:

    python/requirements.txt

Torch export/evaluation:

    python/requirements-torch.txt

Python scripts are not part of the trusted authentication path.

## Build dependencies on Ubuntu 24.04

Core native development packages:

    build-essential
    cmake
    pkg-config
    libopencv-core-dev
    libopencv-videoio-dev
    libpam0g-dev
    libsodium-dev

Developer convenience packages may include:

    libopencv-dev
    python3-opencv
    pamtester
    v4l-utils
    qt6-base-dev

## CI dependency audit

GitHub Actions audits PAM dependencies.

The build fails if pam_face_unlock.so links to disallowed heavy dependencies.

Relevant workflow:

    .github/workflows/build.yml

## Local verification

Run:

    ./scripts/verify-local.sh

This includes PAM dependency auditing.

## Package audit

Build package:

    ./scripts/package-deb.sh

Inspect package:

    dpkg-deb -I build/*.deb
    dpkg-deb -c build/*.deb

Check installed dependencies before publishing releases.
