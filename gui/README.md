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
