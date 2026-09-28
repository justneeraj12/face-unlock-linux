# Milestones

The [roadmap](../ROADMAP.md) is the canonical development sequence. This file
summarizes release-shaped checkpoints.

## Released

### v0.1.0-alpha

Initial daemon, camera, socket, PAM client, crypto scaffold, packaging, and CI.

### v0.1.1-alpha

Guarded sudo development tooling, key/template metadata, GUI scaffolds, model
planning, and expanded safety tests.

## Active v0.2 development

CPU recognition and enrollment infrastructure:

- C++ YuNet detector
- C++ SFace embedding
- versioned encrypted face profile
- daemon enrollment operations
- quality and held-out validation
- CPU latency, memory, and thermal benchmarks

Real authentication remains fail-closed until matching and security gates are
complete.

## Future checkpoints

### Enrollment GUI preview

- live camera preview
- guided head movement
- real pose and quality progress
- encrypted profile commit
- complete Forget Me flow

### Authentication hardening preview

- calibrated thresholds
- presentation-attack evaluation
- bounded retries and cooldown
- corrupted-profile and model handling
- reviewed key storage

### Daily-use packaging preview

- reviewed model redistribution
- one-command package installation
- first-run GUI
- user service setup
- explicit reversible PAM opt-in
- Intel and AMD compatibility matrix

### v1.0 security review

Requires measured accuracy limits, liveness disclosure, reproducible packages,
safe fallback and rollback, audited IPC/PAM/crypto boundaries, and external
security review.
