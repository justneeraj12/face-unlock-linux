# sudo Safe Installer Plan

This document describes the planned safe sudo integration flow.

sudo integration is not automatically installed yet.

## Current status

The current script is dry-run only:

    ./scripts/plan-sudo-pam-install.sh

It does not modify /etc/pam.d/sudo.

## Purpose

The dry-run planner:

- reads /etc/pam.d/sudo
- finds a likely insertion point
- creates a proposed modified copy in /tmp
- prints a unified diff
- prints backup command
- prints rollback command
- makes no system changes

## Planned PAM line

The planned sudo PAM line is:

    auth sufficient pam_face_unlock.so timeout_ms=1000

The line should appear before the normal password authentication path, usually before:

    @include common-auth

or before:

    pam_unix.so

## Why sufficient

The module is intended to be sufficient, not required.

This means:

- if face auth succeeds, sudo can authenticate
- if face auth fails, password fallback can continue
- face unlock should not become the only authentication method

## Required preconditions before any future apply script

Before modifying /etc/pam.d/sudo:

1. Keep an already-authenticated root shell open.
2. Confirm the daemon runs as a user service.
3. Confirm the socket exists.
4. Confirm default auth fails closed.
5. Confirm fake PAM service testing works.
6. Confirm password fallback works.
7. Create a timestamped backup.
8. Print the exact diff.
9. Print rollback command before applying.
10. Require explicit YES confirmation.

## Dry run

Run:

    ./scripts/plan-sudo-pam-install.sh

Expected behavior:

- prints current sudo PAM file
- prints proposed insertion point
- prints diff
- prints backup and rollback commands
- makes no changes

## Rollback principle

Any future apply script must create a backup like:

    /etc/pam.d/sudo.face-unlock-backup.TIMESTAMP

Rollback:

    sudo cp /etc/pam.d/sudo.face-unlock-backup.TIMESTAMP /etc/pam.d/sudo

## Current policy

At this stage:

- dry-run planning is allowed
- automatic sudo modification is not implemented
- manual editing of /etc/pam.d/sudo is discouraged
