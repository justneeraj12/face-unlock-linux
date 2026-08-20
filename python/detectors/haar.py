from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np

from detectors.base import Detection


def find_default_cascade() -> Path:
    filename = "haarcascade_frontalface_default.xml"

    candidates: list[Path] = []

    cv2_data = getattr(cv2, "data", None)
    haarcascades = getattr(cv2_data, "haarcascades", None)

    if haarcascades:
        candidates.append(Path(haarcascades) / filename)

    candidates.extend(
        [
            Path("/usr/share/opencv4/haarcascades") / filename,
            Path("/usr/share/opencv/haarcascades") / filename,
            Path("/usr/local/share/opencv4/haarcascades") / filename,
            Path("/usr/local/share/opencv/haarcascades") / filename,
        ]
    )

    for candidate in candidates:
        if candidate.exists():
            return candidate

    searched = "\n".join(f"  - {candidate}" for candidate in candidates)
    raise RuntimeError(
        "failed to find Haar cascade XML file. Searched:\n"
        f"{searched}\n"
        "Install opencv-data/python3-opencv or pass a future cascade override."
    )


class HaarFaceDetector:
    backend_name = "haar"

    def __init__(self, cascade_path: Path | None = None) -> None:
        if not hasattr(cv2, "CascadeClassifier"):
            raise RuntimeError(
                "this cv2 build does not provide CascadeClassifier/objdetect"
            )

        self.cascade_path = cascade_path or find_default_cascade()
        self.detector = cv2.CascadeClassifier(str(self.cascade_path))

        if self.detector.empty():
            raise RuntimeError(f"failed to load cascade: {self.cascade_path}")

    def detect(self, frame_bgr: np.ndarray) -> list[Detection]:
        gray = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2GRAY)

        faces = self.detector.detectMultiScale(
            gray,
            scaleFactor=1.1,
            minNeighbors=5,
            minSize=(60, 60),
        )

        detections: list[Detection] = []

        for (x, y, w, h) in faces:
            detections.append(
                Detection(
                    x=int(x),
                    y=int(y),
                    w=int(w),
                    h=int(h),
                    score=1.0,
                    backend=self.backend_name,
                )
            )

        return detections
