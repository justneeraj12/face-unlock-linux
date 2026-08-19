# Continuous Integration

This project uses GitHub Actions for CI.

## Workflow

Current workflow:

    .github/workflows/build.yml

## Runner

The workflow uses:

    ubuntu-24.04

This matches the primary target platform for development and testing.

## CI checks

The build workflow performs:

- repository checkout
- dependency installation
- tool version display
- Markdown documentation check
- CMake configure and build
- daemon CLI smoke test
- build output verification
- PAM module dependency audit
- artifact upload

## Dependency audit

The PAM module must remain small.

The CI job fails if pam_face_unlock.so links to heavy or disallowed dependencies such as:

- OpenCV
- Torch
- CUDA
- Qt
- TensorFlow

Allowed dependencies include:

- libpam
- libc
- libaudit
- libcap-ng

## Artifacts

The CI uploads:

    face-unlockd-ubuntu-24.04
    pam-face-unlock-ubuntu-24.04

These artifacts are for development inspection only.

They are not production release packages yet.

## Camera tests

CI does not run camera tests because GitHub-hosted runners do not provide the project target webcam hardware.

Camera tests are manual for now:

    ./build/daemon/face-unlockd --camera 0
    ./build/daemon/face-unlockd --camera 0 --daemon

## PAM tests

CI does not modify system PAM files.

Fake PAM service testing remains manual for now.

See:

    docs/pam-fake-service-test.md

## Debian package artifact

CI also builds the CPack Debian package with:

    cmake --build build --target package

The workflow uploads the package artifact as:

    face-unlock-linux-deb-ubuntu-24.04

The .deb artifact is for development testing and inspection.

It does not automatically modify PAM files.

## Minimal OpenCV dependencies

The daemon currently uses only OpenCV core and videoio APIs.

To keep CI faster and avoid installing the full OpenCV development dependency tree, the workflow installs:

    libopencv-core-dev
    libopencv-videoio-dev

instead of:

    libopencv-dev

The daemon CMake configuration manually locates:

- opencv2/core.hpp
- libopencv_core
- libopencv_videoio

This avoids requiring opencv4 pkg-config metadata in CI.
