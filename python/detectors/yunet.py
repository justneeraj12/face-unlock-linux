from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np

from detectors.base import Detection


class YuNetFaceDetector:
    """CPU-only YuNet wrapper using OpenCV FaceDetectorYN."""

    backend_name = "yunet"

    def __init__(
        self,
        model_path: Path,
        score_threshold: float = 0.9,
        nms_threshold: float = 0.3,
        top_k: int = 5000,
    ) -> None:
        if not model_path.is_file():
            raise RuntimeError(f"YuNet model not found: {model_path}")

        if not hasattr(cv2, "FaceDetectorYN_create"):
            raise RuntimeError("this OpenCV build does not provide FaceDetectorYN")

        self.model_path = model_path
        self.detector = cv2.FaceDetectorYN_create(
            str(model_path),
            "",
            (320, 320),
            score_threshold,
            nms_threshold,
            top_k,
            cv2.dnn.DNN_BACKEND_OPENCV,
            cv2.dnn.DNN_TARGET_CPU,
        )

    def detect(self, frame_bgr: np.ndarray) -> list[Detection]:
        if not isinstance(frame_bgr, np.ndarray) or frame_bgr.ndim != 3:
            raise ValueError("YuNet requires a BGR image array")

        height, width = frame_bgr.shape[:2]

        if width <= 0 or height <= 0:
            return []

        self.detector.setInputSize((width, height))
        _, faces = self.detector.detect(frame_bgr)

        if faces is None:
            return []

        detections: list[Detection] = []

        for face in faces:
            landmarks = [
                (float(face[index]), float(face[index + 1]))
                for index in range(4, 14, 2)
            ]

            detections.append(
                Detection(
                    x=int(round(float(face[0]))),
                    y=int(round(float(face[1]))),
                    w=int(round(float(face[2]))),
                    h=int(round(float(face[3]))),
                    score=float(face[14]),
                    backend=self.backend_name,
                    landmarks=landmarks,
                )
            )

        return detections
