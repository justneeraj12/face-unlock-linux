# Enrollment Format

This document describes the planned enrollment manifest format.

The manifest is metadata only.

It should not contain raw face images.

## Current status

The project currently has:

- encrypted placeholder template CLI and storage scaffold
- native CPU detection and embedding
- native five-pose profile construction and score-only matching
- a strict version-1 binary profile payload
- libsodium encrypted profile round-trip tests
- a guided Python enrollment prototype

The daemon does not create or persist a real biometric profile yet.

## Files

Example manifest:

    schemas/enrollment-manifest.example.json

Schema scaffold:

    schemas/enrollment-manifest.schema.json

Future per-user manifest path:

    ~/.local/share/face-unlock/enrollment.json

Future encrypted template path:

    ~/.local/share/face-unlock/template.enc

## Encrypted profile payload version 1

The plaintext payload is an internal binary format. It is passed directly to
libsodium secretbox encryption and must never be written separately.

All integers and IEEE-754 float32 values use little-endian byte order.

| Field | Constraint |
|---|---|
| Magic | 8 bytes: `FULPRF1\0` |
| Version | unsigned 16-bit value `1` |
| Flags | zero in version 1 |
| Model ID | 1-128 ASCII letters, digits, dot, underscore, or hyphen |
| Embedding dimension | 1-4096 |
| Pose templates | exactly center, left, right, up, down in that order |
| Sample count | 1-64 per pose |
| Centroid | finite, L2-normalized float32 vector |

The parser rejects unknown versions or flags, invalid dimensions, missing or
reordered poses, non-finite or non-normalized centroids, truncation, and
trailing data. The format contains no raw images, crops, threshold, username,
UID, or encryption key.

An acceptance threshold is intentionally not stored in version 1. Thresholds
are policy derived from evaluation data, not biometric template content.

## Design goals

The enrollment format should record:

- format version
- creation time
- user UID and username
- model identity
- embedding dimension
- preprocessing requirements
- encrypted template path
- encryption method
- quality metadata
- pose coverage
- privacy flags
- enrollment completion status

## Privacy rules

The manifest must not contain:

- raw images
- face crops
- unencrypted embeddings
- encryption keys
- decrypted template contents

The manifest may contain:

- model IDs
- embedding dimension
- quality scores
- pose slot completion
- encrypted template path
- privacy flags

## Example

See:

    schemas/enrollment-manifest.example.json

Important example fields:

    "contains_raw_images": false
    "raw_images_saved": false
    "face_crops_saved": false
    "telemetry_enabled": false

## Status fields

The status object tracks whether enrollment is real or placeholder-only.

During current development:

    "enrollment_complete": false
    "real_biometric_template": false
    "placeholder_only": true

Future real enrollment should set:

    "enrollment_complete": true
    "real_biometric_template": true
    "placeholder_only": false

only after real encrypted embeddings/templates exist.

## Quality metadata

Planned quality fields include:

- samples_total
- pose slot coverage
- luma statistics
- sharpness score
- face detection confidence
- alignment quality
- occlusion warnings

## Pose slots

Planned pose slots:

- center
- left
- right
- up
- down

Future GUI enrollment should guide the user through these slots.

## Key management

The manifest does not store encryption keys.

Future key storage options:

- kernel keyring
- GNOME Keyring
- passphrase-wrapped local key
- hardware-backed secret storage where available

## Future work

Planned next steps:

- generate placeholder enrollment manifest from CLI
- write manifest with mode 0600
- delete manifest with template deletion
- connect Qt enrollment GUI to manifest format
- encrypt real embedding templates
- validate manifest against schema in tests

## Placeholder manifest writer

The template CLI now writes a placeholder enrollment manifest when creating a placeholder template.

Command:

    ./build/daemon/face-unlock-template-tool create-placeholder --i-understand-placeholder

Files written:

    ~/.local/share/face-unlock/template.enc
    ~/.local/share/face-unlock/enrollment.json

Both files should have mode 0600.

This is still not real enrollment.

## Daemon enrollment status

The daemon now reads the enrollment manifest status from:

    ~/.local/share/face-unlock/enrollment.json

Socket responses include an enrollment field:

    "enrollment":"missing"
    "enrollment":"placeholder"
    "enrollment":"real"
    "enrollment":"present_unknown"
    "enrollment":"unreadable"

Current placeholder enrollment should report:

    "enrollment":"placeholder"

Real enrollment is still not implemented.
