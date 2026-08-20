# Testing

This document describes the current test setup.

## Current automated tests

The project uses CTest.

Current tests:

- crypto_selftest

The crypto self-test verifies:

- libsodium initializes
- placeholder template bytes encrypt successfully
- encrypted blob writes to a temporary file
- encrypted blob reads back
- decrypted bytes match plaintext
- temporary file is removed

## Run tests

Build first:

    ./scripts/build.sh

Run tests:

    ./scripts/test.sh

Equivalent command:

    ctest --test-dir build --output-on-failure

## Camera tests

Camera tests are currently manual because they require real hardware.

Run one-shot camera test:

    ./build/daemon/face-unlockd --camera 0

Run daemon mode:

    ./build/daemon/face-unlockd --camera 0 --daemon

Then in another terminal:

    ./scripts/test-socket-client.sh ping
    ./scripts/test-socket-client.sh camera_status
    ./scripts/test-socket-client.sh auth

## PAM tests

PAM tests are manual for now.

Use the fake PAM service flow only:

    docs/pam-fake-service-test.md

Do not modify sudo, login, lock-screen, GDM, SDDM, LightDM, or common-auth PAM files during automated testing.

## CI

GitHub Actions runs build and dependency checks.

The CI workflow should also run CTest:

    ctest --test-dir build --output-on-failure

## Key/template flow test

CTest includes:

    key_template_flow

This test runs:

    ./scripts/test-key-template-flow.sh

It verifies the development key plus placeholder template flow using a temporary HOME directory.

## Daemon metadata integration test

CTest includes:

    daemon_metadata

This test runs:

    ./scripts/test-daemon-metadata.sh

It uses temporary HOME and XDG_RUNTIME_DIR directories.

It verifies daemon socket responses include:

- template present
- enrollment placeholder
- key present
- decryptability possible_with_dev_key
- key_storage local_development_key_file

The test uses daemon --serve mode and does not require a camera.

## template_status operation test

The daemon_metadata CTest verifies the template_status socket operation.

It checks:

- template present
- enrollment placeholder
- key present
- decryptability possible_with_dev_key
- key_storage local_development_key_file
- template_decrypt ok

No plaintext is returned.

## template_status operation test

The daemon_metadata CTest verifies the template_status socket operation.

It checks:

- template present
- enrollment placeholder
- key present
- decryptability possible_with_dev_key
- key_storage local_development_key_file
- template_decrypt ok

No plaintext is returned.

## Auth reason integration test

CTest includes:

    auth_reasons

This test runs:

    ./scripts/test-auth-reasons.sh

It verifies fail-closed auth reasons:

- template_missing
- template_not_decryptable
- key_missing
- matcher_not_implemented

The test uses daemon --serve mode and temporary HOME/XDG_RUNTIME_DIR directories.

## Python detector smoke test

Run:

    ./scripts/test-python-detectors.sh

This test does not require a camera.

It verifies detector imports, noop backend behavior, and prototype_detect.py CLI help.

## Detector output generation test

Run:

    ./scripts/test-detector-output-generation.sh

This test does not require camera hardware.

It uses prototype_detect.py with:

    --backend noop
    --synthetic-frame 640x480

## Detector self-test

CTest includes:

    detector_selftest

It verifies the C++ NoopFaceDetector returns zero detections.

## detector_status integration

The daemon_metadata test verifies:

    detector_status

Expected current metadata:

    detector noop
    faces_detected 0
