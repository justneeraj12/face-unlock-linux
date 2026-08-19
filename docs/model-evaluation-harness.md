# Model Evaluation Harness

This document describes the Python model evaluation harness scaffold.

## Script

    python/evaluate_model_stub.py

## Current status

The script uses the random EmbeddingStub model.

It is not real face recognition.

## Purpose

The harness defines the future evaluation workflow:

- load local face crops
- preprocess images
- compute embeddings
- measure inference time
- compute pairwise similarity
- summarize metrics
- optionally write metrics JSON

## Safe default

By default, the script saves nothing.

Run:

    python3 python/evaluate_model_stub.py --input-dir enrollment_samples/crops

## Writing metrics

Metrics are derived from biometric images.

Writing metrics requires:

    --write-metrics
    --i-understand-biometric-risk

Example:

    python3 python/evaluate_model_stub.py --input-dir enrollment_samples/crops --write-metrics --i-understand-biometric-risk

Default output:

    enrollment_samples/model_eval_metrics.json

Do not commit generated metrics.

## Current metrics

The harness reports:

- image count
- embedding dimension
- total runtime
- inference time summary
- pairwise cosine similarity summary

## Limitations

Current model is random.

Metrics are useful only for testing the evaluation pipeline.

Future work:

- load candidate detector
- load candidate embedding model
- compare genuine and negative samples
- compute threshold curves
- export calibration reports

## Metrics format

The planned metrics JSON format is documented in:

    docs/model-evaluation-metrics.md

Schema scaffold:

    schemas/model-eval-metrics.schema.json

Example:

    schemas/model-eval-metrics.example.json
