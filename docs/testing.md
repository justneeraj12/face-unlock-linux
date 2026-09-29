# Testing

This document describes the current test setup.

## Current automated tests

The project uses CTest.

Current CTest cases:

- crypto_selftest
- detector_selftest
- lockscreen_auth_policy
- camera_lease
- camera_lease_protocol
- enrollment_protocol
- frame_quality
- verification_pipeline
- key_template_flow
- daemon_metadata
- auth_reasons
- detector_backends
- cpu_recognizer
- face_profile_cpp
- enrollment_session
- profile_storage
- enrollment_controller
- cpu_face_profile

The crypto self-test verifies:

- libsodium initializes
- placeholder template bytes encrypt successfully
- encrypted blob writes to a temporary file
- encrypted blob reads back
- decrypted bytes match plaintext
- temporary file is removed

## Run tests

Build first:

    ./scripts/build.sh

Run tests:

    ./scripts/test.sh

Equivalent command:

    ctest --test-dir build --output-on-failure

## Camera tests

Camera tests are currently manual because they require real hardware.

Run one-shot camera test:

    ./build/daemon/face-unlockd --camera 0

Measure on-demand camera startup and release without saving frames:

    ./scripts/benchmark-camera-lease.sh --camera 0

For manual protocol inspection, run daemon mode:

    ./build/daemon/face-unlockd --camera 0 --daemon

Then in another terminal:

    ./scripts/test-socket-client.sh camera_status
    ./scripts/test-socket-client.sh lockscreen_start
    ./scripts/test-socket-client.sh camera_status
    ./scripts/test-socket-client.sh lockscreen_password_started

## PAM tests

PAM tests are manual for now.

Use the fake PAM service flow only:

    docs/pam-fake-service-test.md

Do not modify sudo, login, lock-screen, GDM, SDDM, LightDM, or common-auth PAM files during automated testing.

## CI

GitHub Actions runs build and dependency checks.

The CI workflow should also run CTest:

    ctest --test-dir build --output-on-failure

## Key/template flow test

CTest includes:

    key_template_flow

This test runs:

    ./scripts/test-key-template-flow.sh

It verifies the development key plus placeholder template flow using a temporary HOME directory.

## Daemon metadata integration test

CTest includes:

    daemon_metadata

This test runs:

    ./scripts/test-daemon-metadata.sh

It uses temporary HOME and XDG_RUNTIME_DIR directories.

It verifies daemon socket responses include:

- template present
- enrollment placeholder
- key present
- decryptability possible_with_dev_key
- key_storage local_development_key_file

The test uses daemon --serve mode and does not require a camera.

## Lock-screen policy test

CTest includes:

    lockscreen_auth_policy

The native state-machine test verifies the three-candidate bound, one-second
deadline, low-light illumination handshake and settle interval, rejection of
low-quality frames without consuming candidates, immediate password
cancellation, monotonic timing, and invalid-policy rejection.

The daemon_metadata test also verifies the read-only `lockscreen_policy`
response. Neither test accesses a camera or modifies GNOME, GDM, PAM, screen
brightness, or system files.

## Camera lease tests

CTest includes `camera_lease` and `camera_lease_protocol`. The native test uses
an injected fake camera to verify idempotent start, automatic deadlines,
password cancellation, stale-frame deletion, failed opens, and late-frame
rejection. The protocol test uses an impossible camera index to verify that
daemon mode starts idle and camera-open failure remains fail-closed without
stopping the daemon.

Real hardware remains manual because startup and exposure behavior vary by
camera and driver. `benchmark-camera-lease.sh` reports open and first-frame
latency, cancels immediately after the first frame, and saves no frame data.

## Native enrollment tests

`enrollment_session` covers training-to-validation transitions, per-pose
held-out scoring, inconsistent-sample rejection, no held-out leakage into the
stored profile, ready/commit transitions, and cancellation erasure.
`profile_storage` refuses unvalidated profiles before creating files and covers
0600 writes, encrypted reload, key reuse, schema-aligned manifest evidence, and
ciphertext tamper rejection.

`enrollment_controller` uses injected camera, detector, and embedder fakes to
exercise the complete in-memory quality-to-pose pipeline, fresh-frame
enforcement, held-out validation, camera release, encrypted commit,
cancellation, and camera-failure erasure. The
`enrollment_protocol` socket test verifies unsupported enrollment fails closed
and creates no template, key, or manifest.

## Native verification tests

`frame_quality` covers single-face enforcement, landmarks, box size, exposure,
sharpness, confidence policy validation, and fail-closed reasons.
`verification_pipeline` covers score production, rejection before embedding,
pipeline errors, and the invariant that diagnostics never permit
authentication.

The optional live benchmark is:

    ./scripts/benchmark-native-verification.sh --camera 0 --iterations 20

It saves nothing and reports unavailable when the captured frame does not pass
YuNet and the quality gates.

## template_status operation test

The daemon_metadata CTest verifies the template_status socket operation.

It checks:

- template present
- enrollment placeholder
- key present
- decryptability possible_with_dev_key
- key_storage local_development_key_file
- template_decrypt ok

No plaintext is returned.

## Auth reason integration test

CTest includes:

    auth_reasons

This test runs:

    ./scripts/test-auth-reasons.sh

It verifies fail-closed auth reasons:

- template_missing
- template_not_decryptable
- key_missing
- matcher_not_implemented

The test uses daemon --serve mode and temporary HOME/XDG_RUNTIME_DIR directories.

## Python detector smoke test

Run:

    ./scripts/test-python-detectors.sh

This test does not require a camera.

It verifies detector imports, noop backend behavior, and prototype_detect.py CLI help.

## Detector output generation test

Run:

    ./scripts/test-detector-output-generation.sh

This test does not require camera hardware.

It uses prototype_detect.py with:

    --backend noop
    --synthetic-frame 640x480

## Detector self-test

CTest includes:

    detector_selftest

It verifies the C++ NoopFaceDetector returns zero detections and lists all
detector backends compiled into the current build.

With the pinned local model, run a real camera-free YuNet CPU inference test:

    ./build/daemon/face-unlock-detector-selftest --yunet-model models/face_detection_yunet_2022mar.onnx

## detector_status integration

The daemon_metadata test verifies:

    detector_status

Expected current metadata:

    detector noop
    faces_detected 0

## Detector backend integration test

CTest includes:

    detector_backends

This test runs:

    ./scripts/test-detector-backends.sh

It verifies:

- noop backend is supported
- detector_status works with noop and reports zero faces, latency, and detections
- YuNet without a model path fails safely when YuNet is compiled
- a local pinned YuNet model loads and runs CPU inference when present
- unsupported detector backends fail safely

## Conditional Haar detector test

The detector_backends CTest conditionally tests Haar.

If Haar is supported, it verifies:

    face-unlockd --detector haar --serve

responds to detector_status with:

    detector haar

If Haar is not compiled in, the test reports `skipped_not_supported`. If Haar is
compiled in but its cascade XML is unavailable at runtime, the test reports
`skipped_cascade_missing`. Both conditions are safe skips; other Haar startup
failures still fail the test.

## Detector latency test

Detector integration tests verify detector_status includes:

    detector_ms

This ensures detector backends report latency metadata. Responses also include
a detections array; noop and camera-free server mode return an empty array.

## CPU face-profile test

CTest includes:

    cpu_face_profile

This test runs:

    ./scripts/test-cpu-face-profile.sh

It verifies:

- coarse pose classification
- duplicate sample rejection
- pose coverage progress
- profile centroid construction
- synthetic embedding matching

## C++ CPU recognizer test

CTest includes:

    cpu_recognizer

This test runs:

    ./scripts/test-cpu-recognizer.sh

It always verifies that the native SFace backend is compiled. When the pinned
local SFace model is present, it also verifies CPU model loading, 128-value
L2-normalized embeddings, landmark alignment, cosine self-similarity, and
reports camera-free p50 and p95 inference latency. A missing ignored model is a
safe skip so CI remains network-independent.

## C++ face-profile test

CTest includes:

    face_profile_cpp

The native test covers all five pose classifications, sample quality and model
rejection, duplicate rejection, progress, normalized centroids, score-only
matching, binary serialization, libsodium encryption, and rejection of every
truncated payload prefix. It does not choose an authentication threshold.
