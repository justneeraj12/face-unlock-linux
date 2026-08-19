# GUI

This directory contains the optional Qt6 enrollment GUI scaffold.

## Current status

The GUI currently provides:

- title screen
- safety warning
- consent text
- current status text
- I understand button
- Forget me placeholder button
- Close button

The GUI does not yet:

- access the camera
- save face crops
- create templates
- delete templates
- modify PAM
- perform authentication

## Build

The GUI is optional.

Configure with:

    cmake -S . -B build-gui -DBUILD_GUI=ON

Build:

    cmake --build build-gui

Run:

    ./build-gui/gui/face-unlock-enroll

## Planned GUI responsibilities

Future versions should provide:

- consent flow
- live camera preview
- guided multi-angle enrollment
- low-light detection
- optional brightness boost with consent
- image quality checks
- encrypted template creation
- enrollment manifest creation
- forget-me deletion flow

## Safety

The GUI must not silently store biometric data.

Any future feature that saves face crops, embeddings, or templates must require explicit user consent.

## Template status and Forget Me

The GUI displays whether the prototype encrypted template and enrollment manifest exist.

The Forget Me button deletes:

    ~/.local/share/face-unlock/template.enc
    ~/.local/share/face-unlock/enrollment.json

after confirmation.

It does not modify PAM or authentication settings.

## Brightness assist placeholder

The GUI includes a placeholder for future brightness assist.

It currently shows an explanation dialog only.

It does not change screen brightness.

## Pose slots scaffold

The GUI includes placeholder pose slots:

- Center
- Left
- Right
- Up
- Down

Current buttons only mark/reset demo state.

No images are captured or saved.

## Quality checklist scaffold

The GUI includes placeholder quality checklist items:

- Lighting OK
- Sharpness OK
- Face centered
- Pose coverage OK
- Template ready

Current buttons only mark/reset demo state.

No camera analysis is performed yet.
