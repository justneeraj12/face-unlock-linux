# Development Guide

This document tracks the planned local development flow.

## Target OS

Ubuntu 24.04 LTS.

## Planned dependencies

Native:

- build-essential
- cmake
- ninja-build
- pkg-config
- libopencv-dev
- libpam0g-dev
- libsodium-dev
- qt6-base-dev
- qt6-declarative-dev

Python:

- opencv-python
- torch
- torchvision
- numpy

GPU:

- NVIDIA driver
- CUDA
- optional LibTorch CUDA build

## Development phases

We will build the project in phases:

1. repository/docs only
2. daemon without PAM
3. PAM test module without system integration
4. encrypted templates
5. GUI enrollment
6. packaging
7. greeter integration

## Safety rule

No development step should require changing system PAM files until the fake PAM service flow works.
