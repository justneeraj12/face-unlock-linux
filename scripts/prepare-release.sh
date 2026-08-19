#!/usr/bin/env bash
set -euo pipefail

version="${1:-}"

if [[ -z "$version" ]]; then
  echo "Usage:"
  echo "  ./scripts/prepare-release.sh v0.1.0-alpha"
  exit 1
fi

if [[ ! "$version" =~ ^v[0-9]+\.[0-9]+\.[0-9]+(-[A-Za-z0-9.-]+)?$ ]]; then
  echo "ERROR: version must look like v0.1.0 or v0.1.0-alpha"
  exit 1
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

echo "[prepare-release] Preparing $version"
echo
echo "This script will:"
echo "  - verify the working tree is clean"
echo "  - run local verification"
echo "  - print release tag commands"
echo
echo "This script will NOT:"
echo "  - create a git tag automatically"
echo "  - push a git tag automatically"
echo "  - publish a GitHub release automatically"
echo "  - modify PAM files"
echo

if [[ -n "$(git status --short)" ]]; then
  echo "ERROR: working tree is not clean."
  git status --short
  exit 1
fi

echo "[prepare-release] Current commit:"
git --no-pager log --oneline -1
echo

echo "[prepare-release] Running local verification..."
./scripts/verify-local.sh

echo
echo "[prepare-release] Verification passed."
echo
echo "Next manual steps:"
echo
echo "1. Review release checklist:"
echo "     docs/releases/v0.1.0-alpha.md"
echo
echo "2. Create annotated tag:"
echo "     git tag -a $version -m \"$version\""
echo
echo "3. Push tag:"
echo "     git push origin $version"
echo
echo "4. Watch GitHub Actions:"
echo "     gh run list --limit 5"
echo
echo "5. Create GitHub release manually or with gh:"
echo "     gh release create $version --title \"$version\" --notes-file docs/releases/v0.1.0-alpha.md"
echo
echo "Release preparation complete."
