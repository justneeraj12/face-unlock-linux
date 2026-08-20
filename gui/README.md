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

## Camera preview placeholder

The GUI includes a placeholder panel for future camera preview.

It currently does not access the camera or save frames.

## Daemon detector status query

The GUI can query the running daemon for detector_status and display the raw JSON response.

The daemon must be running separately.

This does not start camera capture in the GUI.

## Daemon response panel

The GUI displays the latest detector_status daemon response in a persistent panel.

## Parsed detector summary

The GUI shows a small parsed summary from detector_status:

- daemon available
- detector backend
- faces detected

The raw JSON response remains visible.

## Tabbed layout

The GUI uses scrollable tabs:

- Status
- Enrollment
- Privacy

This keeps the scaffold usable on laptop displays.

## Tabbed layout

The GUI uses scrollable tabs:

- Status
- Enrollment
- Privacy

This keeps the enrollment scaffold usable on laptop displays.

## Daemon JSON parsing

The GUI uses Qt JSON APIs to parse daemon detector_status responses.

## template_status query

The GUI can query template_status and show parsed template/key/decryptability metadata.
