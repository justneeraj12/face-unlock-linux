# sudo Dry-Run Test

This document describes the sudo PAM dry-run regression test.

## Script

    scripts/test-sudo-dry-run.sh

## Purpose

The script verifies that sudo planning scripts do not modify /etc/pam.d/sudo in dry-run mode.

It runs:

    ./scripts/plan-sudo-pam-install.sh
    ./scripts/apply-sudo-pam-install.sh

without:

    --apply

## Safety

The script:

- computes a checksum of /etc/pam.d/sudo before testing
- runs both dry-run scripts
- computes a checksum after testing
- fails if the file changed

It does not modify PAM files.

## Run

    ./scripts/test-sudo-dry-run.sh

Expected final output:

    sudo_pam_unchanged: true
    status: ok

## Limitations

This test is local because it depends on /etc/pam.d/sudo.

It is not currently run in GitHub Actions.
