# Daemon

This directory will contain the user-level face unlock daemon.

The daemon is responsible for:

- opening the camera
- maintaining a recent frame buffer
- loading the face recognition model
- reading encrypted templates
- performing face matching
- exposing a local UNIX domain socket
- enforcing authentication rate limits

The daemon should run as the normal desktop user, not as root.

The daemon must fail closed if:

- the camera is unavailable
- no template exists
- the model is unavailable
- the IPC peer is not trusted
- authentication times out
