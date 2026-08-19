# Code of Conduct

This project follows the Contributor Covenant Code of Conduct.

## Our pledge

We pledge to make participation in this project a harassment-free experience for everyone.

We welcome contributors regardless of background, identity, experience level, or viewpoint.

## Expected behavior

Examples of positive behavior:

- being respectful
- giving constructive feedback
- assuming good intent
- focusing on technical merit
- documenting concerns clearly
- prioritizing user safety

## Unacceptable behavior

Examples of unacceptable behavior:

- harassment
- insults or personal attacks
- publishing private information
- discriminatory language
- deliberately unsafe advice
- encouraging users to bypass security protections

## Scope

This code applies in:

- GitHub issues
- pull requests
- discussions
- documentation
- project communication channels

## Enforcement

Maintainers may remove, edit, or reject comments and contributions that violate this code.

Serious or repeated violations may result in a ban from project spaces.

## v0.1.0-alpha target

The v0.1.0-alpha milestone is an infrastructure prototype.

Target contents:

- daemon camera and IPC prototype
- minimal PAM IPC module
- fake PAM service testing
- systemd user service helpers
- encrypted template storage scaffold
- Python capture prototype
- TorchScript export/load scaffold
- Debian package skeleton
- CI artifacts

Non-goals for v0.1.0-alpha:

- real biometric authentication
- production sudo integration
- lock-screen integration
- greeter/login integration
- Qt enrollment GUI

## Phase 6: Real model evaluation

Goals:

- select candidate detector models
- select candidate embedding models
- document model licenses
- prototype Python evaluation
- export TorchScript/ONNX artifacts
- calibrate matching thresholds
- document liveness limitations

Deliverables:

- [ ] model evaluation plan
- [ ] threshold calibration document
- [ ] Python model evaluation script
- [ ] sample local-only evaluation workflow
- [ ] documented candidate model shortlist

## Milestone plan

Detailed milestone definitions are maintained in:

    docs/milestones.md

Planned milestones:

- v0.2.0-dev-auth-sudo
- v0.3.0-enrollment-cli
- v0.4.0-real-model-prototype
- v0.5.0-qt-enrollment
- v0.6.0-lock-screen-prototype
- v1.0.0-security-review
