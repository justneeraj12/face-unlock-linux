# Models

This directory documents local model artifacts.

Large model files are intentionally not committed to Git.

Ignored examples:

- .pt
- .onnx
- .engine
- TensorRT engines

## Current stub model

A tiny TorchScript embedding stub can be generated with:

    python3 python/export_torchscript_stub.py

Default output:

    models/embedding_stub.pt

This is not a real face recognition model.

It is only used to test model export and future TorchScript loading.

## Git policy

Model artifacts must not be committed by default.

The repository ignores:

    models/*.pt
    models/*.onnx
    models/*.engine

## Future model requirements

Future real models should document:

- source
- license
- expected input size
- preprocessing
- embedding dimension
- threshold calibration method
- CPU/GPU performance
- privacy/security considerations
