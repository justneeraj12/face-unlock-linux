# systemd User Service

This document explains how to run face-unlockd as a normal user service.

## Purpose

The user service starts the daemon as the logged-in desktop user.

It does not run as root.

It creates the daemon socket at:

    /run/user/$UID/face-unlock.sock

or equivalently:

    $XDG_RUNTIME_DIR/face-unlock.sock

## Safety

The user service does not modify PAM files.

It does not modify:

    /etc/pam.d/sudo
    /etc/pam.d/gdm-password
    /etc/pam.d/sddm
    /etc/pam.d/lightdm
    /etc/pam.d/common-auth

The installed service uses:

    FACE_UNLOCK_DEV_ALLOW=0

So auth remains fail-closed by default.

## Install

Build first:

    ./scripts/build.sh

Install and start the user service:

    ./scripts/install-user-service.sh

The script requires typing YES before installing.

## Check status

Check service status:

    systemctl --user status face-unlockd.service

Check logs:

    journalctl --user -u face-unlockd.service -n 100 --no-pager

Check socket:

    ls -l "$XDG_RUNTIME_DIR/face-unlock.sock"

Expected socket permissions:

    srw-------

## Test IPC

Test ping:

    ./scripts/test-socket-client.sh ping

Test camera status:

    ./scripts/test-socket-client.sh camera_status

Test auth default fail-closed behavior:

    ./scripts/test-socket-client.sh auth

Expected auth behavior:

    status fail
    reason auth_not_implemented

## Stop service

Stop without uninstalling:

    systemctl --user stop face-unlockd.service

Start again:

    systemctl --user start face-unlockd.service

Disable autostart:

    systemctl --user disable --now face-unlockd.service

## Remove

Remove service and local daemon binary:

    ./scripts/remove-user-service.sh

This removes:

    ~/.config/systemd/user/face-unlockd.service
    ~/.local/bin/face-unlockd

It also removes the runtime socket if present.

## Development auth

Do not enable FACE_UNLOCK_DEV_ALLOW=1 in the installed service.

Development auth should only be used manually during fake PAM tests:

    FACE_UNLOCK_DEV_ALLOW=1 ./build/daemon/face-unlockd --camera 0 --daemon

Never use development auth as real authentication.

## Root auth peer default

The user service sets:

    FACE_UNLOCK_ALLOW_ROOT_AUTH=0

This means root-owned sudo PAM auth requests are rejected by default.

For manual sudo development testing, run the daemon manually with:

    FACE_UNLOCK_ALLOW_ROOT_AUTH=1

Do not enable root auth peers casually.
