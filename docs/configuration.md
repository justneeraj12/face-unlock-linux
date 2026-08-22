# Configuration

face-unlock-linux supports an optional per-user config file.

## Config path

Default path:

    ~/.config/face-unlock/config.json

## Current supported fields

Example:

    {
      "camera_index": 0,
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
- not enforced yet

Default:

    3

Allowed range:

    1 through 10

Enforcement will be added later when real auth state tracking is implemented.

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

Current supported value:

    noop

Example:

    {
      "camera_index": 0,
      "detector_backend": "noop",
      "max_auth_attempts": 3
    }

Future values may include:

- haar
- yunet
- torchscript

Unsupported values cause daemon startup to fail safely.

## Haar detector backend

If built with OpenCV objdetect support, the daemon may support:

    "detector_backend": "haar"

Haar is a baseline detector only.

Unsupported detector backends fail safely at startup.
