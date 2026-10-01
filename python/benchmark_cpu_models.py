#!/usr/bin/env python3

"""Camera-free CPU inference benchmark for the pinned YuNet/SFace baseline."""

from __future__ import annotations

import argparse
import json
import platform
import resource
import statistics
import sys
import time
from pathlib import Path
from typing import Callable

import cv2
import numpy as np

from detectors.yunet import YuNetFaceDetector
from recognizers.sface import SFaceRecognizer


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Benchmark CPU face models")
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
    parser.add_argument("--iterations", type=int, default=50)
    parser.add_argument("--warmup", type=int, default=5)
    parser.add_argument("--json", action="store_true")
    return parser.parse_args()


def summarize(values: list[float]) -> dict[str, float]:
    return {
        "mean_ms": statistics.fmean(values),
        "p50_ms": float(np.percentile(values, 50)),
        "p95_ms": float(np.percentile(values, 95)),
        "min_ms": min(values),
        "max_ms": max(values),
    }


def benchmark(
    operation: Callable[[], object],
    warmup: int,
    iterations: int,
) -> dict[str, float]:
    for _ in range(warmup):
        operation()

    times: list[float] = []

    for _ in range(iterations):
        before = time.perf_counter()
        operation()
        times.append((time.perf_counter() - before) * 1000.0)

    return summarize(times)


def main() -> int:
    args = parse_args()

    if args.iterations < 1 or args.warmup < 0:
        print("ERROR: iterations must be positive and warmup must be non-negative")
        return 1

    started = time.perf_counter()
    detector = YuNetFaceDetector(args.yunet_model)
    detector_load_ms = (time.perf_counter() - started) * 1000.0

    started = time.perf_counter()
    recognizer = SFaceRecognizer(args.sface_model)
    recognizer_load_ms = (time.perf_counter() - started) * 1000.0

    detector_input = np.zeros((320, 320, 3), dtype=np.uint8)
    recognizer_input = np.zeros((112, 112, 3), dtype=np.uint8)

    detector_metrics = benchmark(
        lambda: detector.detect(detector_input),
        args.warmup,
        args.iterations,
    )
    recognizer_metrics = benchmark(
        lambda: recognizer.embed_aligned(recognizer_input),
        args.warmup,
        args.iterations,
    )

    output = {
        "format": "face-unlock-cpu-model-benchmark",
        "format_version": 1,
        "runtime": {
            "python": platform.python_version(),
            "opencv": cv2.__version__,
            "machine": platform.machine(),
            "system": platform.system(),
            "opencv_threads": cv2.getNumThreads(),
        },
        "configuration": {
            "backend": "opencv",
            "target": "cpu",
            "iterations": args.iterations,
            "warmup": args.warmup,
            "input_contains_biometric_data": False,
        },
        "model_load": {
            "yunet_ms": detector_load_ms,
            "sface_ms": recognizer_load_ms,
        },
        "inference": {
            "yunet_320x320": detector_metrics,
            "sface_112x112": recognizer_metrics,
        },
        "process": {
            "peak_rss_kib": int(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss),
        },
        "warnings": [
            "synthetic input measures runtime cost, not recognition accuracy",
            "camera, alignment, quality gates, and IPC are not included",
        ],
    }

    if args.json:
        print(json.dumps(output, indent=2))
    else:
        print("cpu_benchmark_status: ok")
        print(f"opencv_version: {output['runtime']['opencv']}")
        print(f"opencv_threads: {output['runtime']['opencv_threads']}")
        print(f"yunet_load_ms: {detector_load_ms:.3f}")
        print(f"sface_load_ms: {recognizer_load_ms:.3f}")
        print(f"yunet_p50_ms: {detector_metrics['p50_ms']:.3f}")
        print(f"yunet_p95_ms: {detector_metrics['p95_ms']:.3f}")
        print(f"sface_p50_ms: {recognizer_metrics['p50_ms']:.3f}")
        print(f"sface_p95_ms: {recognizer_metrics['p95_ms']:.3f}")
        print(f"peak_rss_kib: {output['process']['peak_rss_kib']}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
