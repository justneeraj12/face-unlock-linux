#!/usr/bin/env bash
set -euo pipefail

sudo_pam="/etc/pam.d/sudo"

echo "[test-sudo-dry-run] Testing sudo PAM dry-run scripts"
echo
echo "This script will:"
echo "  - checksum $sudo_pam before"
echo "  - run sudo PAM planning scripts in dry-run mode"
echo "  - checksum $sudo_pam after"
echo "  - fail if the file changed"
echo
echo "It will NOT run --apply."
echo "It will NOT modify PAM files."
echo

if [[ ! -f "$sudo_pam" ]]; then
  echo "ERROR: sudo PAM file not found: $sudo_pam"
  exit 1
fi

before_hash="$(sha256sum "$sudo_pam" | awk '{print $1}')"

echo "before_hash: $before_hash"
echo

echo "[test-sudo-dry-run] Running plan-sudo-pam-install.sh"
./scripts/plan-sudo-pam-install.sh >/tmp/face-unlock-plan-sudo-pam.out

if ! grep -q "This script made no changes." /tmp/face-unlock-plan-sudo-pam.out; then
  echo "ERROR: plan script did not report no changes"
  cat /tmp/face-unlock-plan-sudo-pam.out
  exit 1
fi

echo "plan_status: ok"

echo
echo "[test-sudo-dry-run] Running apply-sudo-pam-install.sh dry-run"
./scripts/apply-sudo-pam-install.sh >/tmp/face-unlock-apply-sudo-pam-dry-run.out

if ! grep -q "DRY RUN ONLY. No changes made." /tmp/face-unlock-apply-sudo-pam-dry-run.out; then
  echo "ERROR: apply dry-run did not report no changes"
  cat /tmp/face-unlock-apply-sudo-pam-dry-run.out
  exit 1
fi

echo "apply_dry_run_status: ok"

after_hash="$(sha256sum "$sudo_pam" | awk '{print $1}')"

echo
echo "after_hash: $after_hash"

if [[ "$before_hash" != "$after_hash" ]]; then
  echo "ERROR: $sudo_pam changed during dry-run test"
  exit 1
fi

rm -f /tmp/face-unlock-plan-sudo-pam.out
rm -f /tmp/face-unlock-apply-sudo-pam-dry-run.out

echo
echo "sudo_pam_unchanged: true"
echo "status: ok"
