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
- scripts/pam-module-path.sh

systemd user service:

- scripts/install-user-service.sh
- scripts/remove-user-service.sh

sudo:

- scripts/plan-sudo-pam-install.sh
- scripts/apply-sudo-pam-install.sh
- scripts/rollback-sudo-pam.sh
- scripts/test-sudo-dry-run.sh

Hyprlock:

- scripts/plan-hyprlock-pam-install.sh
- scripts/apply-hyprlock-pam-install.sh
- scripts/rollback-hyprlock-pam.sh
- scripts/test-hyprlock-dry-run.sh

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

Additional detector tests:

- scripts/test-detector-output-generation.sh

GUI build helper:

- scripts/build-gui.sh

Additional detector backend tests:

- scripts/test-detector-backends.sh

CPU face-profile tools:

- scripts/download-cpu-models.sh
- scripts/test-model-selection.sh
- scripts/test-cpu-face-profile.sh
- scripts/benchmark-native-verification.sh

The model downloader pins upstream revisions and verifies SHA-256 checksums.
Downloaded ONNX files are local build inputs and remain ignored by Git.
