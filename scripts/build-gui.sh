#!/usr/bin/env bash
set -euo pipefail

build_dir="${GUI_BUILD_DIR:-build-gui}"
build_type="${CMAKE_BUILD_TYPE:-RelWithDebInfo}"

cmake_args=(
  -S .
  -B "$build_dir"
  -DBUILD_GUI=ON
  -DCMAKE_BUILD_TYPE="$build_type"
)

if command -v ninja >/dev/null 2>&1; then
  cmake_args+=(-G Ninja)
fi

if command -v ccache >/dev/null 2>&1 && [[ "${FACE_UNLOCK_DISABLE_CCACHE:-0}" != "1" ]]; then
  cmake_args+=(
    -DCMAKE_C_COMPILER_LAUNCHER=ccache
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
  )
fi

echo "[build-gui] Configuring:"
printf '  %q' cmake "${cmake_args[@]}"
echo

cmake "${cmake_args[@]}"

echo "[build-gui] Building:"
cmake --build "$build_dir" --parallel

echo
echo "[build-gui] Done."
echo "GUI binary:"
echo "  $build_dir/gui/face-unlock-enroll"
echo
echo "Run manually with:"
echo "  ./$build_dir/gui/face-unlock-enroll"
