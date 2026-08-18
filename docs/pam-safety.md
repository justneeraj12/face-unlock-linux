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

1. Do not manually edit `/etc/pam.d/*` unless you understand PAM.
2. Always keep a root shell open while testing.
3. Always create backups before changes.
4. Always test with a fake PAM service first.
5. Always keep password authentication as fallback.
6. Never make face unlock the only authentication method.

## Safe testing order

Recommended order:

1. test daemon with socket client
2. test PAM module with fake PAM service
3. test `sudo`
4. test lock screen
5. only then investigate greeter login

## Fake PAM service

Before touching real services, create a test service like:

```text
/etc/pam.d/face-unlock-test
