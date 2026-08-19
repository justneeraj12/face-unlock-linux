#!/usr/bin/env bash
set -euo pipefail

repo="${1:-justneeraj12/face-unlock-linux}"

echo "[bootstrap-labels] Bootstrapping labels for $repo"
echo
echo "This script uses GitHub CLI."
echo
echo "It will create or update labels on GitHub."
echo "It will NOT modify local system authentication or PAM files."
echo

if ! command -v gh >/dev/null 2>&1; then
  echo "ERROR: gh CLI not found."
  echo "Install with:"
  echo "  sudo apt install gh"
  exit 1
fi

if ! gh auth status >/dev/null 2>&1; then
  echo "ERROR: gh is not authenticated."
  echo "Run:"
  echo "  gh auth login"
  exit 1
fi

upsert_label() {
  local name="$1"
  local color="$2"
  local description="$3"

  if gh label list --repo "$repo" --limit 200 --json name --jq '.[].name' | grep -Fxq "$name"; then
    echo "Updating label: $name"
    gh label edit "$name" --repo "$repo" --color "$color" --description "$description"
  else
    echo "Creating label: $name"
    gh label create "$name" --repo "$repo" --color "$color" --description "$description"
  fi
}

upsert_label "area/daemon" "1f77b4" "C++ user daemon, IPC, camera worker"
upsert_label "area/pam" "d62728" "PAM module and authentication bridge"
upsert_label "area/gui" "9467bd" "Qt enrollment GUI"
upsert_label "area/python" "2ca02c" "Python prototypes and tooling"
upsert_label "area/models" "8c564b" "Model export, evaluation, candidates"
upsert_label "area/packaging" "7f7f7f" "Debian packaging, systemd, installers"
upsert_label "area/docs" "17becf" "Documentation"
upsert_label "area/ci" "bcbd22" "GitHub Actions and CI"
upsert_label "security" "b60205" "Security-sensitive change"
upsert_label "privacy" "e377c2" "Privacy or biometric data handling"
upsert_label "pam-safety" "ff7f0e" "PAM safety, rollback, sudo/greeter auth"
upsert_label "model" "8c564b" "Model proposal or model integration"
upsert_label "research" "aec7e8" "Research or evaluation task"
upsert_label "testing" "98df8a" "Tests or manual verification"
upsert_label "release" "f7b6d2" "Release preparation"
upsert_label "blocked" "000000" "Blocked by another task"
upsert_label "needs-review" "ff9896" "Needs maintainer/security review"
upsert_label "good first issue" "7057ff" "Good for new contributors"
upsert_label "help wanted" "008672" "Help wanted"
upsert_label "bug" "d73a4a" "Bug or regression"
upsert_label "enhancement" "a2eeef" "Feature or improvement"

echo
echo "[bootstrap-labels] Done."
