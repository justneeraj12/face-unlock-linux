#!/usr/bin/env bash
set -euo pipefail

echo "[test-detector-output-generation] Testing detector output generation without camera"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

output="$tmp_dir/detections.json"

python3 python/prototype_detect.py \
  --backend noop \
  --synthetic-frame 640x480 \
  --write-metadata \
  --i-understand-biometric-risk \
  --output "$output"

test -f "$output"

scripts/validate-detector-output.py "$output"

python3 - <<PY
import json
from pathlib import Path

path = Path("$output")
data = json.loads(path.read_text())

assert data["format"] == "face-unlock-detector-output"
assert data["source"]["type"] == "synthetic"
assert data["summary"]["frames_total"] == 1
assert data["summary"]["detections_total"] == 0
assert data["privacy"]["safe_to_commit"] is False

print("detector_generated_output_status: ok")
PY

echo "status: ok"
