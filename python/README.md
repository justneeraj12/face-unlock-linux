# Python Prototypes

This directory contains Python scripts for rapid experimentation.

Prototype scripts may be used for:

- camera capture tests
- detector experiments
- face alignment experiments
- embedding model export
- threshold evaluation

Python prototype scripts are not part of the trusted authentication path.


## CPU face profile prototype

Download pinned local model files:

    ./scripts/download-cpu-models.sh

Run the guided, non-persistent enrollment prototype:

    python3 python/prototype_enroll_cpu.py --i-understand-biometric-risk

The prototype uses OpenCV's CPU backend and saves no face images, crops,
embeddings, or templates. See `docs/cpu-face-profile.md`.


Run the camera-free CPU latency benchmark after downloading models:

    python3 python/benchmark_cpu_models.py --iterations 100
