# Enrollment format

The daemon stores a compact encrypted face profile plus a JSON metadata
manifest. Raw frames and aligned face crops are never part of either format.

## Files

Per-user files are stored under:

    ~/.local/share/face-unlock/template.enc
    ~/.local/share/face-unlock/template.key
    ~/.local/share/face-unlock/enrollment.json

All three are written with mode 0600. The local key file is development key
storage and remains a release blocker; it is not presented as production key
management.

The static contract and placeholder example are:

    schemas/enrollment-manifest.schema.json
    schemas/enrollment-manifest.example.json

Validate a manifest with:

    ./scripts/validate-enrollment-manifest.py ~/.local/share/face-unlock/enrollment.json

## Encrypted profile payload version 1

The plaintext payload is an internal binary format passed directly to
libsodium secretbox encryption. It must never be written separately.

All integers and IEEE-754 float32 values use little-endian byte order.

| Field | Constraint |
|---|---|
| Magic | 8 bytes: `FULPRF1\0` |
| Version | unsigned 16-bit value `1` |
| Flags | zero in version 1 |
| Model ID | 1-128 ASCII letters, digits, dot, underscore, or hyphen |
| Embedding dimension | 1-4096 |
| Pose templates | exactly center, left, right, up, down in that order |
| Sample count | 1-64 per pose |
| Centroid | finite, L2-normalized float32 vector |

The parser rejects unknown versions or flags, invalid dimensions, missing or
reordered poses, non-finite or non-normalized centroids, truncation, and
trailing data. The payload contains no raw images, crops, username, UID,
acceptance threshold, or encryption key.

## Commit gate

A real profile is committed only after two separate stages:

1. Training samples build one normalized centroid for each of the five poses.
2. Independent held-out samples are compared with those centroids.

The default policy requires three training samples and one held-out sample per
pose: 15 samples build the profile and five additional samples validate it.
Held-out embeddings are scored and discarded; they are not folded back into
the stored centroids.

The current minimum held-out cosine similarity is 0.45. This is a provisional
enrollment-consistency floor for development. It is not a calibrated
authentication threshold and cannot authorize an unlock.

The storage API independently refuses a commit unless the validation flag,
sample count, finite scores, and minimum score all pass. The manifest records:

- training and held-out sample counts
- validation pass status
- lowest observed and required held-out similarity
- five-pose coverage
- model and preprocessing identity
- UID, username, timestamps, and encrypted template path
- privacy and enrollment status flags

The semantic validator rejects a manifest that claims real enrollment without
held-out evidence and complete pose coverage.

## Privacy boundary

The manifest may contain model IDs, dimensions, counts, quality scores, pose
coverage, file paths, and status flags. It must not contain raw frames, face
crops, embeddings, decrypted profile bytes, or encryption keys.

The encrypted profile contains normalized pose centroids. It remains biometric
data and must be protected and deletable even though it is encrypted.

## Placeholder compatibility

The template CLI can still create a placeholder manifest for crypto and status
testing. Placeholder manifests use zero sample counts, null validation scores,
and false completion flags. They cannot be interpreted as real enrollment.

## Remaining work

- replace the local development key file with reviewed key management
- calibrate enrollment consistency and authentication thresholds on consented data
- add migration policy before changing the binary payload version
- validate upgrade, backup, deletion, and corrupted-file behavior
