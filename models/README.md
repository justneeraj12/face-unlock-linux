# Models

Model weights are local build/runtime inputs and are not committed to Git.

## Development baseline

The pinned CPU baseline is:

| Purpose | Model |
|---|---|
| Detection | YuNet 2022mar |
| Alignment and embedding | SFace 2021dec |

Download and verify both files:

    ./scripts/download-cpu-models.sh

Verify existing files without downloading:

    ./scripts/download-cpu-models.sh --check

The downloader pins upstream revisions and SHA-256 checksums. YuNet 2022mar is
used because it is compatible with Ubuntu 24.04's stock OpenCV 4.6.

## Local files

Expected ignored paths:

    models/face_detection_yunet_2022mar.onnx
    models/face_recognition_sface_2021dec.onnx
    models/embedding_stub.pt

The TorchScript file is only a loader stub and is not a recognition model.

## Release requirement

Before a package redistributes model weights, maintainers must document:

- source and exact revision
- license and redistribution rights
- training-data and provenance concerns
- input, output, and preprocessing
- measured CPU performance
- threshold calibration
- accuracy and spoofing limitations

Never commit private biometric data with a model evaluation.
