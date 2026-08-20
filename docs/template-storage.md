# Template Storage

This document describes the encrypted template storage scaffold.

## Current status

The project now includes a libsodium-based encryption utility scaffold.

Current implementation:

- initializes libsodium
- generates a random symmetric key for self-test
- encrypts placeholder template bytes with crypto_secretbox
- writes encrypted blob to disk with mode 0600
- reads encrypted blob back
- decrypts and verifies the plaintext
- removes the temporary self-test file

This is not real biometric enrollment yet.

## Self-test

Build:

    ./scripts/build.sh

Run:

    ./build/daemon/face-unlock-crypto-selftest

Expected important output:

    crypto_status: initialized
    encrypt_status: ok
    write_status: ok
    read_status: ok
    decrypt_status: ok
    cleanup_status: ok
    status: ok

## Planned template path

Default future template path:

    ~/.local/share/face-unlock/template.enc

## Current encryption design

The scaffold uses:

    libsodium crypto_secretbox

The encrypted blob currently contains:

- format magic
- random nonce
- ciphertext with authentication tag

## Key management

Current self-test key management:

- random key generated in memory
- key is not saved
- key is discarded when process exits

Future key management options:

- kernel keyring
- GNOME Keyring
- user-provided passphrase wrapping
- hardware-backed secret storage when available

## Security requirements

Template files must:

- be encrypted at rest
- be readable only by the owning user
- avoid storing raw face images
- avoid logging embeddings or template contents
- be removable by the user

## Privacy

The current scaffold uses placeholder bytes only.

It does not store real face templates, embeddings, face crops, or raw images.

## Template CLI scaffold

The encrypted placeholder template CLI is documented in:

    docs/template-cli.md

Tool:

    ./build/daemon/face-unlock-template-tool

This currently creates encrypted placeholder data only.

It does not create real biometric templates.

## Key management

Key management is documented in:

    docs/key-management.md

Production key management is not implemented yet.
