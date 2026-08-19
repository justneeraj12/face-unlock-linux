# Python Prototypes

This document describes Python prototype scripts.

Python scripts are for rapid experimentation only.

They are not part of the trusted authentication path.

## Current script

    python/prototype_capture.py

## Purpose

The prototype capture script can:

- open a webcam
- run a basic OpenCV Haar face detector
- print frame and face detection status
- optionally show a preview window
- optionally save face crops with explicit privacy consent

## Privacy default

By default, the script saves nothing.

Default safe run:

    python3 python/prototype_capture.py --camera 0 --duration-seconds 10

This reads frames into memory, prints status, and exits.

## Preview mode

Show a preview window:

    python3 python/prototype_capture.py --camera 0 --preview

Press q in the preview window to stop early.

## Saving crops

Face crops are biometric data.

Saving crops requires both:

    --save-crops
    --i-understand-privacy-risk

Example:

    python3 python/prototype_capture.py --camera 0 --preview --save-crops --i-understand-privacy-risk --out-dir enrollment_samples

The output directory is ignored by Git.

Do not commit:

- raw face images
- face crops
- embeddings
- templates
- encrypted templates
- metadata containing sensitive biometric details

## Output

When saving crops, the script writes:

    enrollment_samples/crops/
    enrollment_samples/metadata.json

## Detector

The current detector is OpenCV Haar cascade:

    haarcascade_frontalface_default.xml

This is only a simple prototype detector.

Future detector options may include:

- YuNet
- MTCNN
- RetinaFace
- MediaPipe face detection

## Requirements

Ubuntu package option:

    sudo apt install python3-opencv

Python venv option:

    python3 -m venv .venv
    source .venv/bin/activate
    pip install -r python/requirements.txt

## Safety

Prototype scripts must not silently save biometric data.

Any script that saves face crops or templates must require explicit user consent.

## Embedding prototype

The embedding prototype is documented in:

    docs/python-embedding-prototype.md

Script:

    python/prototype_embed.py

It uses a random stub model and is not real face recognition.

Writing embeddings requires explicit biometric-risk consent.
