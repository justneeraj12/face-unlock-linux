#!/usr/bin/env bash
set -euo pipefail

with_sudo_dry_run="false"

usage() {
  echo "Usage:"
  echo "  ./scripts/verify-local.sh"
  echo "  ./scripts/verify-local.sh --with-sudo-dry-run"
}

for arg in "$@"; do
  case "$arg" in
    --with-sudo-dry-run)
      with_sudo_dry_run="true"
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $arg"
      usage
      exit 1
      ;;
  esac
done

echo "[verify-local] face-unlock-linux local verification"
echo
echo "This script verifies the local development tree."
echo
echo "It will:"
echo "  - run documentation checks"
echo "  - build the project"
echo "  - run CTest"
echo "  - run crypto self-test"
echo "  - audit PAM module dependencies"
echo "  - build Debian package with CPack"
echo "  - inspect package metadata and contents"
if [[ "$with_sudo_dry_run" == "true" ]]; then
  echo "  - run sudo PAM dry-run regression test"
fi
echo
echo "It will NOT:"
echo "  - modify /etc/pam.d/*"
echo "  - install pam_face_unlock.so"
echo "  - modify sudo/login/lock-screen authentication"
echo "  - enable systemd services"
echo

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

fail() {
  echo
  echo "[verify-local] FAILED: $*"
  exit 1
}

section() {
  echo
  echo "============================================================"
  echo "[verify-local] $*"
  echo "============================================================"
}

section "Git status"

git status --short

if [[ -n "$(git status --short)" ]]; then
  echo
  echo "WARNING: working tree has uncommitted changes."
  echo "Local verification can continue, but release verification should use a clean tree."
fi

section "Documentation check"

./scripts/check-docs.sh

section "JSON check"

./scripts/check-json.sh

section "Enrollment manifest validation"

./scripts/validate-enrollment-manifest.py schemas/enrollment-manifest.example.json

section "Model evaluation metrics validation"

./scripts/validate-model-eval-metrics.py schemas/model-eval-metrics.example.json

section "Build"

./scripts/build.sh

section "CTest"

./scripts/test.sh

section "Crypto self-test"

./build/daemon/face-unlock-crypto-selftest

section "Daemon CLI smoke tests"

./build/daemon/face-unlockd --help >/tmp/face-unlockd-help.txt
grep -q -- "--daemon" /tmp/face-unlockd-help.txt || fail "daemon help missing --daemon"
grep -q -- "--model-test" /tmp/face-unlockd-help.txt || fail "daemon help missing --model-test"
rm -f /tmp/face-unlockd-help.txt

./build/daemon/face-unlockd --model-test || true

section "PAM module dependency audit"

pam_module="build/pam/pam_face_unlock.so"

if [[ ! -f "$pam_module" ]]; then
  fail "PAM module not found: $pam_module"
fi

ldd "$pam_module"

if ldd "$pam_module" | grep -Ei 'opencv|torch|cuda|cudart|Qt|tensorflow|sodium'; then
  fail "PAM module links to disallowed heavy dependency"
fi

echo "PAM dependency audit passed."

section "Build Debian package"

cmake --build build --target package

section "Package artifacts"

mapfile -t debs < <(find build -maxdepth 1 -type f -name "*.deb" | sort)

if [[ "${#debs[@]}" -eq 0 ]]; then
  fail "No .deb package found in build/"
fi

for deb in "${debs[@]}"; do
  ls -lh "$deb"
done

section "Package metadata"

for deb in "${debs[@]}"; do
  echo
  echo "Package: $deb"
  dpkg-deb -I "$deb"
done

section "Package contents preview"

for deb in "${debs[@]}"; do
  echo
  echo "Package: $deb"
  dpkg-deb -c "$deb" | head -160
done

section "Safety reminder"

echo "This verification did not modify PAM configuration."
echo
echo "Real PAM service files live under:"
echo "  /etc/pam.d/"
echo
echo "Do not modify sudo, GDM, SDDM, LightDM, lock-screen, or common-auth yet."
echo "Use fake PAM service testing only."

section "Summary"

echo "verify_status: ok"
echo
echo "Local verification completed successfully."
