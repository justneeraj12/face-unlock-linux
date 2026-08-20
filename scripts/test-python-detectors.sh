#!/usr/bin/env bash
set -euo pipefail

echo "[test-python-detectors] Python detector smoke test"

python3 - <<'PY'
import sys
from pathlib import Path

repo = Path.cwd()
sys.path.insert(0, str(repo / "python"))

from detectors.factory import create_detector

detector = create_detector("noop")
assert detector.backend_name == "noop"

detections = detector.detect(__import__("numpy").zeros((32, 32, 3), dtype="uint8"))
assert detections == []

print("noop_detector_status: ok")

try:
    create_detector("does-not-exist")
except ValueError:
    print("unknown_backend_status: ok")
else:
    raise SystemExit("unknown backend did not raise ValueError")
PY

python3 python/prototype_detect.py --help >/tmp/face-unlock-prototype-detect-help.txt

grep -q -- "--backend" /tmp/face-unlock-prototype-detect-help.txt
grep -q -- "noop" /tmp/face-unlock-prototype-detect-help.txt
grep -q -- "yunet" /tmp/face-unlock-prototype-detect-help.txt

rm -f /tmp/face-unlock-prototype-detect-help.txt

echo "prototype_detect_help_status: ok"
echo "status: ok"
