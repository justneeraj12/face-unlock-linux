from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Protocol


@dataclass
class Detection:
    x: int
    y: int
    w: int
    h: int
    score: float
    backend: str
    landmarks: list[tuple[float, float]] = field(default_factory=list)

    def to_dict(self) -> dict:
        result = {
            "x": self.x,
            "y": self.y,
            "w": self.w,
            "h": self.h,
            "score": self.score,
            "backend": self.backend,
        }

        if self.landmarks:
            result["landmarks"] = [
                {"x": float(x), "y": float(y)} for x, y in self.landmarks
            ]

        return result


class FaceDetector(Protocol):
    backend_name: str

    def detect(self, frame_bgr: Any) -> list[Detection]:
        ...
