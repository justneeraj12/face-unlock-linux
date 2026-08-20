from __future__ import annotations

from pathlib import Path

from detectors.base import FaceDetector
from detectors.haar import HaarFaceDetector
from detectors.noop import NoopFaceDetector
from detectors.yunet import YuNetFaceDetector


def create_detector(backend: str, cascade: Path | None = None) -> FaceDetector:
    if backend == "auto":
        try:
            return HaarFaceDetector(cascade_path=cascade)
        except Exception as exc:
            print(f"detector_auto_warning: falling back to noop: {exc}")
            return NoopFaceDetector()

    if backend == "haar":
        return HaarFaceDetector(cascade_path=cascade)

    if backend == "noop":
        return NoopFaceDetector()

    if backend == "yunet":
        return YuNetFaceDetector()

    raise ValueError(f"unknown detector backend: {backend}")
