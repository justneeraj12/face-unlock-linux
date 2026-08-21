# Fast Builds

This document describes local and CI build speed improvements.

## Tools

The project supports:

- Ninja
- ccache
- parallel CMake builds

## Local setup

Install:

    sudo apt install ninja-build ccache

Or run:

    ./setup-dev.sh

## Build

Run:

    ./scripts/build.sh

The script automatically uses Ninja if available.

The script automatically uses ccache if available.

## Disable ccache

For debugging:

    FACE_UNLOCK_DISABLE_CCACHE=1 ./scripts/build.sh

## Custom build directory

Use:

    BUILD_DIR=build-fast ./scripts/build.sh

## Custom build type

Use:

    CMAKE_BUILD_TYPE=Debug ./scripts/build.sh

or:

    CMAKE_BUILD_TYPE=Release ./scripts/build.sh

## CI

GitHub Actions uses ccache caching for:

- main build workflow
- release workflow
- optional GUI build workflow

CI also uses Ninja when available.

## Notes

The first CI run may still be slow because apt packages need to install.

Subsequent C++ rebuilds should be faster due to ccache.

## GUI builds

Build optional GUI:

    ./scripts/build-gui.sh

Custom GUI build directory:

    GUI_BUILD_DIR=build-gui-fast ./scripts/build-gui.sh
