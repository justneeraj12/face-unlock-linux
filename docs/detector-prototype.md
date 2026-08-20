# Detector Prototype

This document describes the Python detector prototype.

## Script

    python/prototype_detect.py

## Current backend

Implemented:

    haar

Placeholder:

    yunet

## Safe default

By default, the detector prototype saves nothing.

Run:

    python3 python/prototype_detect.py --camera 0 --duration-seconds 10

## Preview

Show preview:

    python3 python/prototype_detect.py --camera 0 --preview

Press q to exit early.

## Metadata writing

Detection metadata may be biometric-adjacent.

Writing metadata requires:

    --write-metadata
    --i-understand-biometric-risk

Example:

    python3 python/prototype_detect.py --camera 0 --write-metadata --i-understand-biometric-risk

Default output:

    enrollment_samples/detections.json

Do not commit generated metadata.

## Backends

### haar

OpenCV Haar cascade baseline.

Pros:

- simple
- available locally
- good for plumbing tests

Cons:

- not robust enough for production

### yunet

Planned future backend.

Goal:

- modern lightweight face detector
- OpenCV ecosystem
- better production candidate than Haar

## Future work

- add YuNet backend
- add detector metrics
- add bbox quality checks
- add GUI preview overlay
- add daemon detector abstraction

## auto and noop backends

The default backend is:

    auto

auto tries Haar first.

If the local Python cv2 module does not provide CascadeClassifier or a Haar cascade cannot be found, auto falls back to:

    noop

noop returns zero detections but keeps the camera/prototype pipeline running.

Run explicit noop mode:

    python3 python/prototype_detect.py --backend noop --camera 0 --duration-seconds 5

If you want Haar specifically:

    python3 python/prototype_detect.py --backend haar --camera 0 --duration-seconds 5

If Haar fails, check your Python OpenCV installation.

## Smoke test

Run:

    ./scripts/test-python-detectors.sh

This test does not require a camera.

It verifies:

- detector package imports
- noop backend works
- unknown backend fails safely
- prototype_detect.py help includes detector options

## Lazy backend imports

The detector factory imports Haar/YuNet backends lazily.

This allows the noop backend and smoke tests to run even when Python OpenCV is unavailable or incomplete.

## Dependency-free smoke test

The detector smoke test intentionally uses the noop backend and does not require NumPy, OpenCV, or camera hardware.

This allows CI to run the smoke test before installing apt dependencies.

## Detector output format

The planned detector metadata output format is documented in:

    docs/detector-output-format.md

Example:

    schemas/detector-output.example.json

Schema scaffold:

    schemas/detector-output.schema.json

## Current metadata output

prototype_detect.py now writes the face-unlock-detector-output format.

Validate output with:

    scripts/validate-detector-output.py path/to/detections.json
