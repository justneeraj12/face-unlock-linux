# Lock-screen authentication policy

This document defines the bounded authentication policy intended for a future
GNOME Shell lock-screen integration. The policy controller is implemented and
tested. Camera leasing, biometric matching, screen illumination, and unlock
approval are not connected yet.

## Policy

A lock-screen attempt has these defaults:

| Setting | Default |
|---|---:|
| Qualified face candidates | 3 maximum |
| Recognition window | 1000 ms |
| Illumination settle time | 200 ms |
| Low-light luma threshold | 50/255 |
| Password fallback | Always available |

A frame counts as a candidate only when the integration reports that a frame is
available, exactly one face is present, and all quality gates pass. Missing,
blurred, badly exposed, occluded, or ambiguous frames do not consume one of the
three candidates. The deadline still applies so camera or model failures cannot
hold the lock screen indefinitely.

```mermaid
stateDiagram-v2
    [*] --> WaitingForFrame
    WaitingForFrame --> IlluminationRequested: low light
    IlluminationRequested --> IlluminationSettling: lock screen acknowledges
    IlluminationSettling --> WaitingForFrame: 200 ms elapsed
    WaitingForFrame --> WaitingForFrame: no frame or quality rejected
    WaitingForFrame --> Comparing: one qualified face
    Comparing --> Unlocked: match and all checks pass
    Comparing --> WaitingForFrame: mismatch and candidates remain
    Comparing --> Password: third qualified mismatch
    WaitingForFrame --> Password: 1000 ms deadline
    IlluminationRequested --> Password: 1000 ms deadline
    IlluminationSettling --> Password: 1000 ms deadline
    WaitingForFrame --> Cancelled: password entry starts
    IlluminationRequested --> Cancelled: password entry starts
    IlluminationSettling --> Cancelled: password entry starts
    Comparing --> Cancelled: password entry starts
```

## Low-light behavior

The daemon must never change display brightness or draw over the lock screen.
Only the GNOME Shell lock-screen component can present a temporary neutral
illumination surface. The planned handshake is:

1. The daemon reports that the scene is below the low-light threshold.
2. The lock-screen component displays its illumination surface.
3. It acknowledges that the surface is visible.
4. The controller ignores frames during the 200 ms camera-exposure settling
   interval.
5. Authentication resumes with newly captured frames.
6. The surface is removed immediately after success, fallback, cancellation,
   or timeout.

This design does not require changing the user's saved brightness setting.
Screen illumination is an accessibility-sensitive feature and must have a
user-visible opt-out.

## Camera lifecycle requirement

The production integration must acquire the camera only for an active
lock-screen attempt and release it on every terminal path:

- explicit match success
- three qualified mismatches
- deadline reached
- password entry begins
- lock screen closes
- daemon, model, profile, or camera error

The existing daemon camera worker is not yet an on-demand camera lease. That
work must be implemented and benchmarked before this policy is connected to a
real lock screen.

## Protocol capability

A same-user client can query:

    ./scripts/test-socket-client.sh lockscreen_policy

The response exposes protocol version 1 and the policy constants. It reports:

    "implementation_status":"policy_ready_integration_pending"

This operation is read-only. It cannot start authentication, illuminate the
screen, or unlock a session.

## Security boundary

The future GNOME Shell component may render status and illumination, but it
must not decide whether a face matched. The daemon owns biometric evaluation,
uses a bounded request, and returns an explicit decision. Password input must
cancel face processing immediately, and password or PIN fallback must remain
available.

The initial desktop target is an already-running GNOME user session in
`unlock-dialog` mode. Display-manager login is a separate integration with a
different trust and process model; this policy does not enable GDM login.
