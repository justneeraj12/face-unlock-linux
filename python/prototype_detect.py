#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import time
from pathlib import Path
from typing import Any

import cv2

from detectors.factory import create_detector


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Face detector prototype")

    parser.add_argument("--camera", type=int, default=0)
    parser.add_argument("--backend", choices=["auto", "haar", "noop", "yunet"], default="auto")
    parser.add_argument("--cascade", type=Path, default=None)
    parser.add_argument("--duration-seconds", type=float, default=10.0)
    parser.add_argument("--preview", action="store_true")

    parser.add_argument(
        "--write-metadata",
        action="store_true",
        help="Write detection metadata. Requires --i-understand-biometric-risk",
    )

    parser.add_argument(
        "--i-understand-biometric-risk",
        action="store_true",
        help="Required to write metadata derived from camera/face detections",
    )

    parser.add_argument(
        "--output",
        type=Path,
        default=Path("enrollment_samples/detections.json"),
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.write_metadata and not args.i_understand_biometric_risk:
        print(
            "ERROR: --write-metadata requires --i-understand-biometric-risk\n"
            "Face detection metadata may be biometric-adjacent. Do not commit it."
        )
        return 1

    detector = create_detector(args.backend, cascade=args.cascade)

    cap = cv2.VideoCapture(args.camera, cv2.CAP_V4L2)

    if not cap.isOpened():
        print(f"camera_status: open_failed camera_index={args.camera}")
        return 2

    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

    print("detector_status: started")
    print(f"backend: {args.backend}")
    print(f"camera_index: {args.camera}")
    print(f"preview_enabled: {str(args.preview).lower()}")
    print(f"write_metadata: {str(args.write_metadata).lower()}")

    start_time = time.monotonic()
    last_report = start_time
    frames_total = 0
    detections_total = 0
    reports: list[dict[str, Any]] = []

    while True:
        now = time.monotonic()

        if now - start_time >= args.duration_seconds:
            break

        ok, frame = cap.read()

        if not ok or frame is None:
            print("frame_warning: read_failed")
            continue

        frames_total += 1

        before = time.perf_counter()
        detections = detector.detect(frame)
        detect_ms = (time.perf_counter() - before) * 1000.0
        detections_total += len(detections)

        if args.write_metadata:
            reports.append(
                {
                    "timestamp": time.time(),
                    "frame_index": frames_total,
                    "backend": args.backend,
                    "detect_ms": detect_ms,
                    "detections": [d.to_dict() for d in detections],
                }
            )

        if args.preview:
            for detection in detections:
                x, y, w, h = detection.x, detection.y, detection.w, detection.h
                cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 220, 0), 2)

            cv2.imshow("face-unlock detector prototype", frame)

            if cv2.waitKey(1) & 0xFF == ord("q"):
                break

        if now - last_report >= 1.0:
            print(
                "detect_report:"
                f" frames_total={frames_total}"
                f" faces={len(detections)}"
                f" detections_total={detections_total}"
                f" detect_ms={detect_ms:.3f}"
            )
            last_report = now

    cap.release()

    if args.preview:
        cv2.destroyAllWindows()

    if args.write_metadata:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(reports, indent=2))
        print(f"metadata_written: {args.output}")
        print("privacy_warning: detection metadata is local-only and should not be committed")

    print("detector_status: stopped")
    print(f"frames_total: {frames_total}")
    print(f"detections_total: {detections_total}")
    print("status: ok")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
