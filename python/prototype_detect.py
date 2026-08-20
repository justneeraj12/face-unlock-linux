#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from detectors.factory import create_detector


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def summarize_mean(values: list[float]) -> float | None:
    if not values:
        return None
    return sum(values) / len(values)


def parse_synthetic_frame(value: str) -> tuple[int, int]:
    try:
        width_s, height_s = value.lower().split("x", 1)
        width = int(width_s)
        height = int(height_s)
    except Exception as exc:
        raise argparse.ArgumentTypeError("expected WIDTHxHEIGHT, for example 640x480") from exc

    if width <= 0 or height <= 0:
        raise argparse.ArgumentTypeError("width and height must be positive")

    return width, height


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Face detector prototype")

    parser.add_argument("--camera", type=int, default=0)
    parser.add_argument("--backend", choices=["auto", "haar", "noop", "yunet"], default="auto")
    parser.add_argument("--cascade", type=Path, default=None)
    parser.add_argument("--duration-seconds", type=float, default=10.0)
    parser.add_argument("--preview", action="store_true")

    parser.add_argument(
        "--synthetic-frame",
        type=parse_synthetic_frame,
        default=None,
        metavar="WIDTHxHEIGHT",
        help="Run without camera using a synthetic frame size, for example 640x480",
    )

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


def build_detector_output(
    *,
    backend: str,
    source: dict[str, Any],
    frames_total: int,
    detections_total: int,
    detect_times: list[float],
    reports: list[dict[str, Any]],
) -> dict[str, Any]:
    return {
        "format": "face-unlock-detector-output",
        "format_version": 1,
        "created_at": utc_now_iso(),
        "backend": backend,
        "source": source,
        "summary": {
            "frames_total": frames_total,
            "detections_total": detections_total,
            "mean_detect_ms": summarize_mean(detect_times),
        },
        "frames": reports,
        "privacy": {
            "raw_images_included": False,
            "face_crops_included": False,
            "safe_to_commit": False,
        },
        "warnings": [
            "detection metadata may be biometric-adjacent",
            "do not commit generated detector output",
        ],
    }


def write_output(path: Path, output: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(output, indent=2))
    print(f"metadata_written: {path}")
    print("privacy_warning: detection metadata is local-only and should not be committed")


def run_synthetic(args: argparse.Namespace) -> int:
    width, height = args.synthetic_frame

    if args.preview:
        print("ERROR: --preview cannot be used with --synthetic-frame")
        return 1

    detector = create_detector(args.backend, cascade=args.cascade)

    print("detector_status: started")
    print(f"backend: {args.backend}")
    print("source_type: synthetic")
    print(f"synthetic_width: {width}")
    print(f"synthetic_height: {height}")
    print(f"write_metadata: {str(args.write_metadata).lower()}")

    before = time.perf_counter()
    detections = detector.detect(object())
    detect_ms = (time.perf_counter() - before) * 1000.0

    reports = [
        {
            "timestamp": time.time(),
            "frame_index": 1,
            "backend": args.backend,
            "detect_ms": detect_ms,
            "detections": [d.to_dict() for d in detections],
        }
    ]

    output = build_detector_output(
        backend=args.backend,
        source={
            "type": "synthetic",
            "frame_width": width,
            "frame_height": height,
        },
        frames_total=1,
        detections_total=len(detections),
        detect_times=[detect_ms],
        reports=reports,
    )

    if args.write_metadata:
        write_output(args.output, output)

    print(
        "detect_report:"
        f" frames_total=1"
        f" faces={len(detections)}"
        f" detections_total={len(detections)}"
        f" detect_ms={detect_ms:.3f}"
    )

    print("detector_status: stopped")
    print("frames_total: 1")
    print(f"detections_total: {len(detections)}")
    print("status: ok")

    return 0


def run_camera(args: argparse.Namespace) -> int:
    import cv2

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
    detect_times: list[float] = []
    frame_width = 640
    frame_height = 480

    while True:
        now = time.monotonic()

        if now - start_time >= args.duration_seconds:
            break

        ok, frame = cap.read()

        if not ok or frame is None:
            print("frame_warning: read_failed")
            continue

        frames_total += 1
        frame_height = int(frame.shape[0])
        frame_width = int(frame.shape[1])

        before = time.perf_counter()
        detections = detector.detect(frame)
        detect_ms = (time.perf_counter() - before) * 1000.0
        detect_times.append(detect_ms)
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
        output = build_detector_output(
            backend=args.backend,
            source={
                "type": "camera",
                "camera_index": args.camera,
                "frame_width": frame_width,
                "frame_height": frame_height,
            },
            frames_total=frames_total,
            detections_total=detections_total,
            detect_times=detect_times,
            reports=reports,
        )

        write_output(args.output, output)

    print("detector_status: stopped")
    print(f"frames_total: {frames_total}")
    print(f"detections_total: {detections_total}")
    print("status: ok")

    return 0


def main() -> int:
    args = parse_args()

    if args.write_metadata and not args.i_understand_biometric_risk:
        print(
            "ERROR: --write-metadata requires --i-understand-biometric-risk\n"
            "Face detection metadata may be biometric-adjacent. Do not commit it."
        )
        return 1

    if args.synthetic_frame is not None:
        return run_synthetic(args)

    return run_camera(args)


if __name__ == "__main__":
    raise SystemExit(main())
