# Documentation Index

This directory contains project documentation for face-unlock-linux.

## Start here

| Document | Purpose |
|---|---|
| project-status.md | Current implementation status |
| architecture.md | Detailed technical architecture |
| development-setup.md | Ubuntu development setup |
| local-verification.md | One-command local verification |
| roadmap | See ../ROADMAP.md |

## Safety and security

| Document | Purpose |
|---|---|
| ../SECURITY.md | Security policy |
| threat-model.md | Threat model and mitigations |
| pam-safety.md | PAM safety rules |
| sudo-root-peer-policy.md | Root peer policy for sudo PAM clients |

## PAM and sudo

| Document | Purpose |
|---|---|
| pam-fake-service-test.md | Safe fake PAM test flow |
| sudo-integration-plan.md | sudo integration plan |
| sudo-safe-installer.md | sudo dry-run installer plan |
| sudo-apply-and-rollback.md | Guarded sudo apply and rollback |
| sudo-test-results.md | Manual sudo test results |

## Daemon

| Document | Purpose |
|---|---|
| architecture.md | Daemon architecture and IPC |
| configuration.md | User config file |
| systemd-user-service.md | systemd user service |
| template-storage.md | Encrypted template storage scaffold |
| template-cli.md | Placeholder template CLI |

## Enrollment and templates

| Document | Purpose |
|---|---|
| enrollment-format.md | Enrollment manifest format |
| manifest-validation.md | Manifest validation |
| template-storage.md | Template crypto scaffold |
| template-cli.md | Template CLI scaffold |

## GUI

| Document | Purpose |
|---|---|
| gui.md | Qt GUI scaffold |
| gui-camera-preview.md | Camera preview design |
| brightness-assist.md | Brightness assist design |

## Python and models

| Document | Purpose |
|---|---|
| python-prototypes.md | Python capture prototype |
| python-embedding-prototype.md | Python embedding prototype |
| model-export.md | TorchScript export stub |
| libtorch-loader.md | Optional LibTorch daemon loader |

## Testing and CI

| Document | Purpose |
|---|---|
| testing.md | CTest and manual tests |
| ci.md | GitHub Actions workflows |
| local-verification.md | Local verification script |

## Packaging and releases

| Document | Purpose |
|---|---|
| packaging.md | Debian package skeleton |
| release-process.md | Release process |
| releases/v0.1.0-alpha.md | v0.1.0-alpha checklist and notes |

## Important warnings

This project is not real biometric authentication yet.

Do not use it as your only authentication method.

Do not manually edit real PAM service files unless you understand the rollback process.

Development auth requires:

    FACE_UNLOCK_DEV_ALLOW=1

and must never be used as production authentication.

## Model evaluation and matching

| Document | Purpose |
|---|---|
| model-evaluation-plan.md | Plan for selecting and evaluating real models |
| threshold-calibration.md | Matching threshold calibration plan |

## Candidate models

| Document | Purpose |
|---|---|
| model-candidates.md | Candidate detector, alignment, and embedding models |

## Model evaluation harness

| Document | Purpose |
|---|---|
| model-evaluation-harness.md | Python scaffold for model evaluation metrics |

## Evaluation metrics

| Document | Purpose |
|---|---|
| model-evaluation-metrics.md | JSON format for model evaluation metrics |

## Metrics validation

| Document | Purpose |
|---|---|
| model-evaluation-metrics.md | Metrics format and validator details |

## Contribution review

| Document | Purpose |
|---|---|
| pull-request-review.md | Review expectations for security-sensitive PRs |

## Ownership and review routing

| Document | Purpose |
|---|---|
| codeowners.md | CODEOWNERS and review routing |

## GitHub project management

| Document | Purpose |
|---|---|
| github-labels.md | GitHub issue label setup |

## Milestones

| Document | Purpose |
|---|---|
| milestones.md | Planned project milestone definitions |

## GitHub issues

| Document | Purpose |
|---|---|
| github-issues.md | Starter issue bootstrap script |
