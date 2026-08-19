# Project Log

This file tracks major development steps for face-unlock-linux.

## 2026-08-18

### Completed

- Created public GitHub repository.
- Added documentation-first open-source skeleton.
- Added README, SECURITY, CONTRIBUTING, ROADMAP, and docs.
- Added placeholder directories for daemon, PAM, GUI, Python prototypes, models, packaging, scripts, and tests.

### Current phase

Phase 1A: documented repository skeleton.

### Next step

Phase 1B: daemon-only prototype.

Goals:

- add top-level CMake project
- add C++ daemon directory
- open webcam with OpenCV
- create UNIX socket under `/run/user/$UID/face-unlock.sock`
- respond to local test client with JSON
- no PAM integration yet
- no system authentication changes
