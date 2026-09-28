# Roadmap

face-unlock-linux is working toward a lightweight, CPU-first face recognition
add-on for Linux. Every milestone must preserve password fallback, local-only
processing, and fail-closed authentication.

This roadmap describes direction rather than a release promise.

## Completed foundation

- C++ per-user daemon and OpenCV camera worker
- mode-0600 UNIX socket with SO_PEERCRED policy
- minimal C PAM IPC client and dependency audit
- guarded fake PAM and sudo rollback tooling
- libsodium template encryption scaffold
- optional Qt6 enrollment GUI scaffold
- CPU YuNet detector in Python and C++
- CPU SFace embedding in Python and C++
- multi-pose profile builder prototype
- pinned model downloader with checksum verification
- synthetic CPU benchmark harness
- CI, CTest, Debian package skeleton, and release workflows

## Current phase: CPU recognition and enrollment

The immediate goal is a complete non-PAM enrollment and verification pipeline.

- [x] Run YuNet detection in the C++ daemon
- [x] Return bounding boxes, confidence, landmarks, and latency
- [x] Prototype SFace alignment and embeddings on CPU
- [x] Prototype guided multi-pose profile construction
- [x] Port SFace alignment and embedding to C++
- [ ] Define and validate a versioned encrypted face-profile payload
- [ ] Add daemon enrollment start, status, cancel, and commit operations
- [ ] Add quality gates for lighting, blur, face size, occlusion, and pose
- [ ] Add held-out enrollment validation before committing a profile
- [ ] Benchmark end-to-end latency, memory, and thermal behavior
- [ ] Evaluate false accept and false reject behavior on consented local data

Authentication remains fail-closed throughout this phase.

## Next phase: seamless GUI enrollment

- live camera preview
- guided head-turn instructions
- real pose and quality progress
- enrollment processing indicator
- clear retry and recovery messages
- encrypted profile commit only after validation
- complete Forget Me deletion and verification
- accessible laptop-sized interface

## Security hardening phase

- presentation-attack and liveness evaluation
- calibrated match thresholds with conservative defaults
- bounded retries and cooldown behavior
- corrupted-model and corrupted-template tests
- daemon lifecycle and crash recovery
- key storage design beyond raw development keys
- independent review of IPC, crypto, and PAM boundaries

Face recognition must remain an optional convenience factor with password or
PIN fallback.

## Packaging and daily-use phase

- reviewed redistribution rights for model weights
- versioned Debian packaging with runtime dependencies
- one-command package installation
- first-run GUI setup
- user service configuration
- explicit, reversible PAM opt-in
- tested uninstall and rollback
- Intel and AMD laptop compatibility matrix

No installer may silently edit PAM configuration.

## Desktop integration phase

Integrations are considered separately because their runtime and trust models
differ:

- sudo
- lock screen
- desktop login
- display manager or greeter
- encrypted-home and pre-login environments

Each integration requires its own threat review, fallback path, and rollback
test before it can be enabled.

## v1.0 readiness gates

A stable release requires:

- documented model licenses and provenance
- reproducible builds and packages
- measured accuracy and spoof-resistance limits
- safe enrollment and deletion
- encrypted, versioned profiles
- minimal audited PAM module
- reliable password fallback
- tested upgrade, uninstall, and rollback
- external security review
- clear disclosure that ordinary RGB cameras are not equivalent to dedicated
  depth or infrared face-authentication hardware
