# Packaging

This document describes Debian package generation.

## Current status

The project uses CPack to generate a basic .deb package.

The package installs:

- face-unlockd
- face-unlock-crypto-selftest
- pam_face_unlock.so
- documentation
- helper scripts
- systemd user service template

## Safety

The package does not automatically modify PAM service files.

It does not modify:

    /etc/pam.d/sudo
    /etc/pam.d/gdm-password
    /etc/pam.d/sddm
    /etc/pam.d/lightdm
    /etc/pam.d/common-auth

It does not automatically enable the user service.

It does not configure sudo, lock-screen, login, GDM, SDDM, or LightDM.

## Build package

Run:

    ./scripts/package-deb.sh

The script runs:

- documentation checks
- normal build
- automated tests
- CPack package generation

## Inspect package

Before installing, inspect package contents:

    dpkg-deb -c build/*.deb

Inspect package metadata:

    dpkg-deb -I build/*.deb

## Install package manually

Only after inspection:

    sudo apt install ./build/<package-name>.deb

## Remove package

    sudo apt remove face-unlock-linux

## PAM safety

Installing the package places pam_face_unlock.so in the PAM module directory.

This alone does not enable face unlock.

A PAM service must explicitly reference pam_face_unlock.so before it is used.

Do not edit real PAM service files manually.

Use fake PAM service testing first:

    docs/pam-fake-service-test.md

## User service

The package includes the service template under documentation/shared data paths.

The current recommended development flow still uses:

    ./scripts/install-user-service.sh

Future packages may provide a safer packaged helper command.

## CI package build

GitHub Actions builds the Debian package on Ubuntu 24.04.

The CI artifact is named:

    face-unlock-linux-deb-ubuntu-24.04

Download it from the workflow run artifacts.

Always inspect package contents before installing:

    dpkg-deb -c <package>.deb
    dpkg-deb -I <package>.deb

## Package workflow

Development package artifacts are built by:

    .github/workflows/package.yml

The package workflow can be run manually from GitHub Actions.

It also runs for version tags.
