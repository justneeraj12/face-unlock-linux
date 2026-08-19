#!/usr/bin/env bash
set -euo pipefail

repo="${1:-justneeraj12/face-unlock-linux}"

echo "[bootstrap-milestones] Bootstrapping milestones for $repo"
echo
echo "This script uses GitHub CLI API calls."
echo
echo "It will create milestones if they do not already exist."
echo "It will NOT modify local PAM/system/authentication files."
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

owner="${repo%%/*}"
name="${repo#*/}"

milestone_exists() {
  local title="$1"
  gh api "repos/$owner/$name/milestones?state=all&per_page=100" \
    --jq '.[].title' | grep -Fxq "$title"
}

create_milestone() {
  local title="$1"
  local description="$2"

  if milestone_exists "$title"; then
    echo "Milestone exists: $title"
  else
    echo "Creating milestone: $title"
    gh api "repos/$owner/$name/milestones" \
      --method POST \
      --field title="$title" \
      --field description="$description" \
      --field state="open" >/dev/null
  fi
}

create_milestone "v0.2.0-dev-auth-sudo" \
  "Development-only sudo prototype hardening and documentation."

create_milestone "v0.3.0-enrollment-cli" \
  "CLI enrollment, encrypted template writing, manifest validation, forget-me flow."

create_milestone "v0.4.0-real-model-prototype" \
  "First real detector/embedding model prototype and threshold calibration."

create_milestone "v0.5.0-qt-enrollment" \
  "Usable Qt enrollment flow with preview, quality checks, and encrypted templates."

create_milestone "v0.6.0-lock-screen-prototype" \
  "Logged-in-session lock-screen integration prototype with rollback docs."

create_milestone "v1.0.0-security-review" \
  "Production readiness, security review, liveness limitations, installer audit."

echo
echo "[bootstrap-milestones] Done."
