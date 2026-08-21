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

## Generator mismatch

CMake build directories cannot switch generators after they are configured.

For example, a directory created with Unix Makefiles cannot later be reused with Ninja.

If you see a generator mismatch error, remove the build directory:

    rm -rf build

or for GUI:

    rm -rf build-gui

Then rebuild:

    ./scripts/build.sh
    ./scripts/build-gui.sh

Alternatively, use a different build directory:

    BUILD_DIR=build-ninja ./scripts/build.sh
    GUI_BUILD_DIR=build-gui-ninja ./scripts/build-gui.sh
