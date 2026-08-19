#!/usr/bin/env bash
set -euo pipefail

repo="${1:-justneeraj12/face-unlock-linux}"

echo "[bootstrap-issues] Creating starter issues for $repo"
echo
echo "This script uses GitHub CLI."
echo
echo "It will create GitHub issues."
echo "It will NOT modify local system files, PAM files, sudo, login, or lock-screen config."
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

issue_exists() {
  local title="$1"

  gh issue list \
    --repo "$repo" \
    --state all \
    --search "$title in:title" \
    --json title \
    --jq '.[].title' | grep -Fxq "$title"
}

create_issue() {
  local title="$1"
  local labels="$2"
  local milestone="$3"
  local body="$4"

  if issue_exists "$title"; then
    echo "Issue already exists: $title"
    return
  fi

  echo "Creating issue: $title"

  if [[ -n "$milestone" ]]; then
    gh issue create \
      --repo "$repo" \
      --title "$title" \
      --label "$labels" \
      --milestone "$milestone" \
      --body "$body" >/dev/null
  else
    gh issue create \
      --repo "$repo" \
      --title "$title" \
      --label "$labels" \
      --body "$body" >/dev/null
  fi
}

create_issue \
  "Harden guarded sudo apply and rollback flow" \
  "area/pam,pam-safety,security,needs-review" \
  "v0.2.0-dev-auth-sudo" \
"## Goal

Improve the guarded sudo apply and rollback flow.

## Tasks

- [ ] Review apply-sudo-pam-install.sh prompts.
- [ ] Verify backup naming and rollback instructions.
- [ ] Confirm duplicate-line detection.
- [ ] Confirm password fallback behavior.
- [ ] Document sudo test matrix.
- [ ] Confirm root-peer auth behavior.
- [ ] Add more dry-run checks if needed.

## Safety

Do not modify GDM, SDDM, LightDM, common-auth, or lock-screen PAM files."

create_issue \
  "Design real key management for encrypted templates" \
  "security,privacy,area/daemon,needs-review" \
  "v0.3.0-enrollment-cli" \
"## Goal

Design key management for real encrypted templates.

## Candidate approaches

- kernel keyring
- GNOME Keyring
- passphrase-wrapped local key
- hardware-backed storage where available

## Requirements

- [ ] templates encrypted at rest
- [ ] keys not logged
- [ ] no raw images stored
- [ ] forget-me deletes template and manifest
- [ ] recovery behavior documented
- [ ] threat model updated"

create_issue \
  "Implement real enrollment CLI prototype" \
  "area/daemon,privacy,testing" \
  "v0.3.0-enrollment-cli" \
"## Goal

Create a CLI prototype that writes a real encrypted enrollment template once a model path exists.

## Tasks

- [ ] define input format
- [ ] read local embeddings or crops only with consent
- [ ] write encrypted template
- [ ] write enrollment manifest
- [ ] validate manifest
- [ ] delete via template tool
- [ ] update docs

## Non-goals

No production auth yet."

create_issue \
  "Evaluate first detector candidates" \
  "area/models,model,research" \
  "v0.4.0-real-model-prototype" \
"## Goal

Evaluate detector candidates for the first real model prototype.

## Candidates

- OpenCV YuNet
- MTCNN
- RetinaFace
- SCRFD-style detector
- MediaPipe face detection

## Tasks

- [ ] document licenses
- [ ] document preprocessing
- [ ] measure CPU latency
- [ ] measure detection reliability
- [ ] check export path
- [ ] update docs/model-candidates.md"

create_issue \
  "Evaluate first embedding model candidates" \
  "area/models,model,research,privacy" \
  "v0.4.0-real-model-prototype" \
"## Goal

Evaluate embedding model candidates for real matching.

## Tasks

- [ ] shortlist licensed models
- [ ] document source and redistribution rights
- [ ] export to TorchScript or ONNX
- [ ] run Python evaluation harness
- [ ] produce metrics JSON locally
- [ ] calibrate initial threshold
- [ ] document spoof/liveness limitations

## Safety

Do not commit model artifacts or biometric data unless explicitly allowed."

create_issue \
  "Add Qt GUI camera preview implementation" \
  "area/gui,privacy,enhancement" \
  "v0.5.0-qt-enrollment" \
"## Goal

Replace the GUI camera preview placeholder with a real preview.

## Requirements

- [ ] clearly indicate camera active
- [ ] no frame saving by default
- [ ] release camera on close
- [ ] handle unavailable camera gracefully
- [ ] update privacy docs
- [ ] keep GUI optional"

create_issue \
  "Implement Qt enrollment pose guidance" \
  "area/gui,privacy,enhancement" \
  "v0.5.0-qt-enrollment" \
"## Goal

Implement guided enrollment pose slots.

## Pose slots

- center
- left
- right
- up
- down

## Tasks

- [ ] connect pose slots to preview
- [ ] quality gate per pose
- [ ] progress display
- [ ] no raw image saving by default
- [ ] update enrollment manifest plan"

create_issue \
  "Implement GUI Forget Me using shared deletion logic" \
  "area/gui,privacy,security" \
  "v0.5.0-qt-enrollment" \
"## Goal

Make GUI Forget Me robust and share deletion behavior with template tooling.

## Tasks

- [ ] delete template.enc
- [ ] delete enrollment.json
- [ ] show final status
- [ ] handle partial deletion errors
- [ ] update docs
- [ ] add manual test plan"

create_issue \
  "Create GNOME and KDE lock-screen test matrix" \
  "area/docs,pam-safety,testing" \
  "v0.6.0-lock-screen-prototype" \
"## Goal

Document lock-screen test matrix for logged-in sessions.

## Environments

- GNOME Wayland
- GNOME X11 if available
- KDE Plasma Wayland
- KDE Plasma X11

## Tasks

- [ ] document PAM service files
- [ ] identify safe rollback paths
- [ ] test password fallback
- [ ] document Wayland caveats
- [ ] do not modify greeter login yet"

create_issue \
  "Prepare security review checklist for v1.0" \
  "security,privacy,release,needs-review" \
  "v1.0.0-security-review" \
"## Goal

Create a production readiness security review checklist.

## Topics

- PAM audit
- daemon IPC audit
- SO_PEERCRED policy
- template encryption
- key management
- liveness limitations
- spoofing risks
- installer rollback
- logging privacy
- dependency audit"

echo
echo "[bootstrap-issues] Done."
