# Development Setup

This document explains how to set up a development environment for face-unlock-linux on Ubuntu 24.04.

## Safety

The development setup script installs packages only.

It does not:

- modify /etc/pam.d/*
- install pam_face_unlock.so
- enable the systemd user service
- change sudo authentication
- change login authentication
- change lock-screen authentication

## Supported development platform

Primary target:

- Ubuntu 24.04 LTS
- x86_64
- internal or USB webcam

## Install dependencies

Run:

    ./setup-dev.sh

The script asks for confirmation before installing packages.

## Packages installed

The script installs:

    build-essential
    cmake
    pkg-config
    git
    libopencv-dev
    python3-opencv
    libpam0g-dev
    pamtester
    v4l-utils

## Verify OpenCV

Check C++ OpenCV pkg-config metadata:

    pkg-config --modversion opencv4

Check Python OpenCV:

    python3 -c 'import cv2; print(cv2.__version__)'

## Check camera devices

Run:

    v4l2-ctl --list-devices

or:

    ls -l /dev/video*

## Build

Run:

    ./scripts/build.sh

## Run daemon one-shot camera test

Run:

    ./build/daemon/face-unlockd --camera 0

Expected important output:

    camera_status: opened
    frame_status: ok
    status: ok

## Run daemon mode

Run:

    ./build/daemon/face-unlockd --camera 0 --daemon

In another terminal:

    ./scripts/test-socket-client.sh ping
    ./scripts/test-socket-client.sh camera_status
    ./scripts/test-socket-client.sh auth

Default auth should fail closed:

    status fail
    reason auth_not_implemented

## PAM fake service test

See:

    docs/pam-fake-service-test.md

## systemd user service

See:

    docs/systemd-user-service.md

## sudo integration

sudo integration is not installed yet.

See:

    docs/sudo-integration-plan.md

## Minimal OpenCV development packages

For a minimal daemon build, these packages are sufficient:

    sudo apt install libopencv-core-dev libopencv-videoio-dev

The full development setup script may install libopencv-dev for convenience on developer machines.

CI uses the minimal packages to reduce install time.
