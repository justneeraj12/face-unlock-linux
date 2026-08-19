# GitHub Issues Bootstrap

This document describes the starter issue bootstrap script.

## Script

    scripts/bootstrap-issues.sh

Default repo:

    justneeraj12/face-unlock-linux

Run:

    ./scripts/bootstrap-issues.sh

Or specify another repo:

    ./scripts/bootstrap-issues.sh owner/repo

## Requirements

The script requires GitHub CLI:

    gh

Authenticate first:

    gh auth login

## Safety

The script only creates GitHub issues.

It does not modify:

- PAM files
- sudo configuration
- systemd services
- local authentication settings
- local biometric data

## Created issue areas

The script creates starter issues for:

- guarded sudo apply and rollback hardening
- real key management design
- enrollment CLI prototype
- detector evaluation
- embedding model evaluation
- Qt camera preview
- Qt pose guidance
- GUI Forget Me robustness
- lock-screen test matrix
- v1.0 security review checklist

## Idempotency

The script checks existing issue titles before creating issues.

If an issue with the same title already exists, it skips creation.
