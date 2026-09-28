# CPU Face Profile Enrollment

This document describes the first real-model implementation slice.

## Status

Implemented as a Python prototype:

- CPU-only YuNet detector wrapper
- five-landmark detector output
- CPU-only SFace alignment and embedding wrapper
- pose-aware in-memory face profile builder
- guided live enrollment prototype
- pinned model downloader with SHA-256 verification
- camera-free profile builder tests
- optional local CPU model smoke test

This does not enable authentication or write a biometric profile yet.

## Why profile building is not model training

YuNet and SFace are pretrained models. Enrollment does not fine-tune either
network. It processes many live frames, rejects poor or redundant samples, and
builds a small per-user profile from normalized embeddings.

Raw frames and aligned face crops remain in memory and are discarded. The
prototype does not write them to disk.

## Models

The development baseline uses:

- YuNet 2022mar for compatibility with Ubuntu 24.04's OpenCV 4.6
- SFace 2021dec
- OpenCV DNN backend
- OpenCV CPU target

The newer YuNet 2023mar file currently fails inference with the stock OpenCV
4.6 build, so the downloader deliberately pins the compatible 2022mar model.

Download and verify the local ignored model files:

    ./scripts/download-cpu-models.sh

Verify without downloading:

    ./scripts/download-cpu-models.sh --check

Model files are ignored by Git. Before release packages redistribute model
weights, their licenses, provenance, and training-data terms require a separate
review.

## Guided prototype

Run:

    python3 python/prototype_enroll_cpu.py --i-understand-biometric-risk

The prototype asks for center, left, right, up, and down coverage. Progress is
based on accepted pose samples, not elapsed time.

It saves no images, crops, embeddings, or templates. It does not modify PAM or
enable authentication.

## Profile builder

The in-memory builder:

- normalizes embeddings
- rejects exact or near-exact duplicates
- bounds samples retained per pose
- requires coverage for every pose slot
- removes low-similarity outliers within a pose
- creates one normalized centroid per pose

A future daemon implementation will serialize this profile into the existing
libsodium encrypted template container. Plaintext embeddings must never be
written to the enrollment manifest.

## Test

Run:

    ./scripts/test-cpu-face-profile.sh

When verified model files are present, the test also runs YuNet and SFace on
synthetic, non-biometric images.

Run a longer camera-free CPU latency benchmark:

    python3 python/benchmark_cpu_models.py --iterations 100

Use `--json` for machine-readable output. Synthetic input measures model runtime
cost only; it does not measure recognition accuracy or full authentication
latency.

## Current C++ progress

YuNet detection now runs in the C++ daemon on OpenCV's CPU target. The model is
loaded once at startup, and detector status includes boxes and five landmarks.
SFace embedding and encrypted profile storage remain prototype-only.

## Next slice

- benchmark real-camera end-to-end latency and thermal behavior
- port SFace alignment and embedding into the C++ daemon
- calibrate pose, quality, and match thresholds
- add held-out enrollment validation
- expose enrollment start/status/cancel operations
- connect the Qt GUI progress display
- encrypt and atomically commit real templates

PAM authentication remains fail-closed throughout this work.
