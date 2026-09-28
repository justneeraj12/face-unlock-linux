# Python Prototypes

python/ contains rapid CPU-model and evaluation prototypes. Python is not part
of the trusted PAM path.

## Implemented

- safe camera capture with no default persistence
- noop, Haar, and YuNet detector abstraction
- CPU SFace alignment and normalized embeddings
- guided multi-pose enrollment
- pose coverage and duplicate rejection
- compact pose-centroid profile construction
- TorchScript export stub
- model evaluation scaffold
- synthetic CPU benchmark

## Setup

Download pinned and checksum-verified models:

    ./scripts/download-cpu-models.sh

Run camera-free tests:

    ./scripts/test-python-detectors.sh
    ./scripts/test-cpu-face-profile.sh

Run guided enrollment without saving biometric data:

    python3 python/prototype_enroll_cpu.py --i-understand-biometric-risk

Run the benchmark:

    python3 python/benchmark_cpu_models.py --iterations 100

Saving crops, metadata, embeddings, or templates requires explicit risk flags.
Local generated data and model weights are ignored by Git.

See [CPU face profile](../docs/cpu-face-profile.md) and
[model evaluation plan](../docs/model-evaluation-plan.md).
