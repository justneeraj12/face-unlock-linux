# Configuration

face-unlock-linux supports an optional per-user config file.

## Config path

Default path:

    ~/.config/face-unlock/config.json

## Current supported fields

Example:

    {
      "camera_index": 0,
      "detector_backend": "noop",
      "detector_model_path": "",
      "recognizer_model_path": "",
      "max_auth_attempts": 3
    }

## camera_index

The camera index used by the daemon when no --camera argument is provided.

Default:

    0

Command-line arguments override config.

Example:

    ./build/daemon/face-unlockd --camera 1 --daemon

## max_auth_attempts

The maximum authentication attempts setting.

Current status:

- parsed by daemon
- printed at startup
- enforced by the in-memory auth retry state

Default:

    3

Allowed range:

    1 through 10

The counter resets when the daemon restarts.

## Write default config

Use:

    ./scripts/write-default-config.sh

The script asks for confirmation before writing:

    ~/.config/face-unlock/config.json

The config file is written with mode:

    0600

## Safety

The config file does not currently enable authentication success.

Default auth remains fail-closed.

Development auth is still controlled separately by:

    FACE_UNLOCK_DEV_ALLOW=1

Development auth must never be used as real authentication.

## Auth attempt enforcement

max_auth_attempts is now enforced by the daemon auth operation.

Default:

    3

Behavior:

- failed auth requests increment an in-memory counter
- when the counter reaches max_auth_attempts, later auth requests return too_many_attempts
- successful development-only auth resets the counter
- restarting the daemon resets the counter

This is a scaffold for future retry and fallback behavior.

## detector_backend

The daemon supports a detector_backend config field.

Supported values depend on the OpenCV components available at build time:

- noop is always available and remains the default
- haar is an optional baseline
- yunet is the CPU real-detector candidate

Example:

    {
      "camera_index": 0,
      "detector_backend": "noop",
      "max_auth_attempts": 3
    }

Unsupported values cause daemon startup to fail safely.

## Haar detector backend

If built with OpenCV objdetect support, the daemon may support:

    "detector_backend": "haar"

Haar is a baseline detector only.

Unsupported detector backends fail safely at startup.

## detector_model_path

YuNet requires an explicit readable ONNX model path.

CLI example:

    ./build/daemon/face-unlockd --detector yunet --detector-model models/face_detection_yunet.onnx --serve

Config example:

    {
      "camera_index": 0,
      "detector_backend": "yunet",
      "detector_model_path": "/absolute/path/to/face_detection_yunet.onnx",
      "max_auth_attempts": 3
    }

The daemon fails closed at startup when YuNet is selected without a model path
or when the model cannot be read or loaded. Command-line values override config.


## recognizer_model_path

Native enrollment requires an explicit readable SFace ONNX model path.

CLI example:

    ./build/daemon/face-unlockd --detector yunet --detector-model models/face_detection_yunet.onnx --recognizer-model models/face_recognition_sface_2021dec.onnx --daemon

Config example:

    {
      "camera_index": 0,
      "detector_backend": "yunet",
      "detector_model_path": "/absolute/path/to/face_detection_yunet.onnx",
      "recognizer_model_path": "/absolute/path/to/face_recognition_sface_2021dec.onnx",
      "max_auth_attempts": 3
    }

The recognizer uses the OpenCV DNN CPU target. If a configured model cannot be
read or loaded, daemon startup fails closed. Without a recognizer model, daemon
status operations remain available but `enrollment_start` returns
`recognizer_unavailable`.
