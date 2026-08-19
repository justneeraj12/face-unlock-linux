# GUI Enrollment App

This document describes the optional Qt6 enrollment GUI.

## Current status

The GUI is a scaffold only.

It shows:

- project title
- safety warning
- consent text
- current status text
- I understand button
- Forget me placeholder button

It does not access the camera yet.

It does not save biometric data.

## Build

Install Qt6 development packages:

    sudo apt install qt6-base-dev

Configure:

    cmake -S . -B build-gui -DBUILD_GUI=ON

Build:

    cmake --build build-gui

Run:

    ./build-gui/gui/face-unlock-enroll

## Planned enrollment flow

Future enrollment should guide the user through:

1. consent
2. camera permission check
3. lighting check
4. center pose
5. left pose
6. right pose
7. up pose
8. down pose
9. quality summary
10. encrypted template save
11. enrollment manifest save

## Planned forget-me flow

Future forget-me should delete:

    ~/.local/share/face-unlock/template.enc
    ~/.local/share/face-unlock/enrollment.json

It should confirm deletion and report final status.

## Brightness boost

Future brightness boost must:

- require explicit user consent
- save previous brightness
- boost only temporarily
- restore previous brightness immediately
- fail safely if brightness control is unavailable

## Privacy

The GUI must not save raw images by default.

Any future saving of crops or embeddings must require explicit consent.

## Template status and Forget Me

The GUI scaffold now displays status for:

    ~/.local/share/face-unlock/template.enc
    ~/.local/share/face-unlock/enrollment.json

The Forget Me button deletes these prototype files after confirmation.

Forget Me does not modify:

    /etc/pam.d/sudo
    /etc/pam.d/common-auth
    GDM
    SDDM
    LightDM
    lock-screen configuration

This is still not real enrollment.

## Brightness assist placeholder

The GUI includes a brightness assist placeholder button.

It explains planned behavior but does not change brightness.

Design document:

    docs/brightness-assist.md

## Pose slots scaffold

The GUI includes a non-camera pose slot scaffold.

Current pose slots:

- Center
- Left
- Right
- Up
- Down

The current UI can mark demo slots complete and reset them.

This does not capture images or save enrollment data.

Future versions should connect pose slots to camera preview, face quality checks, and encrypted template creation.

## GUI CI

The optional GUI has a separate GitHub Actions workflow:

    .github/workflows/gui-build.yml

It can be run manually from the GitHub Actions tab.

The main CI workflow does not build the GUI by default.

## Quality checklist scaffold

The GUI includes a quality checklist scaffold.

Current checklist items:

- Lighting OK
- Sharpness OK
- Face centered
- Pose coverage OK
- Template ready

The current UI can mark/reset demo quality state.

No camera analysis is performed yet.

Future versions should compute these from camera frames and enrollment metadata.

## Camera preview placeholder

The GUI includes a camera preview placeholder panel.

It does not access the camera yet.

Design document:

    docs/gui-camera-preview.md
