#!/usr/bin/env python3

"""
Export a tiny TorchScript embedding model stub.

This is not a real face recognition model.

It exists to test:
- Python model export flow
- TorchScript artifact generation
- future C++ TorchScript loading path

Generated model files are ignored by Git.
"""

from __future__ import annotations

import argparse
from pathlib import Path

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
        description="Export TorchScript embedding stub"
    )

    parser.add_argument(
        "--output",
        type=Path,
        default=Path("models/embedding_stub.pt"),
        help="Output TorchScript path",
    )

    parser.add_argument(
        "--embedding-dim",
        type=int,
        default=128,
        help="Embedding dimension",
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()

    args.output.parent.mkdir(parents=True, exist_ok=True)

    model = EmbeddingStub(embedding_dim=args.embedding_dim)
    model.eval()

    example = torch.zeros(1, 3, 112, 112)

    with torch.no_grad():
        traced = torch.jit.trace(model, example)

    traced.save(str(args.output))

    print("export_status: ok")
    print(f"torch_version: {torch.__version__}")
    print(f"output: {args.output}")
    print(f"embedding_dim: {args.embedding_dim}")

    loaded = torch.jit.load(str(args.output))
    loaded.eval()

    with torch.no_grad():
        out = loaded(example)

    print(f"verify_output_shape: {tuple(out.shape)}")

    if tuple(out.shape) != (1, args.embedding_dim):
        print("verify_status: failed")
        return 1

    print("verify_status: ok")
    print("status: ok")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
