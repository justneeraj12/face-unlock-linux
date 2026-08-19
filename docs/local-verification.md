# Local Verification

This document describes the local verification script.

## Script

Run:

    ./scripts/verify-local.sh

## Purpose

The verification script checks the local development tree before release or larger changes.

It runs:

- documentation checks
- project build
- CTest
- crypto self-test
- daemon CLI smoke checks
- PAM module dependency audit
- Debian package build
- Debian package metadata inspection
- Debian package contents preview

## Safety

The verification script does not modify PAM configuration.

It does not modify:

    /etc/pam.d/sudo
    /etc/pam.d/gdm-password
    /etc/pam.d/sddm
    /etc/pam.d/lightdm
    /etc/pam.d/common-auth

It does not:

- install pam_face_unlock.so
- enable systemd services
- change sudo authentication
- change login authentication
- change lock-screen authentication

## PAM dependency audit

The PAM module must stay minimal.

The verification script fails if pam_face_unlock.so links to heavy or disallowed libraries such as:

- OpenCV
- Torch
- CUDA
- Qt
- TensorFlow
- libsodium

Allowed dependencies include:

- libpam
- libc
- libaudit
- libcap-ng

## Package verification

The script builds the Debian package with CPack and shows:

    dpkg-deb -I build/*.deb
    dpkg-deb -c build/*.deb

Review package contents before installing any .deb.

## Release usage

Before v0.1.0-alpha, run:

    ./scripts/verify-local.sh

A clean release candidate should pass this script and GitHub Actions.
