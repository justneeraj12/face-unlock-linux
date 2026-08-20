from __future__ import annotations

from dataclasses import dataclass
from typing import Protocol

import numpy as np


@dataclass
class Detection:
    x: int
    y: int
    w: int
    h: int
    score: float
    backend: str

    def to_dict(self) -> dict:
        return {
            "x": self.x,
            "y": self.y,
            "w": self.w,
            "h": self.h,
            "score": self.score,
            "backend": self.backend,
        }


class FaceDetector(Protocol):
    backend_name: str

    def detect(self, frame_bgr: np.ndarray) -> list[Detection]:
        ...
