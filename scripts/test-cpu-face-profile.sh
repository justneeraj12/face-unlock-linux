#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

echo "[test-cpu-face-profile] CPU face profile tests"

python3 - <<'PY'
import sys
from pathlib import Path

import numpy as np

repo = Path.cwd()
sys.path.insert(0, str(repo / "python"))

from detectors.base import Detection
from face_profile import POSE_SLOTS, FaceProfileBuilder, classify_pose

landmarks = [(30, 40), (70, 40), (50, 58), (38, 75), (62, 75)]
center = Detection(0, 0, 100, 100, 1.0, "test", landmarks)
assert classify_pose(center) == "center"

left = Detection(0, 0, 100, 100, 1.0, "test", [
    (30, 40), (70, 40), (40, 58), (38, 75), (62, 75)
])
assert classify_pose(left) == "left"

up = Detection(0, 0, 100, 100, 1.0, "test", [
    (30, 40), (70, 40), (50, 50), (38, 75), (62, 75)
])
assert classify_pose(up) == "up"

builder = FaceProfileBuilder(
    model_id="synthetic-test-model",
    target_per_pose=3,
    maximum_per_pose=5,
)

rng = np.random.default_rng(7)
bases = {}

for pose in POSE_SLOTS:
    base = rng.normal(size=128).astype(np.float32)
    base /= np.linalg.norm(base)
    bases[pose] = base

    duplicate = builder.add_sample(base, pose, 1.0)
    assert duplicate.accepted
    rejected_duplicate = builder.add_sample(base, pose, 1.0)
    assert not rejected_duplicate.accepted
    assert rejected_duplicate.reason == "duplicate_sample"

    for sample_index in range(1, 3):
        noise = rng.normal(size=128).astype(np.float32) * (0.01 * sample_index)
        result = builder.add_sample(base + noise, pose, 1.0)
        assert result.accepted, (pose, result)

assert builder.ready()
assert builder.progress_percent() == 100
profile = builder.finalize()
assert profile.embedding_dim == 128
assert set(profile.pose_centroids) == set(POSE_SLOTS)
assert profile.summary()["contains_raw_images"] is False
assert profile.match_score(bases["center"]) > 0.95

print("pose_classification_status: ok")
print("profile_builder_status: ok")
print("profile_match_status: ok")
PY

python3 python/prototype_enroll_cpu.py --help >/tmp/face-unlock-enroll-cpu-help.txt

grep -q -- "--yunet-model" /tmp/face-unlock-enroll-cpu-help.txt
grep -q -- "--sface-model" /tmp/face-unlock-enroll-cpu-help.txt
grep -q -- "--i-understand-biometric-risk" /tmp/face-unlock-enroll-cpu-help.txt
rm -f /tmp/face-unlock-enroll-cpu-help.txt

python3 python/benchmark_cpu_models.py --help >/tmp/face-unlock-cpu-benchmark-help.txt
grep -q -- "--iterations" /tmp/face-unlock-cpu-benchmark-help.txt
rm -f /tmp/face-unlock-cpu-benchmark-help.txt

echo "enrollment_help_status: ok"
echo "cpu_benchmark_help_status: ok"

if [[ -f models/face_detection_yunet.onnx && -f models/face_recognition_sface_2021dec.onnx ]]; then
  python3 - <<'PY'
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path.cwd() / "python"))

from detectors.yunet import YuNetFaceDetector
from recognizers.sface import SFaceRecognizer

detector = YuNetFaceDetector(Path("models/face_detection_yunet.onnx"))
assert detector.detect(np.zeros((320, 320, 3), dtype=np.uint8)) == []

recognizer = SFaceRecognizer(Path("models/face_recognition_sface_2021dec.onnx"))
embedding = recognizer.embed_aligned(np.zeros((112, 112, 3), dtype=np.uint8))
assert embedding.shape == (128,)
assert np.isfinite(embedding).all()
assert abs(float(np.linalg.norm(embedding)) - 1.0) < 1e-5

print("cpu_model_smoke_status: ok")
PY
  python3 python/benchmark_cpu_models.py --warmup 1 --iterations 3
else
  echo "cpu_model_smoke_status: skipped_models_not_downloaded"
fi

echo "status: ok"
