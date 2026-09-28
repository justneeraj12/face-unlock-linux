# Daemon

daemon/ contains the C++17 per-user runtime and supporting command-line tools.

## Responsibilities

face-unlockd owns:

- on-demand camera leasing and the latest in-memory frame
- CPU detector model loading and inference
- local UNIX socket IPC
- peer credential checks
- authentication retry state
- encrypted template metadata
- CPU SFace alignment and normalized embeddings
- native five-pose profile construction and score-only matching
- native frame-quality gates and fail-closed diagnostic verification
- future daemon enrollment and thresholded authentication

It runs as the desktop user, not as root.

## Detector backends

- noop is always available
- Haar is an optional compatibility baseline
- YuNet is the CPU real-detector candidate

YuNet uses OpenCV DNN with the OpenCV CPU target. The ONNX model is validated
and loaded once at startup.

Build and run the camera-free model smoke test:

    ./scripts/build.sh
    ./scripts/download-cpu-models.sh
    ./build/daemon/face-unlock-detector-selftest --yunet-model models/face_detection_yunet_2022mar.onnx

Run camera plus socket mode:

    ./build/daemon/face-unlockd --camera 0 --detector yunet --detector-model models/face_detection_yunet_2022mar.onnx --daemon

Run the camera-free native SFace test and benchmark:

    ./scripts/test-cpu-recognizer.sh

## Socket

Default path:

    /run/user/$UID/face-unlock.sock

Properties:

- UNIX stream socket
- mode 0600
- SO_PEERCRED peer inspection
- same-user access by default
- root auth peers require explicit development opt-in

Operations:

- ping
- camera_status
- detector_status
- template_status
- lockscreen_policy
- lockscreen_start
- lockscreen_cancel
- lockscreen_password_started
- auth

Query from another terminal:

    ./scripts/test-socket-client.sh detector_status

detector_status returns backend, status, face count, latency, and detections.
YuNet detections include a box, confidence, and five landmarks.

lockscreen_policy returns the bounded attempt defaults and explicitly reports
that desktop integration is pending. It is a read-only capability query.

In daemon mode, the camera remains closed until `lockscreen_start`. The lease
clears stale frames, reports cold-open and first-frame latency, begins the
one-second recognition window at the first usable frame, and releases on its
deadline or either cancellation operation. These operations manage camera
lifetime only; they cannot approve authentication or unlock a session.

## Authentication state

Real matching is not implemented. Authentication therefore fails closed with
reasons such as template_missing, key_missing, template_decrypt_failed,
matcher_not_implemented, or too_many_attempts.

Development-only success requires:

    FACE_UNLOCK_DEV_ALLOW=1

Root-owned auth requests additionally require:

    FACE_UNLOCK_ALLOW_ROOT_AUTH=1

Neither flag is suitable for production.

## Tools

The build also creates:

- face-unlock-lockscreen-auth-selftest
- face-unlock-camera-lease-selftest
- face-unlock-frame-quality-selftest
- face-unlock-verification-pipeline-selftest
- face-unlock-native-verification-benchmark
- face-unlock-detector-selftest
- face-unlock-recognizer-selftest
- face-unlock-profile-selftest
- face-unlock-crypto-selftest
- face-unlock-key-tool
- face-unlock-template-tool

See [configuration](../docs/configuration.md),
[detector backends](../docs/daemon-detector-scaffold.md), and
[key management](../docs/key-management.md).

## Privacy

Camera frames stay in memory. The daemon must never log or persist raw frames,
face crops, embeddings, encryption keys, or plaintext templates.
