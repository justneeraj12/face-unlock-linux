# Script Inventory

This document describes required project scripts.

## Checker

Run:

    ./scripts/check-scripts.sh

The checker verifies that required scripts exist and are executable.

## Purpose

This prevents CI from referencing scripts that were not committed or lost executable permissions.

## Script groups

Build/test:

- scripts/build.sh
- scripts/test.sh
- scripts/verify-local.sh
- scripts/check-docs.sh
- scripts/check-json.sh
- scripts/check-scripts.sh
- scripts/audit-dependencies.sh

Packaging:

- scripts/package-deb.sh

IPC:

- scripts/test-socket-client.sh

PAM fake test:

- scripts/install-fake-pam-test.sh
- scripts/remove-fake-pam-test.sh

systemd user service:

- scripts/install-user-service.sh
- scripts/remove-user-service.sh

sudo:

- scripts/plan-sudo-pam-install.sh
- scripts/apply-sudo-pam-install.sh
- scripts/rollback-sudo-pam.sh
- scripts/test-sudo-dry-run.sh

Validation:

- scripts/validate-enrollment-manifest.py
- scripts/validate-model-eval-metrics.py

Additional integration tests:

- scripts/test-key-template-flow.sh

Additional daemon integration tests:

- scripts/test-daemon-metadata.sh

Additional auth integration tests:

- scripts/test-auth-reasons.sh

Additional Python prototype tests:

- scripts/test-python-detectors.sh

Additional validators:

- scripts/validate-detector-output.py
