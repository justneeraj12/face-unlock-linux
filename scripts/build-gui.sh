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

if [[ -f "$build_dir/CMakeCache.txt" ]]; then
  if grep -q '^CMAKE_GENERATOR:INTERNAL=' "$build_dir/CMakeCache.txt"; then
    existing_generator="$(grep '^CMAKE_GENERATOR:INTERNAL=' "$build_dir/CMakeCache.txt" | cut -d= -f2-)"
    requested_generator="Unix Makefiles"

    for ((i = 0; i < ${#cmake_args[@]}; ++i)); do
      if [[ "${cmake_args[$i]}" == "-G" && $((i + 1)) -lt ${#cmake_args[@]} ]]; then
        requested_generator="${cmake_args[$((i + 1))]}"
      fi
    done

    if [[ "$existing_generator" != "$requested_generator" ]]; then
      echo "ERROR: existing GUI build directory uses generator:"
      echo "  $existing_generator"
      echo "but this build wants:"
      echo "  $requested_generator"
      echo
      echo "Fix with:"
      echo "  rm -rf $build_dir"
      echo "  ./scripts/build-gui.sh"
      echo
      echo "Or use a different build directory:"
      echo "  GUI_BUILD_DIR=build-gui-ninja ./scripts/build-gui.sh"
      exit 1
    fi
  fi
fi

if [[ -f "$build_dir/CMakeCache.txt" ]]; then
  if grep -q '^CMAKE_GENERATOR:INTERNAL=' "$build_dir/CMakeCache.txt"; then
    existing_generator="$(grep '^CMAKE_GENERATOR:INTERNAL=' "$build_dir/CMakeCache.txt" | cut -d= -f2-)"
    requested_generator="Unix Makefiles"

    for ((i = 0; i < ${#cmake_args[@]}; ++i)); do
      if [[ "${cmake_args[$i]}" == "-G" && $((i + 1)) -lt ${#cmake_args[@]} ]]; then
        requested_generator="${cmake_args[$((i + 1))]}"
      fi
    done

    if [[ "$existing_generator" != "$requested_generator" ]]; then
      echo "ERROR: existing GUI build directory uses generator:"
      echo "  $existing_generator"
      echo "but this build wants:"
      echo "  $requested_generator"
      echo
      echo "Fix with:"
      echo "  rm -rf $build_dir"
      echo "  ./scripts/build-gui.sh"
      echo
      echo "Or use a different build directory:"
      echo "  GUI_BUILD_DIR=build-gui-ninja ./scripts/build-gui.sh"
      exit 1
    fi
  fi
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
