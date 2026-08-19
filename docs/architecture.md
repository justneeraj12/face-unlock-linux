# Architecture

face-unlock-linux is designed as a set of separated components.

The core rule is:

    Privileged or security-sensitive components must stay small.

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

    /run/user/$UID/face-unlock.sock

Planned socket properties:

- owned by user
- mode 0600
- uses SO_PEERCRED checks
- accepts same user and controlled privileged clients
- bounded request/response size
- short timeout

## Authentication flow

Typical sudo flow:

    user runs sudo
        |
        v
    PAM loads pam_face_unlock.so
        |
        v
    PAM module connects to user daemon
        |
        v
    daemon checks camera/model/template
        |
        v
    daemon returns ok/fail
        |
        v
    PAM either succeeds or falls back to password

## Fail-closed behavior

Authentication should fail if:

- daemon is not running
- socket is missing
- camera unavailable
- model unavailable
- template missing
- timeout occurs
- peer credentials are invalid
- retry limit exceeded

Failure should not block password fallback when configured as auth sufficient.

## Greeter/login caveat

Login greeters are harder than lock-screen or sudo authentication because the user session may not exist yet.

For greeter login, the project may need:

- a system helper
- greeter-specific plugin
- strict separation between users
- careful template access control

Greeter integration is not part of the first implementation phase.
