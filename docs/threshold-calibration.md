# Threshold Calibration

This document describes how matching thresholds should be calibrated.

## Current status

Real matching is not implemented yet.

The daemon currently returns fail-closed auth responses unless development auth is explicitly enabled.

## Why thresholds matter

Face recognition systems compare embeddings using a distance or similarity score.

A threshold determines whether two embeddings are considered the same identity.

Bad thresholds can cause:

- false accepts
- false rejects
- poor usability
- unsafe authentication behavior

## Common metrics

Useful metrics:

- cosine similarity
- Euclidean distance
- false accept rate
- false reject rate
- equal error rate
- receiver operating characteristic curve
- precision/recall under realistic conditions

## Local calibration plan

For a single-user laptop unlock prototype:

1. capture multiple enrollment samples
2. capture genuine verification samples
3. capture negative samples only with consent
4. compute embeddings
5. compare genuine vs negative score distributions
6. choose conservative threshold
7. document false reject behavior
8. require password fallback

## Conservative default

The initial real matcher should prefer:

- lower false accept risk
- higher false reject tolerance
- password fallback after failures

Face unlock should be convenience, not the only factor.

## Attempt limits

The daemon already has max_auth_attempts scaffolding.

Future matching should integrate:

- failed match count
- cooldown period
- fallback reason
- logs without biometric data

## Score logging

Logs may include:

- high-level score
- threshold used
- decision reason

Logs must not include:

- raw images
- face crops
- embeddings
- decrypted templates
- encryption keys

## Test scenarios

Threshold testing should include:

- normal lighting
- low light
- backlight
- different poses
- glasses/no glasses
- facial hair changes
- camera angle changes
- printed photo spoof attempt
- phone screen spoof attempt

## Release requirement

No real authentication release should be published until threshold selection is documented and tested.

The selected threshold must be recorded with:

- model ID
- embedding dimension
- preprocessing
- calibration method
- test environment
