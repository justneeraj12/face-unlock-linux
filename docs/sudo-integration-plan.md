# sudo Integration Plan

This document describes the planned sudo integration.

This is a planning document only.

Do not modify /etc/pam.d/sudo manually during early development.

## Current status

The project currently supports:

- user daemon
- UNIX socket IPC
- SO_PEERCRED peer logging
- minimal PAM module
- fake PAM service testing
- systemd user service

The project does not yet install into sudo.

## Safety warning

Incorrect PAM configuration can lock you out of sudo.

Before modifying sudo PAM configuration:

- keep a root shell open
- confirm password fallback works
- create a timestamped backup
- print the exact proposed change
- provide one-command rollback
- test with a fake PAM service first
- do not make face unlock the only authentication method

## Intended sudo PAM line

The planned line is:

    auth sufficient pam_face_unlock.so timeout_ms=1000

This should be placed before normal password authentication.

The reason for using sufficient is that a successful module can satisfy authentication, while failure should allow later password authentication to continue.

## Required preconditions

Before sudo integration is allowed:

1. docs/pam-fake-service-test.md has been completed successfully.
2. The daemon runs as a systemd user service.
3. The daemon socket exists at /run/user/$UID/face-unlock.sock.
4. Default auth fails closed.
5. Development auth is not enabled in the installed service.
6. Password authentication fallback is confirmed.
7. A root shell is open for recovery.

## Proposed manual test order

1. Run docs check.

       ./scripts/check-docs.sh

2. Build.

       ./scripts/build.sh

3. Install user service.

       ./scripts/install-user-service.sh

4. Confirm socket.

       ls -l "$XDG_RUNTIME_DIR/face-unlock.sock"

5. Confirm auth fails closed.

       ./scripts/test-socket-client.sh auth

6. Run fake PAM test.

       ./scripts/install-fake-pam-test.sh
       pamtester face-unlock-test "$USER" authenticate
       ./scripts/remove-fake-pam-test.sh

7. Only after all of the above, inspect sudo PAM.

       ./scripts/inspect-sudo-pam.sh

## Rollback principle

Any future sudo installer must create a backup like:

    /etc/pam.d/sudo.face-unlock-backup.TIMESTAMP

Rollback command must be printed before applying changes:

    sudo cp /etc/pam.d/sudo.face-unlock-backup.TIMESTAMP /etc/pam.d/sudo

## Current policy

At this stage, scripts may inspect sudo PAM configuration but must not modify it automatically.
