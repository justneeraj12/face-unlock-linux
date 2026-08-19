# Candidate Models

This document tracks candidate models for future real face recognition.

No model is selected yet.

Do not commit model artifacts until licensing and redistribution are reviewed.

## Current status

Current implemented model support:

- TorchScript export stub
- optional LibTorch loader scaffold
- random Python embedding prototype

Real detector and embedding models are not integrated yet.

## Selection rules

A model candidate must document:

- source
- license
- redistribution rights
- input shape
- preprocessing
- output shape
- export path
- expected latency
- threshold calibration method
- known limitations

## Detector candidates

| Candidate | Type | Pros | Concerns | Status |
|---|---|---|---|---|
| OpenCV Haar cascade | classical detector | already available, simple baseline | weak accuracy, poor robustness | baseline only |
| OpenCV YuNet | lightweight face detector | OpenCV ecosystem, modern lightweight detector | confirm model license/export details | candidate |
| MTCNN | detector + landmarks | common, good prototype option | multiple implementations/licenses, slower | candidate |
| RetinaFace | detector + landmarks | strong detection quality | model size, license/export complexity | candidate |
| SCRFD-style detector | lightweight modern detector | good speed/accuracy tradeoff | source/license selection needed | candidate |
| MediaPipe Face Detection | lightweight detector | good real-time behavior | integration/export path differs | candidate |

## Alignment candidates

| Candidate | Pros | Concerns | Status |
|---|---|---|---|
| detector-provided landmarks | simple pipeline | depends on detector quality | preferred if available |
| 5-point landmark model | standard face alignment | extra model dependency | candidate |
| no alignment baseline | simple | weaker recognition accuracy | baseline only |

## Embedding candidates

| Candidate | Pros | Concerns | Status |
|---|---|---|---|
| MobileFaceNet-style model | lightweight, laptop-friendly | need licensed pretrained weights | candidate |
| ArcFace ResNet-style model | strong recognition quality | larger, GPU preferred | candidate |
| FaceNet-style model | common conceptually | many old/unclear implementations | candidate |
| lightweight ONNX face embedding model | export-friendly | license/source varies | candidate |
| project EmbeddingStub | already implemented | random, not real recognition | test only |

## Preferred first real prototype path

Recommended first real prototype direction:

1. Use Python to evaluate detector and embedding candidates.
2. Prefer models with clear license and TorchScript/ONNX path.
3. Keep daemon model loading optional.
4. Implement enrollment and matching in Python first.
5. Move only the stable path into C++ daemon.

## License review checklist

Before using a model:

- [ ] source URL recorded
- [ ] license recorded
- [ ] redistribution allowed
- [ ] commercial restrictions understood
- [ ] citation requirements documented
- [ ] model artifact not committed unless allowed
- [ ] export script documented

## Performance review checklist

For each candidate:

- [ ] CPU latency measured
- [ ] GPU latency measured if applicable
- [ ] model load time measured
- [ ] memory use noted
- [ ] detector success rate tested
- [ ] embedding output shape verified
- [ ] end-to-end prototype latency estimated

## Security review checklist

For each candidate:

- [ ] threshold calibration plan exists
- [ ] false accept risk considered
- [ ] false reject behavior considered
- [ ] spoof/liveness limitations documented
- [ ] password fallback remains available

## Current decision

No real model is selected yet.

The next step is a Python evaluation script that can load a candidate model and produce local-only metrics.

## Proposing new candidates

Use the GitHub Model candidate issue template.

Required information:

- source URL
- license
- redistribution rights
- input/output details
- preprocessing requirements
- export path
- performance notes
- threshold calibration notes
- liveness/spoofing limitations
- privacy/security concerns

Do not attach private biometric data.
