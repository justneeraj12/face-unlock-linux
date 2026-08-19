# Model Export

This document describes model export scaffolding.

## Current status

The project includes a tiny TorchScript embedding stub exporter.

Script:

    python/export_torchscript_stub.py

Default output:

    models/embedding_stub.pt

This is not a real face recognition model.

It exists to test:

- Python export workflow
- TorchScript artifact creation
- future C++ TorchScript model loading
- embedding-shaped output

## Install Torch dependencies

Torch is intentionally kept separate from lightweight prototype dependencies.

Option 1: system/user Python environment:

    pip install -r python/requirements-torch.txt

Option 2: virtual environment:

    python3 -m venv .venv
    source .venv/bin/activate
    pip install --upgrade pip
    pip install -r python/requirements-torch.txt

## Export stub

Run:

    python3 python/export_torchscript_stub.py

Expected output:

    export_status: ok
    verify_status: ok
    status: ok

Generated file:

    models/embedding_stub.pt

## Git policy

TorchScript model files are ignored by Git.

Do not commit generated model files by default.

## Future real model export

Future real model export should document:

- model architecture
- source repository
- license
- training dataset constraints
- input preprocessing
- face alignment requirements
- output embedding dimension
- threshold calibration
- liveness limitations
- CPU/CUDA/TensorRT performance
