# Packaging

packaging/ contains assets for development Debian packages and the per-user
systemd service.

## Current behavior

The CPack package installs daemon and GUI binaries, a desktop launcher, an app
icon, documentation, helper scripts, and service assets. It does not automatically:

- edit PAM files
- enable sudo or login face authentication
- start a system service
- remove password fallback
- download model weights

Build and inspect a package:

    ./scripts/package-deb.sh
    dpkg-deb -I build-gui/*.deb
    dpkg-deb -c build-gui/*.deb

## Production requirements

A daily-use package still needs:

- reviewed model redistribution rights
- complete runtime dependencies
- versioned model placement
- first-run GUI enrollment
- explicit per-user service setup
- explicit and reversible PAM opt-in
- tested upgrade, uninstall, and rollback

See [packaging design](../docs/packaging.md).
