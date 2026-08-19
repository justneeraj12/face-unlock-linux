# Model Evaluation Metrics Format

This document describes the planned JSON metrics format for model evaluation.

## Files

Example:

    schemas/model-eval-metrics.example.json

Schema scaffold:

    schemas/model-eval-metrics.schema.json

## Purpose

The metrics format records local-only model evaluation results.

It is intended for:

- detector/embedding experiments
- latency measurement
- pairwise similarity summaries
- threshold calibration planning
- privacy-safe reporting

## Privacy

Metrics may be derived from biometric images.

Generated metrics should not be committed by default.

The example schema includes:

    "safe_to_commit": false

because real metrics may reveal information about biometric samples or evaluation data.

## Required sections

The format includes:

- format
- format_version
- created_at
- model
- dataset
- performance
- similarity
- threshold
- privacy
- warnings

## Model section

Records:

- model_id
- model_type
- real_face_recognition
- embedding_dim

## Dataset section

Records:

- source
- images_count
- contains_biometric_data
- committed_to_git

## Performance section

Records timing summaries such as:

- total_ms
- inference_ms min/max/mean/median

## Similarity section

Records pairwise similarity summaries.

Initial metric:

    cosine_similarity

## Threshold section

Records threshold selection.

Current status:

    not_calibrated

Future real model evaluation must record:

- selected threshold
- calibration method
- false accept behavior
- false reject behavior

## Future work

Future scripts should write this format when evaluation metrics are explicitly requested with privacy consent.
