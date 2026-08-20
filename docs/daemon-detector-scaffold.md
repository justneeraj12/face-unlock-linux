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
