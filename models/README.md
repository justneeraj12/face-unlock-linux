# Models

Model weights are local build/runtime inputs and are not committed to Git.

## Development baseline

The pinned CPU baseline is:

| Purpose | Model |
|---|---|
| Detection | YuNet 2022mar on OpenCV 4.6; YuNet 2023mar on newer OpenCV 4.x |
| Alignment and embedding | SFace 2021dec |

Download and verify both files:

    ./scripts/download-cpu-models.sh

Verify existing files without downloading:

    ./scripts/download-cpu-models.sh --check

The downloader pins upstream revisions and SHA-256 checksums. It detects the
installed OpenCV version and selects a compatible YuNet file. The stable
`face_detection_yunet.onnx` symlink is the path applications should use.

Inspect the choice without downloading:

    ./scripts/download-cpu-models.sh --select-only

`FACE_UNLOCK_YUNET_VARIANT=2022mar|2023mar` is available for controlled tests.

## Local files

Expected ignored paths:

    models/face_detection_yunet_2022mar.onnx
    models/face_detection_yunet_2023mar.onnx
    models/face_detection_yunet.onnx
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
