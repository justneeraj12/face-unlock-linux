#!/usr/bin/env python3

"""
Safe camera and face-detection prototype.

Default behavior:
- opens camera
- detects faces using OpenCV Haar cascade
- prints status
- saves nothing

Saving face crops requires both:
- --save-crops
- --i-understand-privacy-risk

This script is for prototyping only.
It is not part of the trusted authentication path.
"""

from __future__ import annotations

import argparse
import json
import time
from pathlib import Path
from typing import Any

import cv2


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Safe OpenCV face capture prototype for face-unlock-linux"
    )

    parser.add_argument(
        "--camera",
        type=int,
        default=0,
        help="Camera index to open. Default: 0",
    )

    parser.add_argument(
        "--cascade",
        type=Path,
        default=None,
        help="Optional path to Haar cascade XML file",
    )

    parser.add_argument(
        "--duration-seconds",
        type=float,
        default=10.0,
        help="How long to run. Default: 10 seconds",
    )

    parser.add_argument(
        "--preview",
        action="store_true",
        help="Show OpenCV preview window",
    )

    parser.add_argument(
        "--save-crops",
        action="store_true",
        help="Save detected face crops. Requires --i-understand-privacy-risk",
    )

    parser.add_argument(
        "--i-understand-privacy-risk",
        action="store_true",
        help="Required with --save-crops because face crops are biometric data",
    )

    parser.add_argument(
        "--out-dir",
        type=Path,
        default=Path("enrollment_samples"),
        help="Output directory for crops and metadata when --save-crops is used",
    )

    parser.add_argument(
        "--max-crops",
        type=int,
        default=20,
        help="Maximum crops to save when --save-crops is used",
    )

    return parser.parse_args()


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
        "Install opencv-data/python3-opencv or pass --cascade /path/to/xml"
    )


def load_detector(cascade_override: Path | None = None) -> cv2.CascadeClassifier:
    cascade_path = cascade_override if cascade_override is not None else find_default_cascade()

    detector = cv2.CascadeClassifier(str(cascade_path))

    if detector.empty():
        raise RuntimeError(f"failed to load Haar cascade: {cascade_path}")

    print(f"detector_cascade: {cascade_path}")

    return detector


def ensure_privacy_consent(args: argparse.Namespace) -> None:
    if args.save_crops and not args.i_understand_privacy_risk:
        raise SystemExit(
            "ERROR: --save-crops requires --i-understand-privacy-risk\n"
            "Face crops are biometric data. Do not commit them to Git."
        )


def prepare_output(args: argparse.Namespace) -> Path | None:
    if not args.save_crops:
        return None

    args.out_dir.mkdir(parents=True, exist_ok=True)

    crops_dir = args.out_dir / "crops"
    crops_dir.mkdir(parents=True, exist_ok=True)

    return crops_dir


def mean_luma(gray_frame: Any, x: int, y: int, w: int, h: int) -> float:
    region = gray_frame[y : y + h, x : x + w]

    if region.size == 0:
        return 0.0

    return float(region.mean())


def save_crop(
    frame: Any,
    crops_dir: Path,
    crop_index: int,
    x: int,
    y: int,
    w: int,
    h: int,
) -> Path:
    pad = int(0.15 * max(w, h))

    x1 = max(0, x - pad)
    y1 = max(0, y - pad)
    x2 = min(frame.shape[1], x + w + pad)
    y2 = min(frame.shape[0], y + h + pad)

    crop = frame[y1:y2, x1:x2]

    if crop.size == 0:
        raise RuntimeError("empty crop")

    crop_112 = cv2.resize(crop, (112, 112))

    path = crops_dir / f"face_{crop_index:04d}.jpg"
    cv2.imwrite(str(path), crop_112)

    return path


def run_capture(args: argparse.Namespace) -> int:
    ensure_privacy_consent(args)

    detector = load_detector(args.cascade)
    crops_dir = prepare_output(args)

    cap = cv2.VideoCapture(args.camera, cv2.CAP_V4L2)

    if not cap.isOpened():
        print(f"camera_status: open_failed camera_index={args.camera}")
        return 2

    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

    print("prototype_status: started")
    print(f"camera_index: {args.camera}")
    print(f"preview_enabled: {str(args.preview).lower()}")
    print(f"save_crops: {str(args.save_crops).lower()}")

    if args.save_crops:
        print(f"out_dir: {args.out_dir}")
        print("privacy_warning: saving face crops; do not commit them to Git")

    start_time = time.monotonic()
    last_report = start_time
    frames_total = 0
    crops_saved = 0
    metadata: list[dict[str, Any]] = []

    while True:
        now = time.monotonic()

        if now - start_time >= args.duration_seconds:
            break

        ok, frame = cap.read()

        if not ok or frame is None:
            print("frame_warning: read_failed")
            continue

        frames_total += 1

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        faces = detector.detectMultiScale(
            gray,
            scaleFactor=1.1,
            minNeighbors=5,
            minSize=(60, 60),
        )

        if args.save_crops and crops_dir is not None and crops_saved < args.max_crops:
            for (x, y, w, h) in faces:
                if crops_saved >= args.max_crops:
                    break

                try:
                    crop_path = save_crop(frame, crops_dir, crops_saved, x, y, w, h)
                except RuntimeError:
                    continue

                metadata.append(
                    {
                        "file": str(crop_path),
                        "timestamp": time.time(),
                        "bbox": [int(x), int(y), int(w), int(h)],
                        "mean_luma": mean_luma(gray, int(x), int(y), int(w), int(h)),
                    }
                )

                crops_saved += 1
                print(f"crop_saved: {crop_path}")

        if args.preview:
            for (x, y, w, h) in faces:
                cv2.rectangle(
                    frame,
                    (int(x), int(y)),
                    (int(x + w), int(y + h)),
                    (0, 220, 0),
                    2,
                )

            cv2.imshow("face-unlock prototype capture", frame)

            if cv2.waitKey(1) & 0xFF == ord("q"):
                break

        if now - last_report >= 1.0:
            print(
                "capture_report:"
                f" frames_total={frames_total}"
                f" faces={len(faces)}"
                f" crops_saved={crops_saved}"
            )
            last_report = now

    cap.release()

    if args.preview:
        cv2.destroyAllWindows()

    if args.save_crops:
        metadata_path = args.out_dir / "metadata.json"
        metadata_path.write_text(json.dumps(metadata, indent=2))
        print(f"metadata_written: {metadata_path}")

    print("prototype_status: stopped")
    print(f"frames_total: {frames_total}")
    print(f"crops_saved: {crops_saved}")
    print("status: ok")

    return 0


def main() -> int:
    args = parse_args()
    return run_capture(args)


if __name__ == "__main__":
    raise SystemExit(main())
