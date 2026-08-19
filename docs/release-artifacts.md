# Release Artifacts

This document describes automated release artifact publishing.

## Workflow

Release workflow:

    .github/workflows/release.yml

## Trigger

The workflow runs on version tags:

    v*

Examples:

    v0.1.1-alpha
    v0.2.0
    v1.0.0

## What it builds

The workflow builds the Debian package with CPack and collects:

    artifacts/*.deb

## GitHub Release upload

The workflow uses softprops/action-gh-release to create or update the GitHub Release and upload Debian package assets.

Prerelease detection:

- tags containing alpha are prereleases
- tags containing beta are prereleases
- tags containing rc are prereleases

## Permissions

The workflow uses:

    permissions:
      contents: write

This is required for creating/updating GitHub Releases and uploading release assets.

## Safety

The release workflow does not modify PAM files.

It only builds artifacts in GitHub Actions and uploads them to the GitHub Release.

The Debian package itself must still be inspected before installation.

## Manual release flow

Prepare locally:

    ./scripts/prepare-release.sh v0.1.1-alpha

Tag:

    git tag -a v0.1.1-alpha -m "v0.1.1-alpha"

Push tag:

    git push origin v0.1.1-alpha

Then watch:

    gh run list --limit 10
