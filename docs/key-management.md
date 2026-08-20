# Key Management

This document describes key management plans and current development scaffolding.

## Current status

Production key management is not implemented yet.

The project includes a development key tool:

    ./build/daemon/face-unlock-key-tool

This tool can create a raw local development key file:

    ~/.local/share/face-unlock/template.key

This is only a scaffold.

## Development key warning

A raw symmetric key stored in the user's home directory is not final production key management.

The development key tool requires:

    --i-understand-dev-key-risk

before creating a key.

The tool never prints key contents.

## Commands

Check status:

    ./build/daemon/face-unlock-key-tool status

Create development key:

    ./build/daemon/face-unlock-key-tool create-dev-key --i-understand-dev-key-risk

Overwrite development key:

    ./build/daemon/face-unlock-key-tool create-dev-key --i-understand-dev-key-risk --overwrite

Delete development key:

    ./build/daemon/face-unlock-key-tool delete --yes

## File permissions

The development key file is written with mode:

    0600

The parent directory is restricted to the owner.

## Git policy

Key files must not be committed.

The repository ignores:

    *.key
    *.keyring

## Future production key options

Future production key management should evaluate:

- GNOME Keyring
- Secret Service API
- kernel keyring
- passphrase-wrapped local key
- TPM or hardware-backed storage where available

## Requirements for production

Production key management must:

- avoid logging keys
- avoid printing keys
- support deletion
- integrate with Forget Me
- document recovery behavior
- fail closed if key is unavailable
- avoid requiring root privileges

## Relationship to templates

Future real encrypted templates should be decryptable only with the user's key.

Current placeholder templates may still use random/discarded keys or development-key scaffolding depending on the test path.

Real biometric enrollment is still not implemented.

## Template tool integration

The placeholder template tool can use the development key with:

    ./build/daemon/face-unlock-template-tool create-placeholder --i-understand-placeholder --use-dev-key

This requires a development key to exist:

    ./build/daemon/face-unlock-key-tool create-dev-key --i-understand-dev-key-risk

Verify decryptability without printing plaintext:

    ./build/daemon/face-unlock-template-tool verify-decrypt --use-dev-key

This is still not production key management.
