# Project Status

face-unlock-linux is in v0.2 development. It is a working infrastructure and
CPU-model prototype, not production-ready biometric authentication.

## Current focus

The current phase is connecting the proven CPU pipeline to the C++ daemon:

- YuNet detection is already implemented in C++
- SFace alignment and embedding are implemented in C++ and Python
- multi-pose profile construction and score-only matching exist in C++ and Python
- daemon enrollment operations are not implemented
- real authentication matching remains disabled

The next bounded implementation slice is daemon-owned enrollment sessions and
atomic encrypted profile persistence.

## Implemented

### Runtime and IPC

- C++17 per-user daemon
- OpenCV camera probe, loop, and worker thread
- latest-frame memory store
- UNIX socket under the user runtime directory
- socket mode 0600
- SO_PEERCRED peer checks
- same-user policy
- explicit development-only root auth peer policy
- bounded failed-attempt state
- deterministic lock-screen attempt policy and read-only capability metadata
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
- strict versioned profile payload with bounded parsing
- libsodium encrypted profile round-trip test
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
- Qt6 consent, privacy, daemon status, pose, and quality scaffold
- placeholder Forget Me flow
- CPack Debian package skeleton and systemd user service assets

## Not implemented

- daemon-owned encrypted biometric profile creation
- thresholded authentication decisions
- calibrated acceptance thresholds
- held-out enrollment validation
- liveness or presentation-attack defense
- production key management
- production sudo authentication
- GNOME lock-screen rendering, camera leasing, or unlock integration
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
- running the non-persistent enrollment prototype
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
