#!/usr/bin/env bash
set -euo pipefail

echo "[audit-dependencies] Dependency audit"
echo

fail() {
  echo
  echo "[audit-dependencies] FAILED: $*"
  exit 1
}

show_ldd() {
  local path="$1"

  if [[ ! -e "$path" ]]; then
    echo "missing: $path"
    return
  fi

  echo
  echo "ldd: $path"
  echo "------------------------------------------------------------"
  ldd "$path"
  echo "------------------------------------------------------------"
}

check_no_disallowed_pam_deps() {
  local path="$1"

  if [[ ! -f "$path" ]]; then
    fail "PAM module not found: $path"
  fi

  echo
  echo "Checking PAM module for disallowed dependencies..."

  local deps
  deps="$(ldd "$path")"

  echo "$deps"

  if echo "$deps" | grep -Ei 'opencv|torch|cuda|cudart|Qt|tensorflow|sodium'; then
    fail "PAM module links to disallowed heavy dependency"
  fi

  echo "pam_dependency_status: ok"
}

pam_module="build/pam/pam_face_unlock.so"
daemon_bin="build/daemon/face-unlockd"
crypto_selftest="build/daemon/face-unlock-crypto-selftest"
template_tool="build/daemon/face-unlock-template-tool"
gui_bin="build-gui/gui/face-unlock-enroll"

check_no_disallowed_pam_deps "$pam_module"

show_ldd "$daemon_bin"
show_ldd "$crypto_selftest"
show_ldd "$template_tool"

if [[ -x "$gui_bin" ]]; then
  show_ldd "$gui_bin"
else
  echo
  echo "optional_gui_status: not_built"
fi

echo
echo "Package metadata if present:"
mapfile -t debs < <(find build -maxdepth 1 -type f -name "*.deb" | sort 2>/dev/null || true)

if [[ "${#debs[@]}" -eq 0 ]]; then
  echo "package_status: no_deb_found"
else
  for deb in "${debs[@]}"; do
    echo
    echo "Package: $deb"
    dpkg-deb -I "$deb"
  done
fi

echo
echo "dependency_audit_status: ok"
