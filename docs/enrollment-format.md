# Enrollment Format

This document describes the planned enrollment manifest format.

The manifest is metadata only.

It should not contain raw face images.

## Current status

The project currently has:

- encrypted placeholder template CLI
- encrypted template storage scaffold
- Python capture prototype
- Python embedding prototype
- TorchScript model export stub

Real enrollment is not implemented yet.

## Files

Example manifest:

    schemas/enrollment-manifest.example.json

Schema scaffold:

    schemas/enrollment-manifest.schema.json

Future per-user manifest path:

    ~/.local/share/face-unlock/enrollment.json

Future encrypted template path:

    ~/.local/share/face-unlock/template.enc

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
