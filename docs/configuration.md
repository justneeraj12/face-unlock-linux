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
