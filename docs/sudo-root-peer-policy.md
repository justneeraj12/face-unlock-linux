# sudo Root Peer Policy

This document describes how the daemon handles root-owned socket clients for sudo PAM integration.

## Why this exists

When sudo invokes PAM, pam_face_unlock.so may run in a root-owned process.

The daemon normally runs as the desktop user and creates:

    /run/user/$UID/face-unlock.sock

A sudo PAM client may connect to that socket as UID 0.

The daemon uses SO_PEERCRED to inspect the peer UID.

## Current policy

Current socket peer policy:

| Peer UID | Operation | Decision |
|---|---|---|
| same UID as daemon | any known operation | allowed |
| root UID 0 | auth | allowed |
| root UID 0 | non-auth operations | rejected |
| other UID | any operation | rejected |

## Reasoning

Root is already privileged on the local system.

Allowing root for auth requests is needed for sudo PAM integration.

Restricting root to auth requests avoids unnecessarily broad socket access.

All peer credentials and policy decisions are logged by the daemon.

## Expected daemon logs

For same-user requests:

    peer_credentials: pid=123 uid=1000 gid=1000
    client_operation: ping
    peer_policy: allow_same_uid
    peer_status: allowed

For sudo/root auth requests:

    peer_credentials: pid=123 uid=0 gid=0
    client_operation: auth
    peer_policy: allow_root_auth
    peer_status: allowed

For root non-auth requests:

    peer_credentials: pid=123 uid=0 gid=0
    client_operation: ping
    peer_policy: reject
    peer_status: rejected

## Manual root-peer test

Start daemon:

    ./build/daemon/face-unlockd --camera 0 --daemon

In another terminal:

    SOCK="$XDG_RUNTIME_DIR/face-unlock.sock"
    sudo FACE_UNLOCK_SOCKET_PATH="$SOCK" ./scripts/test-socket-client.sh auth

Default expected response:

    status fail
    reason auth_not_implemented

This means root was allowed to ask for auth, but auth still failed closed.

Root ping should be rejected:

    SOCK="$XDG_RUNTIME_DIR/face-unlock.sock"
    sudo FACE_UNLOCK_SOCKET_PATH="$SOCK" ./scripts/test-socket-client.sh ping

Expected response:

    status fail
    reason peer_not_allowed

## Safety

This does not enable sudo integration by itself.

sudo integration still requires guarded PAM configuration changes.

See:

    docs/sudo-apply-and-rollback.md

## Explicit root auth enable flag

Root-owned auth peers are now opt-in.

Enable only for testing sudo PAM behavior:

    FACE_UNLOCK_ALLOW_ROOT_AUTH=1 ./build/daemon/face-unlockd --camera 0 --daemon

Default:

    FACE_UNLOCK_ALLOW_ROOT_AUTH=0

or unset.

When disabled, root auth requests are rejected with policy:

    reject_root_auth_disabled

The systemd user service sets:

    FACE_UNLOCK_ALLOW_ROOT_AUTH=0

by default.

## Apply script behavior

The guarded sudo apply script performs a root-owned auth peer preflight check.

If root auth peers are disabled, the script warns before applying sudo PAM changes.

This helps avoid confusion where sudo integration is installed but face auth always falls back to password.
