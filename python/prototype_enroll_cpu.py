#!/usr/bin/env python3

"""CPU-only guided face-profile enrollment prototype.

Frames and aligned crops remain in memory. This prototype does not write a
profile, modify PAM, or enable authentication.
"""

from __future__ import annotations

import argparse
import time
from pathlib import Path

import cv2
import numpy as np

from detectors.yunet import YuNetFaceDetector
from face_profile import FaceProfileBuilder, classify_pose
from recognizers.sface import SFaceRecognizer


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="CPU face-profile enrollment prototype")
    parser.add_argument("--camera", type=int, default=0)
    parser.add_argument(
        "--yunet-model",
        type=Path,
        default=Path("models/face_detection_yunet.onnx"),
    )
    parser.add_argument(
        "--sface-model",
        type=Path,
        default=Path("models/face_recognition_sface_2021dec.onnx"),
    )
    parser.add_argument("--samples-per-pose", type=int, default=6)
    parser.add_argument("--maximum-seconds", type=float, default=90.0)
    parser.add_argument(
        "--i-understand-biometric-risk",
        action="store_true",
        help="Required because live face embeddings are processed in memory",
    )
    return parser.parse_args()


def quality_score(frame: np.ndarray, face_area: int) -> tuple[float, list[str]]:
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    luma = float(gray.mean())
    sharpness = float(cv2.Laplacian(gray, cv2.CV_64F).var())
    frame_area = int(frame.shape[0] * frame.shape[1])
    area_ratio = face_area / max(frame_area, 1)
    problems: list[str] = []

    if luma < 45.0:
        problems.append("too_dark")
    elif luma > 220.0:
        problems.append("too_bright")

    if sharpness < 35.0:
        problems.append("too_blurry")

    if area_ratio < 0.06:
        problems.append("move_closer")
    elif area_ratio > 0.65:
        problems.append("move_back")

    return (1.0 if not problems else 0.0), problems


def draw_status(
    frame: np.ndarray,
    expected_pose: str,
    observed_pose: str,
    progress: int,
    message: str,
) -> None:
    lines = [
        f"Turn: {expected_pose}",
        f"Observed: {observed_pose}",
        f"Building face profile: {progress}%",
        message,
        "Press q to cancel",
    ]

    for index, line in enumerate(lines):
        cv2.putText(
            frame,
            line,
            (16, 30 + index * 28),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.65,
            (40, 230, 40),
            2,
            cv2.LINE_AA,
        )


def main() -> int:
    args = parse_args()

    if not args.i_understand_biometric_risk:
        print("ERROR: enrollment requires --i-understand-biometric-risk")
        print("Frames are processed in memory and are not saved by this prototype.")
        return 1

    detector = YuNetFaceDetector(args.yunet_model)
    recognizer = SFaceRecognizer(args.sface_model)
    builder = FaceProfileBuilder(
        model_id=recognizer.model_id,
        target_per_pose=args.samples_per_pose,
        maximum_per_pose=max(args.samples_per_pose * 2, args.samples_per_pose),
    )

    camera = cv2.VideoCapture(args.camera, cv2.CAP_V4L2)

    if not camera.isOpened():
        print(f"camera_status: open_failed camera_index={args.camera}")
        return 2

    camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

    print("enrollment_status: started")
    print("runtime: opencv_cpu")
    print("raw_images_saved: false")
    print("face_crops_saved: false")

    started = time.monotonic()
    next_inference = 0.0
    last_message = "Center your face"
    cancelled = False

    try:
        while not builder.ready():
            if time.monotonic() - started >= args.maximum_seconds:
                print("enrollment_status: timed_out")
                print("missing_poses:", ",".join(builder.missing_poses()))
                return 3

            ok, frame = camera.read()

            if not ok or frame is None:
                last_message = "Camera frame unavailable"
                continue

            missing = builder.missing_poses()
            expected_pose = missing[0] if missing else "center"
            observed_pose = "unknown"
            now = time.monotonic()

            if now >= next_inference:
                detections = detector.detect(frame)
                next_inference = now + 0.12

                if len(detections) != 1:
                    last_message = "Need exactly one face"
                else:
                    detection = detections[0]
                    observed_pose = classify_pose(detection)
                    score, problems = quality_score(
                        frame,
                        max(detection.w, 0) * max(detection.h, 0),
                    )

                    if problems:
                        last_message = problems[0].replace("_", " ")
                    elif observed_pose != expected_pose:
                        last_message = f"Slowly turn {expected_pose}"
                    else:
                        before = time.perf_counter()
                        embedding = recognizer.align_and_embed(frame, detection)
                        embedding_ms = (time.perf_counter() - before) * 1000.0
                        result = builder.add_sample(embedding, observed_pose, score)
                        last_message = (
                            f"{result.reason.replace('_', ' ')} ({embedding_ms:.1f} ms)"
                        )

            draw_status(
                frame,
                expected_pose,
                observed_pose,
                builder.progress_percent(),
                last_message,
            )
            cv2.imshow("face-unlock CPU enrollment prototype", frame)

            if cv2.waitKey(1) & 0xFF == ord("q"):
                cancelled = True
                break
    finally:
        camera.release()
        cv2.destroyAllWindows()

    if cancelled:
        print("enrollment_status: cancelled")
        return 4

    profile = builder.finalize()
    summary = profile.summary()
    print("enrollment_status: profile_ready_in_memory")
    print(f"embedding_dim: {summary['embedding_dim']}")
    print(f"pose_templates: {summary['pose_templates']}")
    print(f"samples_per_pose: {summary['samples_per_pose']}")
    print("raw_images_saved: false")
    print("profile_written: false")
    print("authentication_enabled: false")
    print("status: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
