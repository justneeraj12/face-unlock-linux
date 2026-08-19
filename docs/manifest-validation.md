# Manifest Validation

This document describes enrollment manifest validation.

## Validator

Script:

    scripts/validate-enrollment-manifest.py

## Purpose

The validator checks basic safety and consistency of enrollment manifests.

It uses only the Python standard library.

## Current checks

The validator checks:

- JSON parses
- root object exists
- format is face-unlock-enrollment-manifest
- format_version is 1
- user.uid exists and is non-negative
- model fields exist
- template fields exist
- template.contains_raw_images is false
- privacy.raw_images_saved is false
- privacy.face_crops_saved is false
- privacy.telemetry_enabled is false
- placeholder manifests are not marked as real enrollments

## Validate example manifest

Run:

    scripts/validate-enrollment-manifest.py schemas/enrollment-manifest.example.json

Expected:

    manifest_validation_status: ok

## Validate generated placeholder manifest

Create placeholder:

    ./build/daemon/face-unlock-template-tool create-placeholder --i-understand-placeholder --overwrite

Validate:

    scripts/validate-enrollment-manifest.py ~/.local/share/face-unlock/enrollment.json

Delete placeholder:

    ./build/daemon/face-unlock-template-tool delete --yes

## Limitations

This is not full JSON Schema validation.

The JSON schema scaffold remains in:

    schemas/enrollment-manifest.schema.json

Future work may add full schema validation as an optional development dependency.

## CI integration

GitHub Actions validates the example enrollment manifest during the build workflow.

CI runs:

    ./scripts/check-json.sh
    scripts/validate-enrollment-manifest.py schemas/enrollment-manifest.example.json
