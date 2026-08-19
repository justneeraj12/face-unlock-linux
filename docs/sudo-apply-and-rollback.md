# sudo Apply and Rollback

This document describes the guarded sudo PAM apply and rollback scripts.

## Warning

sudo PAM changes are dangerous.

A broken sudo PAM file can lock you out of sudo.

Before applying sudo integration:

- keep a separate root-authenticated shell open
- run fake PAM service testing first
- confirm password fallback works
- confirm daemon service is running
- confirm rollback command is available
- inspect the diff carefully

## Scripts

Dry-run or apply:

    ./scripts/apply-sudo-pam-install.sh
    ./scripts/apply-sudo-pam-install.sh --apply

Rollback:

    ./scripts/rollback-sudo-pam.sh /etc/pam.d/sudo.face-unlock-backup.TIMESTAMP
    ./scripts/rollback-sudo-pam.sh --latest

## Default behavior

The apply script defaults to dry-run.

Dry-run mode:

- reads /etc/pam.d/sudo
- finds an insertion point
- writes a proposed file to /tmp
- prints a unified diff
- prints backup command
- prints rollback command
- makes no changes

## Apply behavior

Apply mode requires:

    --apply

It also requires typing exact confirmation phrases:

    I_HAVE_A_ROOT_SHELL
    FAKE_PAM_TEST_PASSED
    APPLY_SUDO_PAM_CHANGE

Before applying, it checks:

- pam_face_unlock.so exists in the PAM module directory
- daemon socket exists
- sudo PAM does not already contain pam_face_unlock.so

It creates a backup:

    /etc/pam.d/sudo.face-unlock-backup.TIMESTAMP

Then it installs the proposed sudo PAM file.

## Planned PAM line

The planned line is:

    auth sufficient pam_face_unlock.so timeout_ms=1000

It is inserted before the normal password auth path when possible.

## Why sufficient

The module is sufficient so password fallback remains available if face auth fails.

The face unlock module must never be the only sudo authentication method during development.

## Important current limitation

Depending on sudo/PAM behavior, the PAM client may connect to the daemon as root.

The daemon peer policy must explicitly support that before sudo face-auth success can work.

If root peer support is not enabled, face auth should fail and sudo should fall back to password.

This is safer than accidental success.

## Rollback

Rollback with a specific backup:

    ./scripts/rollback-sudo-pam.sh /etc/pam.d/sudo.face-unlock-backup.TIMESTAMP

Rollback latest backup:

    ./scripts/rollback-sudo-pam.sh --latest

After rollback, test sudo:

    sudo -k
    sudo true

## Files not touched

These scripts do not modify:

    /etc/pam.d/gdm-password
    /etc/pam.d/sddm
    /etc/pam.d/lightdm
    /etc/pam.d/common-auth

## Recommended test order

1. Run local verification.

       ./scripts/verify-local.sh

2. Install user service.

       ./scripts/install-user-service.sh

3. Run fake PAM test.

       ./scripts/install-fake-pam-test.sh
       pamtester face-unlock-test "$USER" authenticate
       ./scripts/remove-fake-pam-test.sh

4. Run sudo dry-run.

       ./scripts/apply-sudo-pam-install.sh

5. Only if comfortable, apply.

       ./scripts/apply-sudo-pam-install.sh --apply

6. Test sudo in a new terminal while keeping recovery shell open.

       sudo -k
       sudo true

7. Roll back immediately if anything looks wrong.

       ./scripts/rollback-sudo-pam.sh --latest

## Root peer policy

sudo PAM clients may connect to the daemon as UID 0.

The daemon root-peer policy is documented in:

    docs/sudo-root-peer-policy.md

Current policy:

- same UID clients are allowed
- root is allowed only for auth requests
- root non-auth requests are rejected
- other users are rejected

## Root auth peer opt-in

For sudo PAM face-auth success, the daemon must allow root-owned auth peers.

This is controlled by:

    FACE_UNLOCK_ALLOW_ROOT_AUTH=1

Default is disabled.

The packaged/user service default is:

    FACE_UNLOCK_ALLOW_ROOT_AUTH=0

If disabled, sudo should fall back to password.

## Apply script root-peer preflight

The guarded apply script checks root-owned auth peer behavior before applying sudo PAM changes.

If the daemon rejects root auth peers with:

    peer_not_allowed

the script warns that sudo face auth will likely fall back to password.

To continue applying anyway, it requires:

    ROOT_AUTH_REJECTED_OK

For development-only sudo face-auth testing, run the daemon manually with:

    FACE_UNLOCK_ALLOW_ROOT_AUTH=1

or:

    FACE_UNLOCK_ALLOW_ROOT_AUTH=1 FACE_UNLOCK_DEV_ALLOW=1

## Dry-run regression test

Dry-run behavior can be tested with:

    ./scripts/test-sudo-dry-run.sh

The test verifies that /etc/pam.d/sudo is unchanged after running dry-run planners.

## Troubleshooting

If sudo integration behaves unexpectedly, see:

    docs/sudo-troubleshooting.md
