# Daemon

This directory contains the user-level face unlock daemon.

## Current status

- minimal C++17 executable
- links OpenCV
- prints startup/runtime information
- opens webcam
- reads one frame by default
- can run continuously with --loop
- can run a local UNIX socket server with --serve
- can run camera worker plus socket server with --daemon
- socket path is /run/user/$UID/face-unlock.sock
- socket permissions are set to 0600
- logs UNIX socket peer credentials with SO_PEERCRED
- currently allows same-UID socket clients only
- supports simple socket operations: ping, camera_status, auth
- auth operation fails closed by default
- repeated failed auth attempts are limited by max_auth_attempts
- development-only auth can be enabled with FACE_UNLOCK_DEV_ALLOW=1
- stops cleanly with Ctrl+C
- does not save images

## Planned responsibilities

The daemon is planned to be responsible for:

- opening the camera
- maintaining a recent frame buffer
- loading the face recognition model
- reading encrypted templates
- performing face matching
- exposing a local UNIX domain socket
- enforcing authentication rate limits

The daemon should run as the normal desktop user, not as root.

## Fail-closed behavior

The daemon must fail closed if:

- the camera is unavailable
- no template exists
- the model is unavailable
- the IPC peer is not trusted
- authentication times out
- auth is not implemented
- development auth is not explicitly enabled
- max_auth_attempts is exceeded

## Build

From the repository root, build with:

    ./scripts/build.sh

Run daemon mode with camera worker and socket server:

    ./build/daemon/face-unlockd --camera 0 --daemon

Test auth fail-closed behavior in another terminal:

    ./scripts/test-socket-client.sh auth

Expected default auth response:

    status fail
    reason auth_not_implemented

After max_auth_attempts failures, expected response:

    status fail
    reason too_many_attempts

Run daemon mode with development-only auth enabled:

    FACE_UNLOCK_DEV_ALLOW=1 ./build/daemon/face-unlockd --camera 0 --daemon

Then test auth:

    ./scripts/test-socket-client.sh auth

Expected development-only auth response:

    status ok
    reason dev_allow_camera_ready

Stop daemon mode with Ctrl+C.

## Socket operations

Test ping:

    ./scripts/test-socket-client.sh ping

Test camera status:

    ./scripts/test-socket-client.sh camera_status

Test auth:

    ./scripts/test-socket-client.sh auth

## Security notes

The socket server currently uses SO_PEERCRED to inspect the connected peer process.

Current prototype policy:

- same UID is allowed
- other users are rejected

Auth behavior:

- default auth fails closed
- failed auth attempts are counted in memory
- too many failures return too_many_attempts
- restarting the daemon resets the in-memory attempt counter
- FACE_UNLOCK_DEV_ALLOW=1 is for development testing only
- FACE_UNLOCK_DEV_ALLOW=1 must never be used as real authentication

Future PAM integration may require carefully reviewed handling for privileged PAM clients.

## Privacy

The current daemon reads frames into memory only.

Socket mode currently replies with operation status and frame readiness metadata.

It does not save images, face crops, embeddings, templates, or logs containing biometric data.

## Root peer policy for sudo

The daemon allows root-owned socket peers only for auth requests.

This supports future sudo PAM integration, where pam_face_unlock.so may connect as UID 0.

Policy:

- same UID: allowed
- root UID 0 with auth operation: allowed
- root UID 0 with non-auth operation: rejected
- other users: rejected

See:

    docs/sudo-root-peer-policy.md

## Template-aware auth reasons

Default auth now distinguishes:

- template_missing when no encrypted template file exists
- matcher_not_implemented when a placeholder template exists but no real matcher is implemented
- too_many_attempts when max_auth_attempts is exceeded

Auth still fails closed unless development auth is explicitly enabled.

## Enrollment manifest status

Daemon socket responses include enrollment manifest status.

Possible values:

- missing
- placeholder
- real
- present_unknown
- unreadable

Current placeholder enrollment reports:

    enrollment placeholder

Real biometric enrollment is still not implemented.

## Root auth peer opt-in

Root-owned auth peers are allowed only when:

    FACE_UNLOCK_ALLOW_ROOT_AUTH=1

Default is disabled.

This keeps sudo PAM behavior opt-in.
