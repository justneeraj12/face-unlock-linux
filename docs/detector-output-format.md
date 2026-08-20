# Detector Output Format

This document describes the planned JSON format for detector prototype outputs.

## Files

Example:

    schemas/detector-output.example.json

Schema scaffold:

    schemas/detector-output.schema.json

## Purpose

The detector output format provides a stable metadata structure for:

- Haar baseline detector
- future YuNet backend
- future RetinaFace/SCRFD backends
- detector evaluation
- GUI overlay prototyping
- daemon detector status design

## Privacy

Detector metadata may be biometric-adjacent.

Generated detector output should not be committed by default.

The format includes:

    "safe_to_commit": false

The output must not include raw images or face crops.

## Required fields

- format
- format_version
- created_at
- backend
- source
- summary
- privacy

## Detection fields

Each detection should include:

- x
- y
- w
- h
- score
- backend

Bounding boxes are in pixel coordinates relative to the source frame.

## Current backend

Current implemented prototype backend:

    haar

Fallback backend:

    noop

Planned backend:

    yunet

## Future use

This format may be used by:

- Python detector evaluation
- GUI preview overlay prototype
- daemon detector status reporting
- model comparison tools

## Writer

python/prototype_detect.py writes this format when called with:

    --write-metadata
    --i-understand-biometric-risk

## Validator

Validate detector output:

    scripts/validate-detector-output.py schemas/detector-output.example.json

The validator checks that raw images and face crops are not embedded and that safe_to_commit is false.
