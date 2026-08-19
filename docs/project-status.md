# Project Status

This document summarizes the current implementation status.

## Current phase

The project is currently in pre-release prototype development.

Current target milestone:

    v0.1.0-alpha

## What works today

Implemented:

- documentation-first open-source repository
- CMake build
- GitHub Actions CI on Ubuntu 24.04
- CTest support
- Debian package skeleton with CPack
- C++ user daemon
- OpenCV camera one-shot probe
- OpenCV camera loop mode
- camera worker thread
- UNIX domain socket server
- socket permissions set to 0600
- SO_PEERCRED peer credential logging
- same-UID socket peer policy
- JSON-ish socket operations
- default fail-closed auth operation
- development-only auth gate
- max auth attempt enforcement
- minimal PAM IPC module
- fake PAM service test flow
- fake PAM install/remove scripts
- systemd user service install/remove scripts
- sudo PAM inspection script
- libsodium encrypted template storage scaffold
- crypto self-test
- Python safe capture prototype
- TorchScript export stub
- optional LibTorch loader scaffold

## What does not work yet

Not implemented yet:

- real face recognition authentication
- real face detector in daemon
- face alignment
- encrypted template enrollment
- encrypted template matching
- threshold calibration
- liveness/spoof resistance
- Qt enrollment GUI
- production sudo installer
- lock-screen integration
- greeter/login integration
- release-grade Debian maintainer scripts

## Safety status

Safe development flows exist for:

- building locally
- running daemon manually
- running daemon as user service
- testing IPC
- testing PAM with fake PAM service
- rolling back fake PAM test artifacts
- building Debian package

Unsafe or not-yet-supported flows:

- modifying /etc/pam.d/sudo automatically
- modifying GDM, SDDM, LightDM, or common-auth
- using development auth as real authentication
- using the project as the only authentication factor

## Current authentication behavior

Default auth behavior:

    status fail
    reason auth_not_implemented

Development-only auth can be enabled manually with:

    FACE_UNLOCK_DEV_ALLOW=1

This is for testing only.

It must never be used as real authentication.

## Current package behavior

The Debian package skeleton installs project files but does not automatically enable authentication integration.

Installing the package does not modify real PAM service files.

## First alpha release

The first alpha release has been tagged as:

    v0.1.0-alpha

This release is an infrastructure prototype.

It is not real biometric authentication yet.
