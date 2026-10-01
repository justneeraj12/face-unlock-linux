# Continuous Integration

This project uses GitHub Actions for CI.

## Workflow

Current workflow:

    .github/workflows/build.yml

## Runners

Core, GUI, package, and release jobs use a platform matrix:

    ubuntu-24.04
    ubuntu-26.04

This catches the OpenCV 4.6/4.10 ABI and model-graph differences between the
two supported LTS releases.

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
- version-aware CPU model selection and checksum verification

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

The package workflow uploads one artifact per build platform:

    face-unlock-linux-deb-ubuntu-24.04
    face-unlock-linux-deb-ubuntu-26.04

The .deb artifact is for development testing and inspection.

It does not automatically modify PAM files.

## Focused OpenCV dependencies

CI installs the component development packages used by the CPU pipeline:

    libopencv-core-dev
    libopencv-dnn-dev
    libopencv-imgproc-dev
    libopencv-objdetect-dev
    libopencv-videoio-dev

This avoids the unrelated dependencies pulled in by the complete
`libopencv-dev` meta-package while still building camera capture, YuNet, SFace,
and the optional Haar baseline.

## Manifest validation in CI

CI validates enrollment manifest scaffolding with:

    ./scripts/check-json.sh
    scripts/validate-enrollment-manifest.py schemas/enrollment-manifest.example.json

This ensures the example manifest remains valid and privacy-safe.

## GUI build workflow

The optional Qt GUI is built by a separate workflow:

    .github/workflows/gui-build.yml

The GUI workflow runs manually and for relevant pushes and pull requests.

It installs Qt6 development packages and builds with:

    -DBUILD_GUI=ON

The main build workflow keeps GUI disabled by default to stay faster.

## Model selection

`scripts/download-cpu-models.sh` pins and verifies both supported YuNet
variants, then selects the graph compatible with the runner's OpenCV version.
Ubuntu 24.04 selects YuNet 2022mar; Ubuntu 26.04 selects YuNet 2023mar. Runtime
commands use the stable `models/face_detection_yunet.onnx` symlink.

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

## Detector smoke test dependency behavior

The Python detector smoke test runs before apt dependency installation.

It uses the noop backend and should not require NumPy, OpenCV, or camera hardware.

## Detector output validation in CI

CI validates detector output metadata scaffolding with:

    scripts/validate-detector-output.py schemas/detector-output.example.json

This check does not require a camera.

## Detector output generation in CI

CI runs:

    ./scripts/test-detector-output-generation.sh

This uses a synthetic frame and does not require camera hardware or OpenCV.

## Detector output generation in package/release workflows

The package and release workflows also run:

    ./scripts/test-detector-output-generation.sh

This ensures release artifacts are built only after detector output generation and validation pass.

## GUI workflow build helper

The GUI workflow uses:

    ./scripts/build-gui.sh

This keeps optional GUI build behavior consistent between local development and CI.
