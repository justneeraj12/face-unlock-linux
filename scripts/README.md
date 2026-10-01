# Scripts

scripts/ contains build, test, model, packaging, service, and PAM safety helpers.

Common entry points:

    ./scripts/build.sh
    ./scripts/test.sh
    ./scripts/verify-local.sh
    ./scripts/audit-dependencies.sh
    ./scripts/download-cpu-models.sh
    ./scripts/build-gui.sh
    ./scripts/plan-hyprlock-pam-install.sh

Scripts must be safe by default. Any helper that can change authentication or
system files must print exact paths, require explicit confirmation, create a
backup, and provide rollback instructions.

The Hyprlock planner and apply helper are dry-run by default. The apply helper
also refuses to proceed until an installed module, daemon socket, explicit
authentication success, and recognizable password fallback are present.

See [script inventory](../docs/script-inventory.md).
