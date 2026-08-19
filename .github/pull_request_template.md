## Summary

Describe the change.

## Motivation

Why is this change needed?

## Area

- [ ] documentation
- [ ] daemon
- [ ] PAM
- [ ] GUI
- [ ] Python prototypes
- [ ] model/export tooling
- [ ] template encryption/storage
- [ ] packaging
- [ ] CI
- [ ] tests
- [ ] security
- [ ] other

## Security impact

Does this affect authentication, templates, IPC, PAM, camera access, model loading, encryption, installation, or system configuration?

- [ ] no security impact
- [ ] security impact described below

Details:

## PAM impact

- [ ] does not affect PAM
- [ ] affects PAM module code
- [ ] affects fake PAM test only
- [ ] affects sudo PAM planning
- [ ] affects sudo PAM apply/rollback
- [ ] affects login/greeter/lock-screen documentation
- [ ] affects real PAM service files

If PAM behavior changes, explain fail-closed behavior and password fallback:

## Biometric/privacy impact

- [ ] no biometric data involved
- [ ] reads camera frames only in memory
- [ ] may save face crops
- [ ] may save embeddings
- [ ] may save encrypted templates
- [ ] modifies enrollment manifest behavior
- [ ] modifies deletion or Forget Me behavior

If any biometric data can be saved, explain consent and Git ignore behavior:

## Model impact

- [ ] no model changes
- [ ] changes TorchScript export stub
- [ ] changes optional LibTorch loader
- [ ] proposes a real detector model
- [ ] proposes a real embedding model
- [ ] changes preprocessing/alignment
- [ ] changes threshold/calibration docs

If a real model is involved, include:

- source URL:
- license:
- redistribution rights:
- input shape:
- output shape:
- preprocessing:
- threshold/calibration notes:

## Installer/system impact

- [ ] no installer/system changes
- [ ] changes user service helper
- [ ] changes fake PAM installer/remover
- [ ] changes Debian package
- [ ] changes sudo dry-run planner
- [ ] changes guarded sudo apply script
- [ ] changes rollback behavior

If system files can be modified, list exact paths and rollback command:

## Testing

Commands run:

    ./scripts/check-docs.sh
    ./scripts/check-json.sh
    ./scripts/build.sh
    ./scripts/test.sh

Additional tests:

## Manual verification

If relevant, describe manual testing for:

- [ ] camera probe
- [ ] daemon socket
- [ ] PAM fake service
- [ ] sudo dry-run
- [ ] sudo apply/rollback
- [ ] GUI build
- [ ] Python prototype
- [ ] package build

## Checklist

- [ ] No raw biometric data is committed.
- [ ] No face crops are committed.
- [ ] No embeddings are committed.
- [ ] No encrypted templates are committed.
- [ ] No model artifacts are committed unless explicitly allowed.
- [ ] No secrets or keys are committed.
- [ ] PAM behavior remains fail-closed.
- [ ] Password fallback remains available.
- [ ] Rollback instructions are included for system changes.
- [ ] Model licenses are documented if model behavior changes.
- [ ] Documentation was updated if behavior changed.
- [ ] Tests or manual verification steps are included.
