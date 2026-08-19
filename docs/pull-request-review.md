# Pull Request Review

This document describes review expectations for pull requests.

## PR template

The repository uses:

    .github/pull_request_template.md

GitHub automatically inserts this template into new pull request descriptions.

## Security-sensitive areas

Extra care is required for changes involving:

- PAM
- sudo integration
- lock-screen or greeter integration
- UNIX socket IPC
- SO_PEERCRED policy
- camera access
- biometric data saving
- encrypted templates
- key management
- model loading
- model licenses
- threshold calibration
- installer scripts
- rollback behavior

## Required review focus

Reviewers should check:

- auth fails closed
- password fallback remains available
- no raw biometric data is committed
- no secrets are committed
- PAM module remains minimal
- heavy dependencies do not enter PAM
- installers print exact changes
- rollback is documented
- model licenses are documented
- privacy behavior is explicit

## PAM-specific review

For PAM changes, reviewers should verify:

- pam_face_unlock.so does not link OpenCV
- pam_face_unlock.so does not link Torch
- pam_face_unlock.so does not link Qt
- pam_face_unlock.so does not link CUDA
- pam_face_unlock.so does not link libsodium
- bounded timeouts remain in place
- failure returns PAM_AUTH_ERR
- success is only returned for explicit auth ok responses

## Model-specific review

For real model proposals, reviewers should require:

- source URL
- license
- redistribution rights
- preprocessing description
- input/output shape
- export method
- threshold calibration plan
- liveness/spoofing limitations

## Installer review

Any script that modifies system files must:

- show exact paths
- print diff or proposed change
- create backup
- print rollback command
- require explicit confirmation
- avoid modifying unrelated PAM services
