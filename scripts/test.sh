#!/usr/bin/env bash
set -euo pipefail

if [[ ! -d build ]]; then
  echo "Build directory not found. Run:"
  echo "  ./scripts/build.sh"
  exit 1
fi

ctest --test-dir build --output-on-failure
