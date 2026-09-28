# Enrollment GUI

gui/ contains the optional Qt6 enrollment application.

## Current implementation

The GUI provides:

- safety and consent information
- status, enrollment, and privacy tabs
- daemon detector, template, and auth diagnostic queries
- parsed status summaries plus raw local responses
- camera preview placeholder
- center, left, right, up, and down pose scaffolds
- lighting, sharpness, centering, pose, and template quality scaffolds
- placeholder template and manifest deletion with confirmation
- brightness-assist explanation

It does not yet capture enrollment frames, run models, build a face profile, or
enable PAM authentication.

## Build

    ./scripts/build-gui.sh
    ./build-gui/gui/face-unlock-enroll

Qt6 is optional for the core daemon build.

## Next implementation

The GUI will become a client of daemon enrollment operations:

- start and cancel enrollment
- show live camera preview
- display head-turn guidance
- report real pose and quality coverage
- show profile processing and validation progress
- commit only an encrypted validated profile
- verify Forget Me removed profile material

See [GUI design](../docs/gui.md) and
[CPU face profile](../docs/cpu-face-profile.md).

## Safety

The GUI must never silently save biometric data or modify PAM configuration.
Enrollment and deletion require explicit user action.
