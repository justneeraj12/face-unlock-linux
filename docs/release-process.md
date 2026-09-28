# Release Process

Releases are manual, reviewable, and must not modify PAM configuration.

## Choose a release tag

Use a semantic tag such as:

    v0.2.0-alpha

Create matching notes under docs/releases/ and move completed entries from the
Unreleased section of CHANGELOG.md into the release section.

## Prepare

Run the full local verification:

    ./scripts/verify-local.sh

Then run the release preparation helper:

    ./scripts/prepare-release.sh v0.2.0-alpha

The helper checks the working tree, builds, tests, audits PAM dependencies, and
builds the development package. It does not create or push a tag.

Before continuing, inspect:

    git status
    git diff
    dpkg-deb -I build/*.deb
    dpkg-deb -c build/*.deb

## Tag and publish

After local verification and GitHub Actions pass:

    git tag -a v0.2.0-alpha -m "v0.2.0-alpha"
    git push origin v0.2.0-alpha

The tag-triggered release workflow builds the package and uploads it to the
GitHub release. A release can also be created with:

    gh release create v0.2.0-alpha --title "v0.2.0-alpha" --notes-file docs/releases/v0.2.0-alpha.md

Use the actual reviewed tag and notes filename for each release.

## Required release notes

Every pre-1.0 release must state:

- whether real biometric matching is enabled
- that password or PIN fallback is required
- known accuracy and liveness limitations
- model sources, licenses, and redistribution status
- whether any PAM integration is supported
- upgrade, uninstall, and rollback instructions

## PAM safety

Release scripts and packages must not silently modify:

    /etc/pam.d/sudo
    /etc/pam.d/common-auth
    /etc/pam.d/gdm-password
    /etc/pam.d/sddm
    /etc/pam.d/lightdm

Any future PAM opt-in must show the exact diff, back up the target, require
explicit confirmation, and print a tested rollback command.

## Correcting a tag

Avoid rewriting public tags. If an unpublished tag was created by mistake:

    git tag -d v0.2.0-alpha
    git push origin :refs/tags/v0.2.0-alpha

Only do this when no user or release depends on that tag.
