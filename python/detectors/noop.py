from __future__ import annotations

import numpy as np

from detectors.base import Detection


class NoopFaceDetector:
    backend_name = "noop"

    def detect(self, frame_bgr: np.ndarray) -> list[Detection]:
        return []
