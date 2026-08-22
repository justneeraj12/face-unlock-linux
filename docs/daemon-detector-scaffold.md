# Daemon Detector Scaffold

This document describes the C++ daemon detector abstraction scaffold.

## Current status

Implemented:

- DetectionBox
- DetectorResult
- FaceDetector interface
- NoopFaceDetector
- detector self-test
- daemon metadata fields

No real detector is implemented yet.

## Current daemon metadata

Daemon socket responses include:

    "detector":"noop"
    "faces_detected":0

## Self-test

Run:

    ./build/daemon/face-unlock-detector-selftest

CTest includes:

    detector_selftest

## Future work

Planned detector backends:

- OpenCV Haar baseline
- OpenCV YuNet
- TorchScript/ONNX detector
- SCRFD/RetinaFace-style detector

Future detector metadata should include:

- backend name
- face count
- bounding boxes
- confidence scores
- detector latency
- quality flags

## Safety

The current Noop detector does not inspect or save images.

Real detector integration must not log raw frames or store images by default.

## detector_status operation

The daemon supports a dedicated socket operation:

    detector_status

Run:

    ./scripts/test-socket-client.sh detector_status

Current fields:

    detector noop
    faces_detected 0

## Detector backend configuration

Current supported backend:

    noop

CLI:

    ./build/daemon/face-unlockd --detector noop --serve

Config:

    "detector_backend": "noop"

Unsupported detector backends fail safely at startup.

## Haar backend scaffold

The daemon can optionally support a Haar detector backend when OpenCV objdetect/imgproc development libraries are available.

CLI:

    ./build/daemon/face-unlockd --detector haar --serve

Current behavior:

- noop is always supported
- haar is supported only if OpenCV objdetect is available at build time
- unsupported detector backends fail safely at startup

Haar is a baseline detector only and is not considered production quality.

## Detector backend integration test

Run:

    ./scripts/test-detector-backends.sh

CTest also runs this as:

    detector_backends
