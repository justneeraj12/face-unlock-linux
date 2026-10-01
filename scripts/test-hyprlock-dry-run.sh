#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
fixture_dir="$(mktemp -d /tmp/face-unlock-hyprlock-test.XXXXXX)"
cleanup() {
  rm -rf "$fixture_dir"
}
trap cleanup EXIT

fixture="$fixture_dir/hyprlock"
printf '# PAM fixture for Hyprlock\nauth include login\n' > "$fixture"
before="$(sha256sum "$fixture" | awk '{print $1}')"

HYPRLOCK_PAM_PATH="$fixture" "$script_dir/plan-hyprlock-pam-install.sh" \
  > "$fixture_dir/plan.out"
HYPRLOCK_PAM_PATH="$fixture" "$script_dir/apply-hyprlock-pam-install.sh" \
  > "$fixture_dir/apply.out"

grep -Fq 'auth sufficient pam_face_unlock.so timeout_ms=1000' \
  "$fixture_dir/plan.out"
grep -Fq 'This script made no changes.' "$fixture_dir/plan.out"
grep -Fq 'DRY RUN ONLY. No changes made.' "$fixture_dir/apply.out"

after="$(sha256sum "$fixture" | awk '{print $1}')"
if [[ "$before" != "$after" ]]; then
  echo "hyprlock_pam_unchanged: false"
  exit 1
fi
if grep -Fq 'pam_face_unlock.so' "$fixture"; then
  echo "hyprlock_pam_fixture_modified: true"
  exit 1
fi

echo "hyprlock_plan_status: ok"
echo "hyprlock_apply_dry_run_status: ok"
echo "hyprlock_pam_unchanged: true"
echo "hyprlock_dry_run_test_status: ok"
