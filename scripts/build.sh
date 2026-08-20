#!/usr/bin/env bash
set -euo pipefail

build_dir="${BUILD_DIR:-build}"
build_type="${CMAKE_BUILD_TYPE:-RelWithDebInfo}"

cmake_args=(
  -S .
  -B "$build_dir"
  -DCMAKE_BUILD_TYPE="$build_type"
)

# Prefer Ninja when available for faster local and CI builds.
if command -v ninja >/dev/null 2>&1; then
  cmake_args+=(-G Ninja)
fi

# Use ccache when available unless explicitly disabled.
if command -v ccache >/dev/null 2>&1 && [[ "${FACE_UNLOCK_DISABLE_CCACHE:-0}" != "1" ]]; then
  cmake_args+=(
    -DCMAKE_C_COMPILER_LAUNCHER=ccache
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
  )
fi

echo "[build] Configuring:"
printf '  %q' cmake "${cmake_args[@]}"
echo

cmake "${cmake_args[@]}"

echo "[build] Building:"
cmake --build "$build_dir" --parallel

echo
echo "[build] Done."
echo "Build directory: $build_dir"
