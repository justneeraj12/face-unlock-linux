# CODEOWNERS

This project uses GitHub CODEOWNERS.

File:

    .github/CODEOWNERS

## Purpose

CODEOWNERS helps route pull requests to maintainers for review.

This is especially important for security-sensitive areas such as:

- PAM
- sudo integration
- daemon IPC
- SO_PEERCRED policy
- template encryption
- installer scripts
- rollback behavior
- model proposals
- biometric data handling
- GitHub Actions workflows

## Current owner

Current primary owner:

    @justneeraj12

More maintainers can be added later.

## Review expectations

CODEOWNERS is not a substitute for security review.

For security-sensitive changes, reviewers should also consult:

    docs/pull-request-review.md
    SECURITY.md
    docs/threat-model.md
    docs/pam-safety.md
