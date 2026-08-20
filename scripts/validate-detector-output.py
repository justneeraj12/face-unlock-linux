#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def fail(message: str) -> None:
    raise SystemExit(f"detector_output_validation_status: failed\nreason: {message}")


def require_key(obj: dict[str, Any], key: str, context: str) -> Any:
    if key not in obj:
        fail(f"missing key {context}.{key}")
    return obj[key]


def require_str(obj: dict[str, Any], key: str, context: str) -> str:
    value = require_key(obj, key, context)
    if not isinstance(value, str):
        fail(f"{context}.{key} must be string")
    return value


def require_int(obj: dict[str, Any], key: str, context: str) -> int:
    value = require_key(obj, key, context)
    if not isinstance(value, int):
        fail(f"{context}.{key} must be integer")
    return value


def require_bool(obj: dict[str, Any], key: str, context: str) -> bool:
    value = require_key(obj, key, context)
    if not isinstance(value, bool):
        fail(f"{context}.{key} must be boolean")
    return value


def require_object(obj: dict[str, Any], key: str, context: str) -> dict[str, Any]:
    value = require_key(obj, key, context)
    if not isinstance(value, dict):
        fail(f"{context}.{key} must be object")
    return value


def validate(path: Path) -> None:
    try:
        data = json.loads(path.read_text())
    except Exception as exc:
        fail(f"invalid JSON: {exc}")

    if not isinstance(data, dict):
        fail("root must be object")

    if require_str(data, "format", "root") != "face-unlock-detector-output":
        fail("root.format mismatch")

    if require_int(data, "format_version", "root") != 1:
        fail("root.format_version must be 1")

    require_str(data, "created_at", "root")
    require_str(data, "backend", "root")

    source = require_object(data, "source", "root")
    require_str(source, "type", "source")

    summary = require_object(data, "summary", "root")
    frames_total = require_int(summary, "frames_total", "summary")
    detections_total = require_int(summary, "detections_total", "summary")

    if frames_total < 0:
        fail("summary.frames_total must be >= 0")

    if detections_total < 0:
        fail("summary.detections_total must be >= 0")

    privacy = require_object(data, "privacy", "root")
    raw_images = require_bool(privacy, "raw_images_included", "privacy")
    crops = require_bool(privacy, "face_crops_included", "privacy")
    safe = require_bool(privacy, "safe_to_commit", "privacy")

    if raw_images:
        fail("privacy.raw_images_included must be false")

    if crops:
        fail("privacy.face_crops_included must be false")

    if safe:
        fail("privacy.safe_to_commit must be false for generated detector output")

    print(f"detector_output_path: {path}")
    print("detector_output_validation_status: ok")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=Path)
    args = parser.parse_args()

    validate(args.path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
