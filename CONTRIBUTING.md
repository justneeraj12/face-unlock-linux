# Contributing

Thank you for your interest in contributing to face-unlock-linux.

This project deals with authentication and biometrics, so contributions must prioritize safety, clarity, and reviewability.

## Development philosophy

We prefer:

- small pull requests
- simple code
- explicit security tradeoffs
- documented design decisions
- tests for security-sensitive behavior
- fail-closed defaults

We avoid:

- large opaque changes
- hidden network behavior
- automatic PAM modifications
- storing raw biometric data
- unnecessary dependencies in privileged components

## Areas for contribution

Planned areas:

- C++ daemon
- PAM IPC module
- Qt6 enrollment GUI
- Python prototyping
- model export scripts
- encrypted template storage
- packaging
- documentation
- tests
- threat modeling

## Code style

General rules:

- C++17 for daemon code
- C11-compatible C for PAM module
- Python 3.12+ for prototypes
- CMake for native builds
- clear error handling
- minimal global state
- no raw image logging

## Security-sensitive changes

Security-sensitive changes include:

- PAM behavior
- socket permissions
- peer credential checks
- template encryption
- enrollment logic
- fallback behavior
- installer behavior
- greeter/login integration

These changes require extra review.

## Pull request checklist

Before submitting a PR:

- [ ] The change is small and focused.
- [ ] Security implications are documented.
- [ ] No raw biometric data is committed.
- [ ] PAM behavior is fail-closed.
- [ ] Rollback behavior is documented if system files are touched.
- [ ] New behavior is covered by tests or manual test instructions.

## Running checks

Early repo stage:

    git status
    ./scripts/check-docs.sh
    ./scripts/build.sh

Later stages will add:

    cmake --build build
    ctest --test-dir build

## License

By contributing, you agree that your contributions are licensed under the Apache License 2.0.

## Proposing real models

Use the Model candidate issue template for detector, alignment, embedding, or liveness model proposals.

Model proposals must document:

- source URL
- license
- redistribution rights
- preprocessing
- input/output shape
- export path
- threshold calibration notes
- spoofing/liveness limitations
- privacy/security concerns

Do not attach private biometric data to issues.

## Pull request security checklist

The pull request template includes security-sensitive sections for:

- PAM behavior
- biometric data handling
- model licensing
- template encryption
- installer behavior
- rollback instructions

Contributors must complete the relevant checklist items before requesting review.

GitHub automatically places the pull request template text into new pull request descriptions, so these prompts should appear when opening a PR.
