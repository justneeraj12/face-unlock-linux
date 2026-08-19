#!/usr/bin/env python3

"""
Model evaluation harness scaffold.

This script uses the current random EmbeddingStub to define evaluation flow.

It is not real model evaluation yet.

Default behavior:
- reads local image crops
- computes random-stub embeddings
- prints pairwise cosine similarity stats
- saves nothing

Writing metrics requires:
- --write-metrics
- --i-understand-biometric-risk
"""

from __future__ import annotations

import argparse
import itertools
import json
import statistics
import time
from pathlib import Path
from typing import Any

import cv2
import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F


class EmbeddingStub(nn.Module):
    def __init__(self, embedding_dim: int = 128) -> None:
        super().__init__()
        self.features = nn.Sequential(
            nn.Conv2d(3, 16, kernel_size=3, stride=2, padding=1),
            nn.ReLU(inplace=True),
            nn.Conv2d(16, 32, kernel_size=3, stride=2, padding=1),
            nn.ReLU(inplace=True),
            nn.Conv2d(32, 64, kernel_size=3, stride=2, padding=1),
            nn.ReLU(inplace=True),
            nn.AdaptiveAvgPool2d((1, 1)),
            nn.Flatten(),
        )
        self.embedding = nn.Linear(64, embedding_dim)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = self.features(x)
        x = self.embedding(x)
        return F.normalize(x, p=2, dim=1)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Face model evaluation harness scaffold")

    parser.add_argument(
        "--input-dir",
        type=Path,
        default=Path("enrollment_samples/crops"),
        help="Directory containing local face crop images",
    )

    parser.add_argument(
        "--output",
        type=Path,
        default=Path("enrollment_samples/model_eval_metrics.json"),
        help="Metrics output path when --write-metrics is used",
    )

    parser.add_argument(
        "--embedding-dim",
        type=int,
        default=128,
        help="Embedding dimension",
    )

    parser.add_argument(
        "--write-metrics",
        action="store_true",
        help="Write metrics JSON. Requires --i-understand-biometric-risk",
    )

    parser.add_argument(
        "--i-understand-biometric-risk",
        action="store_true",
        help="Required to write metrics derived from biometric images",
    )

    return parser.parse_args()


def list_images(input_dir: Path) -> list[Path]:
    images: list[Path] = []

    if not input_dir.exists():
        raise RuntimeError(f"input directory not found: {input_dir}")

    for pattern in ("*.jpg", "*.jpeg", "*.png"):
        images.extend(sorted(input_dir.glob(pattern)))

    return sorted(images)


def load_image(path: Path) -> torch.Tensor:
    image = cv2.imread(str(path), cv2.IMREAD_COLOR)

    if image is None:
        raise RuntimeError(f"failed to read image: {path}")

    image = cv2.resize(image, (112, 112))
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    array = image.astype(np.float32) / 255.0

    return torch.from_numpy(array).permute(2, 0, 1).unsqueeze(0)


def cosine_similarity(a: np.ndarray, b: np.ndarray) -> float:
    denom = float(np.linalg.norm(a) * np.linalg.norm(b))

    if denom == 0.0:
        return 0.0

    return float(np.dot(a, b) / denom)


def summarize(values: list[float]) -> dict[str, Any]:
    if not values:
        return {
            "count": 0,
            "min": None,
            "max": None,
            "mean": None,
            "median": None,
        }

    return {
        "count": len(values),
        "min": min(values),
        "max": max(values),
        "mean": statistics.fmean(values),
        "median": statistics.median(values),
    }


def main() -> int:
    args = parse_args()

    if args.write_metrics and not args.i_understand_biometric_risk:
        print(
            "ERROR: --write-metrics requires --i-understand-biometric-risk\n"
            "Metrics are derived from biometric images. Do not commit them to Git."
        )
        return 1

    images = list_images(args.input_dir)

    print("model_eval_status: started")
    print("model_id: random-embedding-stub")
    print(f"input_dir: {args.input_dir}")
    print(f"images_found: {len(images)}")
    print(f"write_metrics: {str(args.write_metrics).lower()}")

    if len(images) < 2:
        print("status: not_enough_images")
        return 1

    torch.manual_seed(0)
    model = EmbeddingStub(args.embedding_dim)
    model.eval()

    embeddings: list[dict[str, Any]] = []

    start = time.perf_counter()

    with torch.no_grad():
        for image in images:
            tensor = load_image(image)
            before = time.perf_counter()
            embedding = model(tensor)[0].cpu().numpy().astype(np.float32)
            elapsed_ms = (time.perf_counter() - before) * 1000.0

            embeddings.append(
                {
                    "file": str(image),
                    "embedding": embedding,
                    "inference_ms": elapsed_ms,
                }
            )

    total_ms = (time.perf_counter() - start) * 1000.0

    similarities: list[float] = []

    for left, right in itertools.combinations(embeddings, 2):
        similarities.append(
            cosine_similarity(left["embedding"], right["embedding"])
        )

    inference_times = [float(item["inference_ms"]) for item in embeddings]

    metrics: dict[str, Any] = {
        "model_id": "random-embedding-stub",
        "warning": "not real face recognition",
        "images_count": len(images),
        "embedding_dim": args.embedding_dim,
        "total_ms": total_ms,
        "inference_ms": summarize(inference_times),
        "pairwise_cosine_similarity": summarize(similarities),
        "created_from": str(args.input_dir),
    }

    print(f"images_count: {metrics['images_count']}")
    print(f"embedding_dim: {metrics['embedding_dim']}")
    print(f"total_ms: {metrics['total_ms']:.3f}")
    print(f"inference_ms_mean: {metrics['inference_ms']['mean']:.3f}")
    print(f"pairwise_count: {metrics['pairwise_cosine_similarity']['count']}")
    print(f"pairwise_mean: {metrics['pairwise_cosine_similarity']['mean']:.6f}")

    if args.write_metrics:
        args.output.parent.mkdir(parents=True, exist_ok=True)

        serializable = dict(metrics)
        args.output.write_text(json.dumps(serializable, indent=2))

        print(f"metrics_written: {args.output}")
        print("privacy_warning: metrics are derived from biometric data; do not commit them")

    print("model_eval_status: stopped")
    print("status: ok")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
