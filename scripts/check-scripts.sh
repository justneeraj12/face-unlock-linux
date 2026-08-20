#!/usr/bin/env bash
set -euo pipefail

echo "[check-scripts] Checking required scripts"

required_scripts=(
  scripts/check-docs.sh
  scripts/check-json.sh
  scripts/ci-install-deps.sh
  scripts/build.sh
  scripts/test.sh
  scripts/verify-local.sh
  scripts/audit-dependencies.sh
  scripts/package-deb.sh
  scripts/test-socket-client.sh
  scripts/install-fake-pam-test.sh
  scripts/remove-fake-pam-test.sh
  scripts/install-user-service.sh
  scripts/remove-user-service.sh
  scripts/plan-sudo-pam-install.sh
  scripts/apply-sudo-pam-install.sh
  scripts/rollback-sudo-pam.sh
  scripts/test-sudo-dry-run.sh
  scripts/test-key-template-flow.sh
  scripts/test-daemon-metadata.sh
  scripts/test-auth-reasons.sh
  scripts/test-python-detectors.sh
  scripts/validate-enrollment-manifest.py
  scripts/validate-model-eval-metrics.py
  scripts/validate-detector-output.py
)

missing=0

for script in "${required_scripts[@]}"; do
  if [[ ! -f "$script" ]]; then
    echo "[MISSING] $script"
    missing=1
    continue
  fi

  if [[ ! -x "$script" ]]; then
    echo "[NOT EXECUTABLE] $script"
    missing=1
    continue
  fi

  echo "[OK] $script"
done

if [[ "$missing" -ne 0 ]]; then
  echo "[check-scripts] FAILED"
  exit 1
fi

echo "[check-scripts] OK"
