# Brightness Assist

This document describes the planned brightness assist behavior for enrollment.

## Current status

Brightness assist is not implemented yet.

The Qt GUI currently includes a placeholder button that explains the planned behavior.

No brightness changes are performed.

## Purpose

During enrollment, low light can reduce face image quality.

A future enrollment GUI may temporarily increase screen brightness to illuminate the user's face during snapshots.

## Safety requirements

Brightness assist must:

- require explicit user consent
- explain why brightness is being changed
- store the previous brightness value
- restore the previous brightness immediately after capture
- restore brightness on error
- restore brightness on cancellation
- restore brightness on application exit
- fail safely if brightness control is unavailable
- avoid running with elevated privileges when possible

## Privacy requirements

Brightness assist must not:

- save images by itself
- trigger enrollment without consent
- enable telemetry
- log raw images
- log biometric data

## Possible Linux implementations

Potential implementation paths:

- desktop portal if available
- DBus desktop environment APIs
- power management DBus APIs
- /sys/class/backlight for direct backlight control

Direct /sys/class/backlight writes may require permissions and must be handled carefully.

## Wayland considerations

Wayland limits direct control of desktop/session behavior.

Brightness control may depend on:

- desktop environment
- compositor
- hardware
- permissions
- laptop backlight driver

The GUI must handle unsupported systems gracefully.

## Planned flow

Future flow:

1. detect low light during preview
2. ask user for consent
3. read current brightness
4. temporarily boost brightness
5. capture enrollment snapshot
6. restore previous brightness
7. report status to user

## Current GUI behavior

The current GUI button only shows an informational dialog.

It does not change brightness.
