# Model Evaluation Plan

This document describes how face detection, alignment, and embedding models should be evaluated before integration.

## Current status

The project currently has:

- Python capture prototype
- Python embedding prototype using a random stub model
- TorchScript export stub
- optional LibTorch loader scaffold

Real face recognition models are not integrated yet.

## Goals

Model integration must support:

- local-only inference
- low latency
- clear licensing
- documented preprocessing
- reproducible export
- safe threshold calibration
- encrypted template matching
- no raw image telemetry

## Model categories

The future biometric pipeline may need:

1. face detector
2. landmark/alignment model
3. embedding model
4. optional liveness/spoof model

## Candidate detector types

Possible detector candidates:

- OpenCV Haar or LBP cascades for baseline only
- OpenCV YuNet
- MTCNN
- RetinaFace
- MediaPipe face detection
- SCRFD-style detectors

Selection criteria:

- license compatibility
- TorchScript/ONNX export path
- CPU performance
- CUDA performance
- robustness to lighting
- robustness to webcam quality
- landmark availability
- model size
- maintenance status

## Candidate embedding model types

Possible embedding candidates:

- MobileFaceNet-style models
- ArcFace-style backbones
- FaceNet-style models
- lightweight ResNet embeddings
- ONNX/TorchScript-compatible open models

Selection criteria:

- license compatibility
- embedding dimension
- preprocessing requirements
- alignment requirements
- threshold calibration evidence
- CPU latency
- GPU latency
- false accept behavior
- false reject behavior

## License requirements

Before adding any model artifact or export script, document:

- source repository
- model license
- training dataset notes if available
- redistribution permission
- commercial-use restrictions if any
- citation requirements

Do not commit model artifacts unless redistribution is clearly allowed.

## Export requirements

Every model export path must document:

- original format
- export command
- expected input shape
- color order
- normalization
- output tensor shape
- TorchScript or ONNX opset
- tested runtime
- CPU/GPU compatibility

## Evaluation data policy

Evaluation data must not be committed.

Ignored paths include:

    enrollment_samples/
    captures/
    templates/
    *.enc
    *.npy
    *.npz

Any dataset or sample collection must be opt-in and local-only.

## Metrics

Track at minimum:

- detection success rate
- embedding latency
- end-to-end auth latency
- false accept rate
- false reject rate
- threshold selected
- lighting sensitivity
- pose sensitivity
- glasses/hat/occlusion sensitivity

## Latency targets

Initial target hardware:

- Intel i5-12500H
- NVIDIA RTX 3050 Ti
- Ubuntu 24.04

Initial targets:

- daemon warm start
- camera already running
- auth request under 500 ms
- detector cadence optimized
- embedding inference under 100 ms on GPU where possible

## Integration stages

Recommended stages:

1. Python-only evaluation
2. TorchScript export
3. standalone C++ model-test
4. daemon model load at startup
5. daemon embedding inference on latest frame
6. encrypted template matching
7. PAM fake-service validation
8. guarded sudo validation

## Anti-spoofing note

Basic face recognition is vulnerable to presentation attacks such as photos and videos.

Before production use, evaluate:

- challenge-response prompts
- blink/head movement checks
- texture/liveness models
- hardware support if available
- fallback requirements

This project must not claim strong biometric security without liveness evaluation.

## Candidate shortlist

Candidate detectors and embedding models are tracked in:

    docs/model-candidates.md

No model is selected yet.

Model artifacts must not be committed until license and redistribution rights are reviewed.

## Evaluation harness scaffold

The initial Python evaluation harness is:

    python/evaluate_model_stub.py

It currently uses a random stub model and defines the metrics/reporting flow.

Documentation:

    docs/model-evaluation-harness.md
