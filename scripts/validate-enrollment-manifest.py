#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any


def fail(message: str) -> None:
    raise SystemExit(f"manifest_validation_status: failed\nreason: {message}")


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
    if isinstance(value, bool) or not isinstance(value, int):
        fail(f"{context}.{key} must be integer")
    return value


def require_optional_number(
    obj: dict[str, Any], key: str, context: str
) -> float | None:
    value = require_key(obj, key, context)
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        fail(f"{context}.{key} must be a number or null")
    number = float(value)
    if not math.isfinite(number):
        fail(f"{context}.{key} must be finite")
    if number < -1.0 or number > 1.0:
        fail(f"{context}.{key} must be between -1 and 1")
    return number


def require_str(obj: dict[str, Any], key: str, context: str) -> str:
    value = require_key(obj, key, context)
    if not isinstance(value, str):
        fail(f"{context}.{key} must be string")
    return value


def validate_manifest(path: Path) -> None:
    try:
        data = json.loads(path.read_text())
    except Exception as exc:
        fail(f"invalid JSON: {exc}")

    if not isinstance(data, dict):
        fail("manifest root must be object")

    fmt = require_str(data, "format", "root")
    if fmt != "face-unlock-enrollment-manifest":
        fail("root.format mismatch")

    version = require_int(data, "format_version", "root")
    if version != 1:
        fail("root.format_version must be 1")

    require_str(data, "created_at", "root")

    user = require_key(data, "user", "root")
    if not isinstance(user, dict):
        fail("root.user must be object")
    uid = require_int(user, "uid", "user")
    if uid < 0:
        fail("user.uid must be >= 0")
    require_str(user, "username", "user")

    model = require_key(data, "model", "root")
    if not isinstance(model, dict):
        fail("root.model must be object")
    require_str(model, "embedding_model_id", "model")
    embedding_dim = require_int(model, "embedding_dim", "model")
    if embedding_dim < 0:
        fail("model.embedding_dim must be >= 0")

    quality = require_key(data, "quality", "root")
    if not isinstance(quality, dict):
        fail("root.quality must be object")
    training_samples = require_int(
        quality, "training_samples_total", "quality"
    )
    heldout_samples = require_int(
        quality, "heldout_samples_total", "quality"
    )
    if training_samples < 0 or heldout_samples < 0:
        fail("quality sample counts must be >= 0")
    heldout_passed = require_bool(
        quality, "heldout_validation_passed", "quality"
    )
    lowest_heldout = require_optional_number(
        quality, "lowest_heldout_similarity", "quality"
    )
    required_heldout = require_optional_number(
        quality, "required_heldout_similarity", "quality"
    )
    pose_slots = require_key(quality, "pose_slots", "quality")
    if not isinstance(pose_slots, dict):
        fail("quality.pose_slots must be object")
    pose_coverage = [
        require_bool(pose_slots, pose, "quality.pose_slots")
        for pose in ("center", "left", "right", "up", "down")
    ]

    template = require_key(data, "template", "root")
    if not isinstance(template, dict):
        fail("root.template must be object")
    require_str(template, "encrypted_template_path", "template")
    require_str(template, "encryption", "template")
    contains_raw_images = require_bool(template, "contains_raw_images", "template")
    if contains_raw_images:
        fail("template.contains_raw_images must be false")

    privacy = require_key(data, "privacy", "root")
    if not isinstance(privacy, dict):
        fail("root.privacy must be object")
    raw_images_saved = require_bool(privacy, "raw_images_saved", "privacy")
    face_crops_saved = require_bool(privacy, "face_crops_saved", "privacy")
    telemetry_enabled = require_bool(privacy, "telemetry_enabled", "privacy")

    if raw_images_saved:
        fail("privacy.raw_images_saved must be false for current prototype")

    if face_crops_saved:
        fail("privacy.face_crops_saved must be false for current prototype")

    if telemetry_enabled:
        fail("privacy.telemetry_enabled must be false")

    status = require_key(data, "status", "root")
    if not isinstance(status, dict):
        fail("root.status must be object")

    enrollment_complete = require_bool(status, "enrollment_complete", "status")
    real_biometric_template = require_bool(status, "real_biometric_template", "status")
    placeholder_only = require_bool(status, "placeholder_only", "status")

    if enrollment_complete != real_biometric_template:
        fail("real biometric and enrollment completion flags must agree")

    if placeholder_only:
        if enrollment_complete:
            fail("placeholder_only manifest cannot have enrollment_complete true")
        if real_biometric_template:
            fail("placeholder_only manifest cannot have real_biometric_template true")

    if enrollment_complete or real_biometric_template:
        if placeholder_only:
            fail("real enrollment cannot be placeholder_only")
        if training_samples < 5:
            fail("real enrollment requires at least five training samples")
        if heldout_samples < 5 or not heldout_passed:
            fail("real enrollment requires held-out validation")
        if lowest_heldout is None or required_heldout is None:
            fail("real enrollment requires held-out similarity evidence")
        if lowest_heldout < required_heldout:
            fail("real enrollment held-out similarity is below requirement")
        if not all(pose_coverage):
            fail("real enrollment requires all pose slots")
    elif heldout_passed:
        fail("incomplete enrollment cannot claim held-out validation")

    print(f"manifest_path: {path}")
    print("manifest_validation_status: ok")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    args = parser.parse_args()

    validate_manifest(args.manifest)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
