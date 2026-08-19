# Optional LibTorch Loader

The daemon has optional TorchScript loader scaffolding.

This is disabled by default.

## Default build

Default build does not require LibTorch:

    ./scripts/build.sh

This keeps the normal daemon, PAM module, and CI lightweight.

## Export stub model

Generate a local TorchScript stub:

    python3 python/export_torchscript_stub.py

Default output:

    models/embedding_stub.pt

Model files are ignored by Git.

## Build with LibTorch

Download or install LibTorch separately.

Then configure with:

    cmake -S . -B build-torch -DWITH_TORCH=ON -DCMAKE_PREFIX_PATH=/path/to/libtorch
    cmake --build build-torch

Run model test:

    ./build-torch/daemon/face-unlockd --model-test --model models/embedding_stub.pt

Expected output:

    torch_status: enabled
    model_load_status: ok
    model_forward_status: ok
    status: ok

## If built without LibTorch

Running model test on a normal build prints:

    torch_status: disabled
    status: torch_not_enabled

## Current limitations

This is not real face recognition yet.

The model test only:

- loads a TorchScript module
- runs a dummy tensor through it
- prints output shape

Future work will add:

- detector model loading
- embedding model loading
- preprocessing
- face alignment
- encrypted template comparison
