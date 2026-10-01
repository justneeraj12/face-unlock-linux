# Hyprland and Hyprlock

Hyprland is the primary desktop-integration development target on Ubuntu 26.04.
The core daemon, Qt enrollment app, encrypted profiles, and PAM client remain
desktop-independent.

## Verified development environment

The compatibility baseline currently exercised locally is:

- Ubuntu 26.04.1 LTS, x86_64
- Linux 7.0
- Hyprland 0.53.3
- Hyprlock 0.9.2
- OpenCV 4.10
- GCC 15 and CMake 4.2

Ubuntu 24.04 with OpenCV 4.6 remains in the CI matrix.

## Current boundary

Hyprlock authenticates through `/etc/pam.d/hyprlock`. On the verified Ubuntu
package, the service delegates password authentication through:

    auth include login

The safe proposed addition is placed before that fallback:

    auth sufficient pam_face_unlock.so timeout_ms=1000
    auth include login

A failed face request therefore continues to the existing password stack.
Nothing in the repository applies this change automatically.

Hyprlock 0.9 normally submits PAM authentication after user input and Enter.
Adding a PAM module does not create Android-style automatic background unlock.
Automatic face attempts, live status, and screen illumination require a small
reviewed Hyprlock-native integration or an accepted upstream interface. Killing
the locker, injecting input, or treating process exit as authentication are
explicitly rejected designs.

Hyprlock protects an already-running session. Greetd, SDDM, GDM, and other
login/greeter paths are separate integrations and are not enabled by this work.

## Read-only planning

Inspect the installed service and generate a proposed diff:

    ./scripts/plan-hyprlock-pam-install.sh

The planner prints:

- the exact target
- the proposed insertion point and diff
- the backup path
- the rollback command
- the automatic-unlock limitation

It never writes under `/etc`.

The guarded installer is also dry-run by default:

    ./scripts/apply-hyprlock-pam-install.sh

Its `--apply` mode requires the multiarch PAM module, daemon socket, explicit
biometric success, recognizable password fallback, and typed confirmation. It
explicitly rejects `dev_allow_camera_ready` success and currently refuses to
apply because production biometric decisions remain disabled.

Rollback tooling is prepared for future controlled tests:

    ./scripts/rollback-hyprlock-pam.sh /etc/pam.d/hyprlock.face-unlock-backup.TIMESTAMP

## Regression test

The dry-run regression operates only on a temporary PAM fixture:

    ./scripts/test-hyprlock-dry-run.sh

It verifies both planners, checks the proposed line, and proves the fixture hash
did not change.

## Low-light illumination

Only the active session locker can safely draw a bright neutral surface over a
Wayland session lock. The daemon may request illumination and wait for an
acknowledgement, but it must not change saved brightness, draw an overlay, or
unlock the compositor itself. Hyprlock support therefore needs a native status
and illumination channel before this feature can be enabled.

## Safety status

Safe today:

- build and run the daemon and enrollment GUI under Hyprland
- run CPU model tests and benchmarks
- inspect the Hyprlock PAM plan
- run fixture-only dry-run tests

Not enabled today:

- real face-auth success
- automatic Hyprlock unlocking
- live PAM modification
- greetd or display-manager login integration
- low-light lock-surface illumination
