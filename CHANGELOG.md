# Changelog

All notable changes to face-unlock-linux will be documented in this file.

This project follows a lightweight changelog format inspired by Keep a Changelog.

## Unreleased

### Added

- Guarded sudo PAM apply and rollback scripts.
- sudo root-peer policy opt-in with FACE_UNLOCK_ALLOW_ROOT_AUTH.
- sudo dry-run regression test.
- Optional sudo dry-run mode in local verification.
- sudo troubleshooting documentation.
- Dependency audit documentation and script.
- Required script inventory checker.
- Release artifact workflow.
- GitHub labels bootstrap script.
- GitHub milestones bootstrap script.
- GitHub starter issues bootstrap script.
- CODEOWNERS review routing.
- Expanded pull request security checklist.
- Model candidate issue template.
- GUI scaffolds for:
  - Forget Me
  - brightness assist
  - pose slots
  - quality checklist
  - camera preview placeholder
- Enrollment manifest format scaffold.
- Enrollment manifest JSON validator.
- Model evaluation planning docs.
- Candidate model shortlist.
- Python model evaluation harness scaffold.
- Model evaluation metrics format.
- Model evaluation metrics validator.
- Placeholder enrollment manifest writer.
- Daemon enrollment manifest status reporting.
- Development key management scaffold.
- Development-key encrypted placeholder template support.
- Key/template integration self-test.
- Daemon key/decryptability metadata reporting.
- Explicit daemon template_status operation.
- Daemon metadata integration test.
- Key-aware auth failure reasons.
- Auth reason integration test.

### Changed

- README refactored into a cleaner project front page.
- CI now uses shared dependency audit and script inventory checks.
- CI validates enrollment manifest and model evaluation metrics examples.
- Local verification now includes script inventory, JSON, manifest, metrics, tests, dependency audit, and package inspection.
- Root-owned sudo auth peers are disabled by default and must be explicitly enabled.
- Placeholder template status now reports decryptability metadata.
- Daemon auth failures now distinguish:
  - template_missing
  - template_not_decryptable
  - key_missing
  - template_decrypt_failed
  - matcher_not_implemented

### Security

- PAM module dependency audit remains enforced.
- sudo integration remains guarded and rollback-focused.
- Development auth remains explicitly opt-in and non-production.
- Root sudo peer auth is explicitly opt-in.
- Template/key tooling avoids printing plaintext or key material.
- Integration tests use temporary HOME and XDG_RUNTIME_DIR where possible.

## v0.1.0-alpha

### Added

- Initial infrastructure prototype.
- C++ user daemon with OpenCV camera probe.
- Camera loop mode and camera worker thread.
- UNIX domain socket server.
- Socket permissions set to 0600.
- SO_PEERCRED peer credential logging.
- same-UID socket policy.
- fail-closed auth operation.
- max auth attempt enforcement.
- development-only auth gate.
- minimal C PAM IPC module.
- fake PAM service testing.
- fake PAM install/remove scripts.
- systemd user service install/remove scripts.
- sudo PAM inspection and dry-run planning.
- libsodium encrypted template storage scaffold.
- crypto self-test.
- encrypted placeholder template CLI.
- placeholder enrollment manifest scaffold.
- Python safe capture prototype.
- Python embedding prototype.
- TorchScript export stub.
- optional LibTorch loader scaffold.
- optional Qt enrollment GUI scaffold.
- CTest support.
- GitHub Actions build workflow.
- CPack Debian package skeleton.
- local verification script.
- release preparation script.

### Security

- No real biometric authentication.
- Default auth fails closed.
- No real PAM service files are modified automatically.
- Password fallback remains required for safe testing.
- Raw biometric data is ignored by Git and not saved by default.
