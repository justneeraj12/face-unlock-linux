#!/usr/bin/env bash
set -euo pipefail

multiarch="${FACE_UNLOCK_MULTIARCH:-}"
if [[ -z "$multiarch" ]] && command -v dpkg-architecture >/dev/null 2>&1; then
  multiarch="$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || true)"
fi
if [[ -z "$multiarch" ]] && command -v gcc >/dev/null 2>&1; then
  multiarch="$(gcc -print-multiarch 2>/dev/null || true)"
fi
if [[ -z "$multiarch" ]]; then
  echo "ERROR: unable to determine Debian multiarch directory" >&2
  exit 1
fi

printf '/usr/lib/%s/security/pam_face_unlock.so\n' "$multiarch"
