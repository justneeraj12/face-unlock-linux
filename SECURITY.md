# Security Policy

face-unlock-linux is authentication-related software. Security reports are taken seriously.

## Supported versions

This project is currently pre-release.

No version is considered production-stable yet.

## Reporting vulnerabilities

Please do not open public GitHub issues for vulnerabilities.

For now, report security concerns by opening a GitHub issue with the title:

    Security concern: contact requested

Do not include exploit details in the issue.

Once the project has maintainers and releases, this file will be updated with a dedicated security contact.

## Security principles

The project follows these principles:

1. The PAM module must stay minimal.
2. Heavy dependencies must live outside PAM.
3. Authentication must fail closed.
4. Password/PIN fallback must remain available.
5. Face templates must be encrypted at rest.
6. Raw images must never be logged.
7. No telemetry by default.
8. System configuration changes must be reversible.
9. PAM modifications must require explicit consent.
10. Rollback instructions must be documented before install instructions.

## PAM module rules

The PAM module must not link to:

- LibTorch
- OpenCV
- CUDA
- TensorRT
- Qt
- Python

The PAM module should only:

- resolve the current authentication user
- connect to a UNIX domain socket
- send a small authentication request
- wait for a bounded timeout
- return success or failure to PAM

## Template storage rules

Face templates should be:

- encrypted at rest
- stored per-user
- removable by the user
- never uploaded
- never silently shared between accounts

Planned storage path:

    ~/.local/share/face-unlock/

## Logging rules

Logs must not contain:

- raw face images
- face crops
- biometric embeddings
- encryption keys
- detailed template material

Logs may contain:

- startup status
- camera availability
- IPC errors
- high-level authentication result
- non-sensitive timing information

## PAM safety

Before any installer modifies PAM files, it must:

1. show the exact file changes
2. create timestamped backups
3. provide one-command rollback
4. require explicit user confirmation
5. recommend keeping a root shell open

See docs/pam-safety.md.

## Dependency audit

Dependency policy is documented in:

    docs/dependency-audit.md

The PAM module must remain minimal and must not link OpenCV, Torch, Qt, CUDA, TensorRT, Python, or libsodium.
