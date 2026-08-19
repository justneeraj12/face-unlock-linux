# Fake PAM Service Test

This document explains how to test pam_face_unlock.so safely without modifying sudo, GDM, SDDM, LightDM, lock-screen, or login PAM files.

## Purpose

The fake PAM service test verifies:

- pam_face_unlock.so loads correctly
- the PAM module connects to the user daemon socket
- the PAM module sends an auth request
- the daemon returns a response
- default auth fails closed
- password fallback still works
- development-only auth can satisfy PAM when explicitly enabled

This test uses a dedicated fake PAM service:

    /etc/pam.d/face-unlock-test

This service is only invoked when explicitly running pamtester.

## Safety warning

Do not modify these files during this test:

    /etc/pam.d/sudo
    /etc/pam.d/gdm-password
    /etc/pam.d/sddm
    /etc/pam.d/lightdm
    /etc/pam.d/common-auth

This test does not require editing real login or sudo authentication stacks.

## Prerequisites

Build the project:

    ./scripts/build.sh

Install pamtester:

    sudo apt update
    sudo apt install pamtester

Verify the PAM module exists:

    ls -l build/pam/pam_face_unlock.so

Audit dependencies:

    ldd build/pam/pam_face_unlock.so

Expected dependency rule:

- libpam is okay
- libc is okay
- libaudit and libcap-ng are okay
- OpenCV must not appear
- Torch must not appear
- Qt must not appear
- CUDA must not appear

## Install module for fake test

Install the PAM module into the system PAM module directory:

    sudo install -m 0644 build/pam/pam_face_unlock.so /usr/lib/x86_64-linux-gnu/security/pam_face_unlock.so

Verify:

    ls -l /usr/lib/x86_64-linux-gnu/security/pam_face_unlock.so

## Create fake PAM service

Create the fake service:

    sudo tee /etc/pam.d/face-unlock-test >/dev/null <<'PAM_EOF'
    auth sufficient pam_face_unlock.so timeout_ms=1000 debug
    auth required pam_unix.so
    account required pam_unix.so
    PAM_EOF

Verify:

    cat /etc/pam.d/face-unlock-test

Expected content:

    auth sufficient pam_face_unlock.so timeout_ms=1000 debug
    auth required pam_unix.so
    account required pam_unix.so

## Test 1: Default fail-closed behavior

Start the daemon without development auth:

    ./build/daemon/face-unlockd --camera 0 --daemon

Expected daemon startup includes:

    dev_auth_enabled: false

In another terminal, run:

    pamtester face-unlock-test "$USER" authenticate

Expected behavior:

- pam_face_unlock.so sends an auth request to the daemon
- daemon returns status fail for op auth
- PAM falls back to pam_unix
- pamtester asks for your password
- entering the correct password succeeds

This confirms the module fails closed and password fallback still works.

## Test 2: Development-only auth success

Stop the daemon with Ctrl+C.

Restart with development-only auth enabled:

    FACE_UNLOCK_DEV_ALLOW=1 ./build/daemon/face-unlockd --camera 0 --daemon

Expected daemon startup includes:

    dev_auth_enabled: true

In another terminal, run:

    pamtester face-unlock-test "$USER" authenticate

Expected behavior:

- pam_face_unlock.so sends an auth request to the daemon
- daemon returns status ok for op auth if camera is ready
- pamtester succeeds without requiring password

This confirms the IPC path can satisfy PAM when explicitly enabled for development.

## Debug logs

PAM debug logs may be visible with:

    journalctl -n 100 --no-pager | grep face_unlock

or:

    sudo journalctl -n 100 --no-pager | grep face_unlock

## Rollback

Remove the fake PAM service and installed module:

    sudo rm -f /etc/pam.d/face-unlock-test
    sudo rm -f /usr/lib/x86_64-linux-gnu/security/pam_face_unlock.so

Verify rollback:

    test ! -e /etc/pam.d/face-unlock-test && echo "fake PAM service removed"
    test ! -e /usr/lib/x86_64-linux-gnu/security/pam_face_unlock.so && echo "PAM module removed"

## Current limitations

This is not real biometric authentication yet.

The development-only auth path is controlled by:

    FACE_UNLOCK_DEV_ALLOW=1

This must never be used as real authentication.

Production auth must remain fail-closed until real enrollment, encrypted templates, matching, retry limits, and liveness checks are implemented.
