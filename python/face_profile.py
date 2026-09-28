from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable

import numpy as np

from detectors.base import Detection


POSE_SLOTS = ("center", "left", "right", "up", "down")


def normalize_embedding(embedding: np.ndarray) -> np.ndarray:
    vector = np.asarray(embedding, dtype=np.float32).reshape(-1)

    if vector.size == 0 or not np.isfinite(vector).all():
        raise ValueError("embedding must contain finite values")

    norm = float(np.linalg.norm(vector))

    if norm <= 0.0:
        raise ValueError("embedding norm must be positive")

    return vector / norm


def classify_pose(detection: Detection) -> str:
    """Estimate a coarse enrollment slot from YuNet's five landmarks."""

    if len(detection.landmarks) != 5:
        return "unknown"

    right_eye, left_eye, nose, right_mouth, left_mouth = detection.landmarks
    eye_mid_x = (right_eye[0] + left_eye[0]) / 2.0
    eye_mid_y = (right_eye[1] + left_eye[1]) / 2.0
    mouth_mid_y = (right_mouth[1] + left_mouth[1]) / 2.0
    eye_span = max(abs(left_eye[0] - right_eye[0]), 1.0)
    eye_to_mouth = max(mouth_mid_y - eye_mid_y, 1.0)

    yaw = (nose[0] - eye_mid_x) / eye_span
    pitch = (nose[1] - eye_mid_y) / eye_to_mouth

    if yaw <= -0.16:
        return "left"
    if yaw >= 0.16:
        return "right"
    if pitch <= 0.40:
        return "up"
    if pitch >= 0.62:
        return "down"
    return "center"


@dataclass(frozen=True)
class AddSampleResult:
    accepted: bool
    reason: str
    pose: str
    progress_percent: int


@dataclass(frozen=True)
class FaceProfile:
    model_id: str
    embedding_dim: int
    pose_centroids: dict[str, np.ndarray]
    samples_per_pose: dict[str, int]

    def match_score(self, embedding: np.ndarray) -> float:
        query = normalize_embedding(embedding)

        if query.size != self.embedding_dim:
            raise ValueError("embedding dimension does not match profile")

        return max(
            float(np.dot(query, centroid))
            for centroid in self.pose_centroids.values()
        )

    def summary(self) -> dict:
        return {
            "model_id": self.model_id,
            "embedding_dim": self.embedding_dim,
            "pose_templates": len(self.pose_centroids),
            "samples_per_pose": dict(self.samples_per_pose),
            "contains_raw_images": False,
        }


class FaceProfileBuilder:
    """Keeps diverse embeddings in memory and produces pose-aware centroids."""

    def __init__(
        self,
        model_id: str,
        target_per_pose: int = 6,
        maximum_per_pose: int = 12,
        duplicate_similarity: float = 0.9999,
    ) -> None:
        if target_per_pose < 1:
            raise ValueError("target_per_pose must be positive")
        if maximum_per_pose < target_per_pose:
            raise ValueError("maximum_per_pose must be >= target_per_pose")

        self.model_id = model_id
        self.target_per_pose = target_per_pose
        self.maximum_per_pose = maximum_per_pose
        self.duplicate_similarity = duplicate_similarity
        self._embedding_dim: int | None = None
        self._samples: dict[str, list[np.ndarray]] = {
            pose: [] for pose in POSE_SLOTS
        }

    def progress_percent(self) -> int:
        required = self.target_per_pose * len(POSE_SLOTS)
        captured = sum(
            min(len(samples), self.target_per_pose)
            for samples in self._samples.values()
        )
        return min(100, int(round(captured * 100.0 / required)))

    def samples_per_pose(self) -> dict[str, int]:
        return {pose: len(samples) for pose, samples in self._samples.items()}

    def missing_poses(self) -> list[str]:
        return [
            pose
            for pose, samples in self._samples.items()
            if len(samples) < self.target_per_pose
        ]

    def ready(self) -> bool:
        return not self.missing_poses()

    def add_sample(
        self,
        embedding: np.ndarray,
        pose: str,
        quality_score: float,
    ) -> AddSampleResult:
        if pose not in self._samples:
            return AddSampleResult(False, "unsupported_pose", pose, self.progress_percent())

        if not np.isfinite(quality_score) or quality_score < 0.5:
            return AddSampleResult(False, "quality_too_low", pose, self.progress_percent())

        vector = normalize_embedding(embedding)

        if self._embedding_dim is None:
            self._embedding_dim = int(vector.size)
        elif vector.size != self._embedding_dim:
            return AddSampleResult(False, "embedding_dimension_mismatch", pose, self.progress_percent())

        samples = self._samples[pose]

        if len(samples) >= self.maximum_per_pose:
            return AddSampleResult(False, "pose_full", pose, self.progress_percent())

        if samples:
            most_similar = max(float(np.dot(vector, sample)) for sample in samples)

            if most_similar >= self.duplicate_similarity:
                return AddSampleResult(False, "duplicate_sample", pose, self.progress_percent())

        samples.append(vector.copy())
        return AddSampleResult(True, "accepted", pose, self.progress_percent())

    @staticmethod
    def _robust_centroid(samples: Iterable[np.ndarray]) -> np.ndarray:
        matrix = np.stack(list(samples)).astype(np.float32)
        initial = normalize_embedding(matrix.mean(axis=0))

        if matrix.shape[0] >= 5:
            similarities = matrix @ initial
            cutoff = float(np.percentile(similarities, 10.0))
            matrix = matrix[similarities >= cutoff]

        return normalize_embedding(matrix.mean(axis=0))

    def finalize(self) -> FaceProfile:
        missing = self.missing_poses()

        if missing:
            raise RuntimeError("missing enrollment poses: " + ", ".join(missing))

        if self._embedding_dim is None:
            raise RuntimeError("no enrollment embeddings captured")

        centroids = {
            pose: self._robust_centroid(samples)
            for pose, samples in self._samples.items()
        }

        return FaceProfile(
            model_id=self.model_id,
            embedding_dim=self._embedding_dim,
            pose_centroids=centroids,
            samples_per_pose=self.samples_per_pose(),
        )
