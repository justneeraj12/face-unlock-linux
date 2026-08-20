# Milestones

This document describes planned project milestones.

## v0.1.0-alpha

Status:

    released

Purpose:

Infrastructure prototype.

Included:

- daemon camera and IPC prototype
- minimal PAM IPC module
- fake PAM testing
- systemd user service helpers
- Debian package skeleton
- encrypted placeholder template scaffold
- Python prototypes
- optional Qt GUI scaffold
- CI and local verification

Not included:

- real biometric authentication
- production sudo integration
- lock-screen integration

## v0.2.0-dev-auth-sudo

Purpose:

Make the development-only sudo prototype safer and better documented.

Goals:

- improve guarded sudo apply/rollback
- validate root peer auth behavior
- improve sudo test docs
- add automated checks where safe
- keep dev auth clearly marked as non-production

Non-goal:

- real biometric auth

## v0.3.0-enrollment-cli

Purpose:

Create a CLI-based placeholder-to-real enrollment path.

Goals:

- enrollment manifest writer
- encrypted template writer
- local-only embedding prototype
- template delete/forget-me CLI
- manifest validation
- no raw images by default

## v0.4.0-real-model-prototype

Purpose:

Integrate first real detector/embedding prototype outside production auth.

Goals:

- select model candidates
- document licenses
- export TorchScript/ONNX
- run Python evaluation harness
- calibrate initial thresholds
- prototype matching outside PAM

## v0.5.0-qt-enrollment

Purpose:

Build the first usable Qt enrollment flow.

Goals:

- camera preview
- guided pose slots
- low-light warnings
- quality checks
- encrypted template creation
- forget-me flow
- consent screens

## v0.6.0-lock-screen-prototype

Purpose:

Prototype lock-screen integration in a logged-in session.

Goals:

- validate GNOME/KDE lock-screen behavior
- document Wayland/X11 caveats
- ensure password fallback
- keep rollback simple

## v1.0.0-security-review

Purpose:

Production readiness review.

Required before 1.0:

- real biometric matching implemented
- threshold calibration documented
- liveness/spoofing limitations documented
- PAM behavior audited
- installer rollback audited
- template encryption reviewed
- no raw image telemetry
- external security review preferred

## Current active development

Current active target after v0.1.1-alpha:

    v0.2.0-dev-auth-sudo

Primary focus:

- harden development-only sudo integration
- improve rollback/testing
- keep real biometric auth out of scope until model/enrollment work is ready
