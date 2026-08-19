# sudo Test Results

This document records manual sudo integration test results for face-unlock-linux.

## Test status

sudo integration has been tested manually with the guarded apply and rollback scripts.

## Safety model tested

The sudo integration test used:

    auth sufficient pam_face_unlock.so timeout_ms=1000

This line was inserted into:

    /etc/pam.d/sudo

using:

    ./scripts/apply-sudo-pam-install.sh --apply

The script created a timestamped backup before applying changes.

Rollback was tested with:

    ./scripts/rollback-sudo-pam.sh --latest

## Preconditions used

Before applying sudo integration:

- daemon build completed
- PAM module built
- PAM module dependency audit passed
- fake PAM service test passed
- user daemon service was available
- root-authenticated recovery shell was kept open
- dry-run diff was reviewed

## Default fail-closed test

Daemon mode:

    FACE_UNLOCK_DEV_ALLOW=0

Test command:

    sudo -k
    sudo true

Expected behavior:

- pam_face_unlock.so sends auth request to daemon
- daemon returns status fail
- sudo falls back to password
- correct password succeeds

Result:

    PASS

## Development-only auth test

Daemon mode:

    FACE_UNLOCK_DEV_ALLOW=1

Test command:

    sudo -k
    sudo true

Expected behavior:

- sudo PAM client connects to daemon
- daemon logs root peer with SO_PEERCRED
- daemon allows root peer for auth operation only
- daemon returns development-only auth success if camera is ready
- sudo succeeds without password

Result:

    PASS

## Root peer policy test

Root auth request:

    PASS

Root non-auth request:

    PASS

Expected policy:

- same UID clients are allowed
- root UID 0 is allowed only for auth
- root UID 0 is rejected for non-auth operations
- other UIDs are rejected

## Rollback test

Rollback command:

    ./scripts/rollback-sudo-pam.sh --latest

Expected behavior:

- latest sudo PAM backup is restored
- sudo returns to normal password behavior

Result:

    PASS

## Important limitations

This is still not real biometric authentication.

The successful passwordless sudo test used:

    FACE_UNLOCK_DEV_ALLOW=1

That is development-only and must never be used as real authentication.

Real sudo face unlock still requires:

- real face detection
- real embedding model
- encrypted template enrollment
- encrypted template matching
- threshold calibration
- liveness/spoof-resistance considerations
- more audit and testing

## Files touched during test

Temporarily modified:

    /etc/pam.d/sudo

Backup created:

    /etc/pam.d/sudo.face-unlock-backup.TIMESTAMP

Restored by rollback:

    yes

## Final state after test

sudo PAM was rolled back to the previous configuration.

The system should require normal password sudo unless sudo integration is applied again.
