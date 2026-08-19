#!/usr/bin/env bash
set -euo pipefail

echo "[check-docs] Checking for accidental terminal paste artifacts..."

if grep -RInE 'heredoc>|zsh: command not found|╭─|╰─|▒▓|░|Open WebUI|command not found' \
  --include='*.md' .; then
  echo
  echo "[check-docs] Found suspicious terminal/UI artifacts in Markdown files."
  exit 1
fi

echo "[check-docs] Checking Markdown code fence balance..."

python3 - <<'PY'
from pathlib import Path

bad = False

for path in sorted(Path(".").rglob("*.md")):
    if ".git" in path.parts:
        continue

    lines = path.read_text(errors="replace").splitlines()
    fence_count = 0
    fence_lines = []

    for i, line in enumerate(lines, start=1):
        if line.strip().startswith("```"):
            fence_count += 1
            fence_lines.append(i)

    if fence_count % 2 != 0:
        bad = True
        print(f"[BAD] {path}: odd number of triple-backtick fences: {fence_count}")
        print(f"      fence lines: {fence_lines}")

if bad:
    raise SystemExit(1)

print("[OK] All Markdown files have balanced triple-backtick fences.")
PY

echo "[check-docs] OK"
