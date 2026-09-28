from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np

from detectors.base import Detection


class SFaceRecognizer:
    """CPU-only SFace wrapper using OpenCV FaceRecognizerSF."""

    model_id = "opencv-sface-2021dec"

    def __init__(self, model_path: Path) -> None:
        if not model_path.is_file():
            raise RuntimeError(f"SFace model not found: {model_path}")

        if not hasattr(cv2, "FaceRecognizerSF_create"):
            raise RuntimeError("this OpenCV build does not provide FaceRecognizerSF")

        self.model_path = model_path
        self.recognizer = cv2.FaceRecognizerSF_create(
            str(model_path),
            "",
            cv2.dnn.DNN_BACKEND_OPENCV,
            cv2.dnn.DNN_TARGET_CPU,
        )

    @staticmethod
    def _normalize(feature: np.ndarray) -> np.ndarray:
        vector = np.asarray(feature, dtype=np.float32).reshape(-1)
        norm = float(np.linalg.norm(vector))

        if not np.isfinite(vector).all() or norm <= 0.0:
            raise RuntimeError("SFace returned an invalid embedding")

        return vector / norm

    def embed_aligned(self, aligned_face_bgr: np.ndarray) -> np.ndarray:
        if aligned_face_bgr.shape[:2] != (112, 112):
            aligned_face_bgr = cv2.resize(aligned_face_bgr, (112, 112))

        return self._normalize(self.recognizer.feature(aligned_face_bgr))

    def align_and_embed(
        self,
        frame_bgr: np.ndarray,
        detection: Detection,
    ) -> np.ndarray:
        if len(detection.landmarks) != 5:
            raise ValueError("SFace alignment requires five YuNet landmarks")

        face = np.zeros((1, 15), dtype=np.float32)
        face[0, 0:4] = [detection.x, detection.y, detection.w, detection.h]

        for index, (x, y) in enumerate(detection.landmarks):
            face[0, 4 + index * 2] = x
            face[0, 5 + index * 2] = y

        face[0, 14] = detection.score
        aligned = self.recognizer.alignCrop(frame_bgr, face)
        return self.embed_aligned(aligned)

    @staticmethod
    def cosine_similarity(left: np.ndarray, right: np.ndarray) -> float:
        left_norm = SFaceRecognizer._normalize(left)
        right_norm = SFaceRecognizer._normalize(right)
        return float(np.dot(left_norm, right_norm))
