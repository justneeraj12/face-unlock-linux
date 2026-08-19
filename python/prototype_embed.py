#!/usr/bin/env python3

"""
Prototype embedding generator for saved face crops.

This is not a real face recognition model.

It uses a tiny randomly initialized PyTorch model to exercise:
- image loading
- preprocessing
- embedding-shaped output
- optional embedding metadata writing

Embeddings are derived biometric data.
Saving embeddings requires --i-understand-biometric-risk.
"""

from __future__ import annotations

import argparse
import json
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
    parser = argparse.ArgumentParser(
        description="Prototype embedding generator for saved face crops"
    )

    parser.add_argument(
        "--input-dir",
        type=Path,
        default=Path("enrollment_samples/crops"),
        help="Directory containing face crop images",
    )

    parser.add_argument(
        "--output",
        type=Path,
        default=Path("enrollment_samples/embeddings.json"),
        help="Output JSON path",
    )

    parser.add_argument(
        "--embedding-dim",
        type=int,
        default=128,
        help="Embedding dimension",
    )

    parser.add_argument(
        "--write-output",
        action="store_true",
        help="Write embeddings JSON. Requires --i-understand-biometric-risk",
    )

    parser.add_argument(
        "--i-understand-biometric-risk",
        action="store_true",
        help="Required to write embeddings because embeddings are biometric data",
    )

    return parser.parse_args()


def load_image(path: Path) -> torch.Tensor:
    image = cv2.imread(str(path), cv2.IMREAD_COLOR)

    if image is None:
        raise RuntimeError(f"failed to read image: {path}")

    image = cv2.resize(image, (112, 112))
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    array = image.astype(np.float32) / 255.0
    tensor = torch.from_numpy(array).permute(2, 0, 1).unsqueeze(0)

    return tensor


def list_images(input_dir: Path) -> list[Path]:
    if not input_dir.exists():
        raise RuntimeError(f"input directory not found: {input_dir}")

    images: list[Path] = []

    for pattern in ("*.jpg", "*.jpeg", "*.png"):
        images.extend(sorted(input_dir.glob(pattern)))

    return sorted(images)


def main() -> int:
    args = parse_args()

    if args.write_output and not args.i_understand_biometric_risk:
        print(
            "ERROR: --write-output requires --i-understand-biometric-risk\n"
            "Embeddings are derived biometric data. Do not commit them to Git."
        )
        return 1

    images = list_images(args.input_dir)

    print("prototype_embed_status: started")
    print(f"input_dir: {args.input_dir}")
    print(f"images_found: {len(images)}")
    print(f"write_output: {str(args.write_output).lower()}")

    if not images:
        print("status: no_images")
        return 1

    torch.manual_seed(0)

    model = EmbeddingStub(embedding_dim=args.embedding_dim)
    model.eval()

    results: list[dict[str, Any]] = []

    with torch.no_grad():
        for image_path in images:
            tensor = load_image(image_path)
            embedding = model(tensor)[0].cpu().numpy().astype(float)

            norm = float(np.linalg.norm(embedding))

            print(
                "embedding_report:"
                f" file={image_path}"
                f" dim={len(embedding)}"
                f" norm={norm:.6f}"
            )

            results.append(
                {
                    "file": str(image_path),
                    "embedding_dim": int(len(embedding)),
                    "embedding_norm": norm,
                    "embedding": embedding.tolist(),
                    "warning": "prototype random-model embedding; not real face recognition",
                }
            )

    if args.write_output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(results, indent=2))
        print(f"output_written: {args.output}")
        print("privacy_warning: embeddings are derived biometric data; do not commit them")

    print("prototype_embed_status: stopped")
    print("status: ok")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
