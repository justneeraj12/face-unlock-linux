# Architecture

`face-unlock-linux` is designed as a set of separated components.

The core rule is:

> Privileged or security-sensitive components must stay small.

## Components

## 1. PAM module

The PAM module is a tiny C shared object loaded by PAM.

Responsibilities:

- connect to user daemon
- send authentication request
- wait for bounded response
- return PAM success/failure

Non-responsibilities:

- camera access
- model loading
- face detection
- face recognition
- template decryption
- GUI
- network access

## 2. User daemon

The daemon runs as the logged-in user.

Responsibilities:

- open camera
- keep model warm
- maintain frame buffer
- perform face detection and matching
- read encrypted template
- expose local UNIX socket
- enforce rate limits
- return authentication decision

The daemon should not require root privileges.

## 3. Enrollment GUI

The GUI runs as the user.

Responsibilities:

- explain consent
- show camera preview
- guide multi-angle enrollment
- check image quality
- handle optional brightness boost
- encrypt templates
- delete templates on request

## 4. Python prototypes

Python scripts are used for experimentation.

Responsibilities:

- capture sample images during development
- test face detectors
- export TorchScript models
- evaluate thresholds

Prototype scripts are not part of the trusted authentication path.

## IPC design

The PAM module and daemon communicate over a UNIX domain socket:

```text
/run/user/$UID/face-unlock.sock
