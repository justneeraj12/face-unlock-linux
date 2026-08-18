# PAM Module

This directory will contain the minimal PAM module.

The PAM module must stay tiny and auditable.

It should only:

- resolve the user
- connect to the user daemon socket
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
