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
