#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "Usage:"
  echo "  ./scripts/prepare-release.sh v0.1.1-alpha"
  echo
  echo "The script expects release notes at:"
  echo "  docs/releases/VERSION.md"
  echo
  echo "Example:"
  echo "  docs/releases/v0.1.1-alpha.md"
}

version="${1:-}"

if [[ -z "$version" || "$version" == "--help" || "$version" == "-h" ]]; then
  usage
  exit 0
fi

if [[ ! "$version" =~ ^v[0-9]+\.[0-9]+\.[0-9]+(-[A-Za-z0-9.-]+)?$ ]]; then
  echo "ERROR: version must look like v0.1.0 or v0.1.0-alpha"
  exit 1
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

release_notes="docs/releases/${version}.md"

if [[ ! -f "$release_notes" ]]; then
  echo "ERROR: release notes not found:"
  echo "  $release_notes"
  exit 1
fi

echo "[prepare-release] Preparing $version"
echo
echo "Release notes:"
echo "  $release_notes"
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
echo "1. Review release notes:"
echo "     $release_notes"
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
echo "     gh release create \"$version\" --title \"$version\" --notes-file \"$release_notes\" --prerelease"
echo
echo "Release preparation complete."
