## Summary

Describe the change.

## Area

- [ ] documentation
- [ ] daemon
- [ ] PAM
- [ ] GUI
- [ ] packaging
- [ ] tests
- [ ] security
- [ ] other

## Security impact

Does this affect authentication, templates, IPC, PAM, camera access, or installation?

- [ ] no security impact
- [ ] security impact described below

Details:

## Testing

What did you test?

Example:

    ./scripts/build.sh
    ./scripts/check-docs.sh

## Checklist

- [ ] No raw biometric data is committed.
- [ ] No secrets or keys are committed.
- [ ] PAM behavior remains fail-closed.
- [ ] Password fallback remains available.
- [ ] Rollback instructions are included for system changes.
- [ ] Documentation was updated if behavior changed.
