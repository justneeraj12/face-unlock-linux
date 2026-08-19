# Python Embedding Prototype

This document describes the Python embedding prototype.

## Script

    python/prototype_embed.py

## Purpose

The script turns saved face crops into dummy embedding vectors using a tiny randomly initialized PyTorch model.

This is not a real face recognition model.

It exists to prototype:

- image loading
- preprocessing
- embedding-shaped output
- metadata writing
- privacy warnings around derived biometric data

## Input

Default input directory:

    enrollment_samples/crops

This directory is produced by the capture prototype only when explicit privacy consent flags are used.

## Safe behavior

The script can print embedding reports without writing output.

Run:

    python3 python/prototype_embed.py --input-dir enrollment_samples/crops

## Writing embeddings

Embeddings are derived biometric data.

Writing embeddings requires both:

    --write-output
    --i-understand-biometric-risk

Example:

    python3 python/prototype_embed.py --input-dir enrollment_samples/crops --write-output --i-understand-biometric-risk

Default output:

    enrollment_samples/embeddings.json

Do not commit embeddings.

## Git policy

The repository ignores:

    enrollment_samples/
    embeddings.json
    *.npy
    *.npz

## Limitations

Current embeddings are produced by a random stub model.

They are not useful for real identity matching.

Future work:

- load real TorchScript embedding model
- align faces before embedding
- calibrate thresholds
- encrypt enrolled template embeddings
- compare daemon embeddings against encrypted templates
