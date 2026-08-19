# Release Process

This document describes the manual release process.

## Current target

Current planned release:

    v0.1.0-alpha

Checklist:

    docs/releases/v0.1.0-alpha.md

## Safety

Release scripts do not modify PAM configuration.

They do not modify:

    /etc/pam.d/sudo
    /etc/pam.d/gdm-password
    /etc/pam.d/sddm
    /etc/pam.d/lightdm
    /etc/pam.d/common-auth

## Prepare release

Run:

    ./scripts/prepare-release.sh v0.1.0-alpha

The script verifies:

- working tree is clean
- local verification passes
- package builds
- tests pass
- PAM dependency audit passes

The script does not create or push tags automatically.

## Manual tag

After prepare-release succeeds, create an annotated tag:

    git tag -a v0.1.0-alpha -m "v0.1.0-alpha"

Push the tag:

    git push origin v0.1.0-alpha

## GitHub release

Create a GitHub release manually from the web UI, or with GitHub CLI:

    gh release create v0.1.0-alpha --title "v0.1.0-alpha" --notes-file docs/releases/v0.1.0-alpha.md

## Artifacts

GitHub Actions uploads development artifacts for each workflow run.

Expected artifacts:

- face-unlockd-ubuntu-24.04
- pam-face-unlock-ubuntu-24.04
- face-unlock-linux-deb-ubuntu-24.04

For release-quality packaging, inspect the .deb before publishing.

## Release notes

The v0.1.0-alpha release notes must clearly state:

- this is not real biometric authentication
- default auth fails closed
- development auth is only for testing
- no real PAM service files are modified automatically
- sudo/login/lock-screen integration is not production-ready

## Rollback

If a release tag is created by mistake and has not been used by others, delete locally:

    git tag -d v0.1.0-alpha

Delete remote tag:

    git push origin :refs/tags/v0.1.0-alpha

Use caution when deleting public release tags.
