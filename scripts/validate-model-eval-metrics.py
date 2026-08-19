#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def fail(message: str) -> None:
    raise SystemExit(f"metrics_validation_status: failed\nreason: {message}")


def require_key(obj: dict[str, Any], key: str, context: str) -> Any:
    if key not in obj:
        fail(f"missing key {context}.{key}")
    return obj[key]


def require_bool(obj: dict[str, Any], key: str, context: str) -> bool:
    value = require_key(obj, key, context)
    if not isinstance(value, bool):
        fail(f"{context}.{key} must be boolean")
    return value


def require_int(obj: dict[str, Any], key: str, context: str) -> int:
    value = require_key(obj, key, context)
    if not isinstance(value, int):
        fail(f"{context}.{key} must be integer")
    return value


def require_number(obj: dict[str, Any], key: str, context: str) -> int | float | None:
    value = require_key(obj, key, context)
    if value is not None and not isinstance(value, (int, float)):
        fail(f"{context}.{key} must be number or null")
    return value


def require_str(obj: dict[str, Any], key: str, context: str) -> str:
    value = require_key(obj, key, context)
    if not isinstance(value, str):
        fail(f"{context}.{key} must be string")
    return value


def require_object(obj: dict[str, Any], key: str, context: str) -> dict[str, Any]:
    value = require_key(obj, key, context)
    if not isinstance(value, dict):
        fail(f"{context}.{key} must be object")
    return value


def validate_summary(obj: dict[str, Any], context: str) -> None:
    count = require_int(obj, "count", context)
    if count < 0:
        fail(f"{context}.count must be >= 0")

    for key in ("min", "max", "mean", "median"):
        require_number(obj, key, context)


def validate_metrics(path: Path) -> None:
    try:
        data = json.loads(path.read_text())
    except Exception as exc:
        fail(f"invalid JSON: {exc}")

    if not isinstance(data, dict):
        fail("metrics root must be object")

    fmt = require_str(data, "format", "root")
    if fmt != "face-unlock-model-eval-metrics":
        fail("root.format mismatch")

    version = require_int(data, "format_version", "root")
    if version != 1:
        fail("root.format_version must be 1")

    require_str(data, "created_at", "root")

    model = require_object(data, "model", "root")
    require_str(model, "model_id", "model")
    require_str(model, "model_type", "model")
    require_bool(model, "real_face_recognition", "model")
    embedding_dim = require_int(model, "embedding_dim", "model")
    if embedding_dim < 0:
        fail("model.embedding_dim must be >= 0")

    dataset = require_object(data, "dataset", "root")
    require_str(dataset, "source", "dataset")
    images_count = require_int(dataset, "images_count", "dataset")
    if images_count < 0:
        fail("dataset.images_count must be >= 0")
    require_bool(dataset, "contains_biometric_data", "dataset")
    committed_to_git = require_bool(dataset, "committed_to_git", "dataset")
    if committed_to_git:
        fail("dataset.committed_to_git must be false")

    performance = require_object(data, "performance", "root")
    require_number(performance, "total_ms", "performance")
    inference = require_object(performance, "inference_ms", "performance")
    validate_summary(inference, "performance.inference_ms")

    similarity = require_object(data, "similarity", "root")
    require_str(similarity, "metric", "similarity")
    pairwise = require_object(similarity, "pairwise", "similarity")
    validate_summary(pairwise, "similarity.pairwise")

    privacy = require_object(data, "privacy", "root")
    raw_images_included = require_bool(privacy, "raw_images_included", "privacy")
    embeddings_included = require_bool(privacy, "embeddings_included", "privacy")
    safe_to_commit = require_bool(privacy, "safe_to_commit", "privacy")

    if raw_images_included:
        fail("privacy.raw_images_included must be false")

    if embeddings_included:
        fail("privacy.embeddings_included must be false")

    if safe_to_commit:
        fail("privacy.safe_to_commit must be false for current prototype metrics")

    print(f"metrics_path: {path}")
    print("metrics_validation_status: ok")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("metrics", type=Path)
    args = parser.parse_args()

    validate_metrics(args.metrics)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
