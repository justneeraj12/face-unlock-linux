# CI Dependencies

This document describes shared CI dependency installation.

## Script

    scripts/ci-install-deps.sh

## Modes

Core build dependencies:

    ./scripts/ci-install-deps.sh core

GUI build dependencies:

    ./scripts/ci-install-deps.sh gui

## Core packages

Core mode installs:

- build-essential
- cmake
- ninja-build
- ccache
- pkg-config
- file
- libopencv-core-dev
- libopencv-videoio-dev
- libpam0g-dev
- libsodium-dev

## GUI packages

GUI mode includes all core packages plus:

- qt6-base-dev

## Workflows using this script

Core mode:

- .github/workflows/build.yml
- .github/workflows/package.yml
- .github/workflows/release.yml

GUI mode:

- .github/workflows/gui-build.yml

## Why this exists

The workflows previously duplicated apt setup logic.

A shared script keeps dependency changes consistent across workflows.
