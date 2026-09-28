# Daemon CPU Detector Backends

This document describes the C++ detector pipeline used by the per-user daemon.

## Current status

Implemented:

- detector interface shared by all backends
- always-available noop backend
- optional Haar baseline
- optional YuNet ONNX backend through OpenCV DNN
- forced OpenCV CPU inference target
- one-time model loading at daemon startup
- bounding boxes, confidence, five landmarks, and latency metadata
- camera-free self-tests and socket integration tests

YuNet is the real detector candidate. Haar remains a compatibility baseline and
is not considered production-quality face detection.

Face recognition and authentication matching are not enabled by this work.
Authentication remains fail-closed.

## Build requirements

YuNet is compiled when OpenCV core, imgproc, objdetect, and dnn development
libraries are available. It does not require CUDA, a discrete GPU, or
vendor-specific Intel/AMD acceleration.

List the compiled backends:

    ./build/daemon/face-unlock-detector-selftest

## Model setup

Download the pinned, checksum-verified development models:

    ./scripts/download-cpu-models.sh

The YuNet development model is written to:

    models/face_detection_yunet_2022mar.onnx

Model files are ignored by Git. Release packaging and redistribution require a
separate model-license and provenance review.

## YuNet self-test

Run camera-free CPU inference on a synthetic blank frame:

    ./build/daemon/face-unlock-detector-selftest --yunet-model models/face_detection_yunet_2022mar.onnx

A successful result includes:

    supported_backend: yunet
    yunet_status: ok

## Daemon configuration

Noop remains the safe default:

    ./build/daemon/face-unlockd --detector noop --serve

Run YuNet explicitly:

    ./build/daemon/face-unlockd --detector yunet --detector-model models/face_detection_yunet_2022mar.onnx --serve

Equivalent config fields:

    "detector_backend": "yunet"
    "detector_model_path": "/absolute/path/to/face_detection_yunet_2022mar.onnx"

A missing model, unreadable model, unsupported backend, or model-load failure
causes startup to fail. YuNet is loaded once and reused instead of being loaded
for each socket request.

## detector_status operation

Query detector metadata:

    ./scripts/test-socket-client.sh detector_status

No-camera example:

    {
      "status": "ok",
      "op": "detector_status",
      "detector": "yunet",
      "detector_status": "ready",
      "faces_detected": 0,
      "detector_ms": 0,
      "detections": []
    }

With a camera frame, every detection contains:

- x, y, w, h
- score
- landmarks, containing five x/y points

Non-finite detections and boxes with non-positive dimensions are discarded.
Inference exceptions return detector status error and do not crash the daemon.

## Haar behavior

Haar is compiled only when OpenCV objdetect and imgproc are present. At runtime,
the cascade must exist in a standard OpenCV data directory.

The integration test reports:

    haar_backend_status: ok
    haar_backend_status: skipped_not_supported
    haar_backend_status: skipped_cascade_missing

## Tests

Run:

    ./scripts/test-detector-backends.sh

The test always verifies noop and unsupported-backend failure. It also checks
that YuNet without a model path fails closed. When the pinned local model exists,
it runs real CPU inference and starts the socket server with YuNet. Model-free CI
reports:

    yunet_backend_status: skipped_model_missing

CTest runs the same integration script as:

    detector_backends

## Privacy and safety

The detector does not save frames or crops. Detection metadata is
biometric-adjacent and must not be persisted or logged by default.

Detector success alone must never authenticate a user. Recognition thresholds,
liveness, held-out validation, retry limits, and encrypted profile loading must
all succeed before an authentication result can become successful.
