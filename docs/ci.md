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

## Manifest validation in CI

CI validates enrollment manifest scaffolding with:

    ./scripts/check-json.sh
    scripts/validate-enrollment-manifest.py schemas/enrollment-manifest.example.json

This ensures the example manifest remains valid and privacy-safe.

## GUI build workflow

The optional Qt GUI is built by a separate workflow:

    .github/workflows/gui-build.yml

The GUI workflow runs:

- manually with workflow_dispatch
- on pull requests that modify gui files

It installs Qt6 development packages and builds with:

    -DBUILD_GUI=ON

The main build workflow keeps GUI disabled by default to stay faster.

## Model metrics validation in CI

CI validates model evaluation metrics scaffolding with:

    scripts/validate-model-eval-metrics.py schemas/model-eval-metrics.example.json

This ensures the model evaluation metrics example remains privacy-safe and structurally valid.

## Dependency audit script in CI

CI runs:

    ./scripts/audit-dependencies.sh

This keeps local and CI dependency policy aligned.

## Split build and package workflows

The main build workflow is intended to stay fast.

Main workflow:

    .github/workflows/build.yml

Runs:

- docs
- JSON checks
- validators
- build
- tests
- dependency audit

Package workflow:

    .github/workflows/package.yml

Runs on:

- workflow_dispatch
- version tags

It builds and uploads the Debian package artifact.

Release workflow:

    .github/workflows/release.yml

Runs on version tags and publishes release assets.

## Shared CI dependency installer

CI workflows use:

    ./scripts/ci-install-deps.sh core
    ./scripts/ci-install-deps.sh gui

Documentation:

    docs/ci-dependencies.md

## Python detector smoke test in CI

CI runs:

    ./scripts/test-python-detectors.sh

This test uses the noop detector backend and does not require a camera.

The detector factory uses lazy imports so noop tests do not require Python OpenCV.
