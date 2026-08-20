# GUI Camera Preview

This document describes the planned GUI camera preview.

## Current status

The GUI includes a camera preview placeholder panel.

No camera frames are read by the GUI yet.

No images are saved.

## Purpose

Future enrollment requires a live preview so users can position their face and follow pose instructions.

The preview should support:

- camera availability status
- face visible indication
- pose guidance
- low-light warnings
- quality feedback
- capture progress

## Safety requirements

The preview must:

- not save frames by default
- not log raw images
- clearly indicate when camera is active
- stop camera on window close
- release camera resources on errors
- avoid running as root

## Future implementation options

Possible approaches:

- Qt Multimedia camera APIs
- OpenCV capture thread feeding Qt image widget
- shared capture logic with daemon
- daemon-provided preview metadata only

## Privacy

A live preview is sensitive.

Future versions must clearly tell the user:

- when the camera is active
- whether anything is being saved
- where templates are stored
- how to delete enrollment data

## Current GUI behavior

The current panel is visual only.

It does not access the camera.

## Daemon status bridge

Before implementing live camera preview, the GUI can query daemon detector metadata with:

    detector_status

This lets the GUI display daemon-side detector state without directly accessing the camera.
