# PAM Safety Guide

PAM controls authentication on Linux.

A bad PAM configuration can lock you out of:

- sudo
- graphical login
- lock screen
- su
- SSH

This project must treat PAM changes as dangerous.

## Rules

1. Do not manually edit /etc/pam.d/* unless you understand PAM.
2. Always keep a root shell open while testing.
3. Always create backups before changes.
4. Always test with a fake PAM service first.
5. Always keep password authentication as fallback.
6. Never make face unlock the only authentication method.

## Safe testing order

Recommended order:

1. test daemon with socket client
2. test PAM module with fake PAM service
3. test sudo
4. test lock screen
5. only then investigate greeter login

## Fake PAM service

Before touching real services, create a test service like:

    /etc/pam.d/face-unlock-test

Then test with:

    pamtester face-unlock-test "$USER" authenticate

This avoids modifying sudo, GDM, SDDM, or LightDM.

## Rollback principle

Every installer must print rollback instructions before applying changes.

Example rollback:

    sudo cp /etc/pam.d/sudo.backup.TIMESTAMP /etc/pam.d/sudo

## Recommended sudo integration style

When mature, face unlock should be configured as:

    auth sufficient pam_face_unlock.so timeout_ms=700

This means:

- if face auth succeeds, PAM can continue as authenticated
- if face auth fails, PAM falls back to the next method
- password auth remains available

During early development, do not install this into real PAM services.
