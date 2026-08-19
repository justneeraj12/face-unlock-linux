# PAM Module

This directory contains the minimal PAM module.

## Current status

- builds a PAM module named pam_face_unlock.so
- connects to the user daemon socket
- sends an auth request
- waits for a bounded timeout
- returns PAM_SUCCESS only when daemon returns status ok for op auth
- returns PAM_AUTH_ERR on timeout, missing socket, failed connection, or fail response
- does not link OpenCV
- does not link LibTorch
- does not link Qt
- does not access the camera
- does not read templates

## Safety

The PAM module must stay tiny and auditable.

It should only:

- resolve the target user
- connect to /run/user/$UID/face-unlock.sock
- send an authentication request
- wait for a bounded timeout
- return PAM success or failure

The PAM module must not link to:

- OpenCV
- LibTorch
- Qt
- CUDA
- TensorRT
- Python

During early development, the PAM module must only be tested with a fake PAM service.

Do not modify sudo, GDM, SDDM, LightDM, or lock-screen PAM files yet.

## Build

From the repository root:

    ./scripts/build.sh

Module path after build:

    build/pam/pam_face_unlock.so

Audit dependencies:

    ldd build/pam/pam_face_unlock.so

Expected dependency rule:

- PAM/system C libraries are okay
- OpenCV should not appear
- Torch should not appear
- Qt should not appear
- CUDA should not appear

## Options

Supported PAM module options:

    timeout_ms=700
    debug

The timeout value must be between 50 and 5000 milliseconds.
