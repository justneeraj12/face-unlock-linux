from __future__ import annotations

from typing import Any

from detectors.base import Detection


class NoopFaceDetector:
    backend_name = "noop"

    def detect(self, frame_bgr: Any) -> list[Detection]:
        return []
