#!/usr/bin/env bash
set -euo pipefail

echo "[check-json] Checking JSON files"

python3 - <<'PY'
import json
from pathlib import Path

bad = False

for path in sorted(Path(".").rglob("*.json")):
    if ".git" in path.parts:
        continue
    if "build" in path.parts:
        continue
    if ".venv" in path.parts:
        continue

    try:
        json.loads(path.read_text())
    except Exception as exc:
        bad = True
        print(f"[BAD] {path}: {exc}")
    else:
        print(f"[OK] {path}")

if bad:
    raise SystemExit(1)

print("[check-json] OK")
PY
