# Threat Model

This document describes expected threats and design responses.

## Assets

The project protects:

- local user account access
- encrypted face templates
- authentication decisions
- camera privacy
- PAM configuration integrity

## Sensitive data

Sensitive data includes:

- raw face images
- face crops
- face embeddings
- template encryption keys
- encrypted templates
- authentication logs if too detailed

## Assumptions

Initial assumptions:

- attacker may have local unprivileged access
- attacker may try to spoof the daemon socket
- attacker may try to replay IPC messages
- attacker may try to use printed/displayed face images
- attacker may inspect files in user home if permissions allow
- root compromise is out of scope for local protection

## Threats and mitigations

## Socket spoofing

Threat:

An attacker creates a fake socket and tricks PAM into accepting authentication.

Mitigations:

- socket path under `/run/user/$UID`
- strict socket permissions
- daemon verifies peer credentials
- PAM verifies expected path
- future: daemon identity verification or systemd socket activation

## Unauthorized peer

Threat:

Another user connects to the daemon and asks for authentication.

Mitigations:

- socket mode `0600`
- `SO_PEERCRED` checks
- reject unexpected UID/GID

## Template theft

Threat:

An attacker copies face templates from disk.

Mitigations:

- encrypt templates at rest
- per-user keys
- restrictive permissions
- no raw image storage by default

## Replay attacks

Threat:

An attacker replays a previous successful daemon response.

Mitigations:

- request/response over connected local socket
- no reusable token in starter design
- future: nonce in request and response

## Camera spoofing

Threat:

An attacker presents a photo or video to the camera.

Mitigations planned:

- liveness heuristics
- multi-frame checks
- blink/head movement challenge optional
- depth/IR support optional if hardware exists

## Denial of service

Threat:

An attacker floods the daemon socket.

Mitigations:

- small request size
- connection limit
- rate limiting
- timeout
- reject unauthorized peers

## PAM misconfiguration

Threat:

Incorrect PAM config locks user out or bypasses password authentication.

Mitigations:

- never modify PAM without consent
- timestamped backups
- rollback script
- fake PAM test service first
- always recommend keeping root shell open

## Privacy leaks

Threat:

Logs or crash dumps expose images/templates.

Mitigations:

- no image logging
- no embedding logging
- no telemetry by default
- opt-in debug capture only
