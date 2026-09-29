# Project Status

face-unlock-linux is in v0.2 development. It is a working infrastructure and
CPU-model prototype, not production-ready biometric authentication.

## Current focus

The current phase is calibrating and benchmarking the native CPU enrollment
pipeline:

- YuNet detection is already implemented in C++
- SFace alignment and embedding are implemented in C++ and Python
- multi-pose profile construction and score-only matching exist in C++ and Python
- daemon enrollment sessions and encrypted profile commit are implemented
- the Qt GUI drives consent-gated enrollment and encrypted commit
- independent held-out samples gate storage for every pose
- real authentication matching remains disabled

The next bounded implementation slice is repeatable end-to-end benchmarking
and threshold calibration, followed by privacy-safe live-preview design.

## Implemented

### Runtime and IPC

- C++17 per-user daemon
- OpenCV camera probe, manual loop, and on-demand lease thread
- per-generation latest-frame memory store with stale-frame clearing
- UNIX socket under the user runtime directory
- socket mode 0600
- SO_PEERCRED peer checks
- same-user policy
- explicit development-only root auth peer policy
- bounded failed-attempt state
- deterministic lock-screen policy and on-demand camera lifecycle operations
- clean signal handling

### CPU face pipeline

- pinned, checksum-verified YuNet and SFace models
- OpenCV CPU target with no required CUDA runtime
- C++ YuNet detector loaded once at daemon startup
- bounding boxes, confidence, five landmarks, and detector latency
- C++ and Python SFace alignment and normalized embeddings
- guided center/left/right/up/down enrollment prototype
- duplicate sample rejection and pose coverage
- C++ and Python pose centroids and score-only matching tests
- native one-face, landmark, exposure, sharpness, size, and confidence gates
- fail-closed detection-to-profile diagnostic scoring pipeline
- strict versioned profile payload with bounded parsing
- libsodium encrypted profile round-trip test
- daemon-owned enrollment start, capture, status, cancel, and commit operations
- bounded enrollment camera lease with release on ready, cancel, failure, or commit
- atomic 0600 encrypted profile, development key, and manifest writes
- held-out pose validation with storage-layer enforcement and manifest evidence
- camera-free latency benchmark

### PAM and safety

- minimal C PAM socket client
- bounded timeout
- success only on explicit daemon auth success
- fail-closed missing socket, timeout, and error behavior
- heavy dependency audit
- fake PAM test flow
- guarded sudo planning, backup, apply, and rollback scripts

### Storage and GUI

- libsodium encryption scaffold
- development key and placeholder template tools
- decryptability metadata without plaintext output
- Qt6 consent-gated enrollment controls, automatic sampling, pose guidance, and quality feedback
- separately confirmed encrypted commit and complete local profile/key Forget Me flow
- placeholder Forget Me flow
- CPack Debian package skeleton and systemd user service assets

## Not implemented

- calibrated thresholded authentication decisions
- calibrated acceptance thresholds
- liveness or presentation-attack defense
- production key management
- production sudo authentication
- GNOME lock-screen rendering or actual unlock integration
- display-manager integration
- one-command end-user installation
- automatic, production-safe PAM configuration

## Authentication behavior

Normal authentication remains fail-closed. With a valid placeholder template,
the daemon reports matcher_not_implemented rather than approving authentication.

The following flags exist only for controlled development:

    FACE_UNLOCK_DEV_ALLOW=1
    FACE_UNLOCK_ALLOW_ROOT_AUTH=1

They are not production features.

## Performance baseline

On the current Intel i5-12500H development laptop with synthetic inputs:

- Python YuNet 320x320 p50 is about 3.8 ms
- Python SFace 112x112 p50 is about 9.9 ms
- C++ YuNet blank-frame smoke inference is about 6.6 ms
- C++ SFace 112x112 p50 is about 10.1 ms and p95 is about 12.6 ms
- Python benchmark peak RSS is about 277 MB
- two local cold-camera observations reached open in about 205-207 ms and the
  first 640x480 frame in about 842-970 ms

These values are implementation baselines, not authentication or accuracy
claims. Real-camera latency, thermal behavior, false accepts, false rejects, and
spoof resistance still require measurement.

## Supported development environment

Primary CI and development target:

- Ubuntu 24.04 LTS
- x86_64
- GCC or Clang with C++17
- OpenCV 4.6-compatible APIs
- Intel and AMD laptop CPUs
- no required discrete GPU

Other Linux distributions and architectures are not yet validated.

## Safe usage boundary

Safe today:

- building and testing locally
- running camera and detector diagnostics
- running the daemon enrollment flow with development key storage
- querying daemon metadata
- testing the PAM module with the fake PAM service
- building and inspecting packages

Not safe as a daily authentication replacement:

- removing password fallback
- enabling development auth as a real factor
- treating RGB face detection as liveness
- manually editing production PAM files
- enabling login or lock-screen integration

See the [roadmap](../ROADMAP.md) for the remaining implementation gates.
