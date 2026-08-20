# Template CLI Scaffold

This document describes the encrypted placeholder template CLI.

## Tool

Build target:

    face-unlock-template-tool

Local path after build:

    ./build/daemon/face-unlock-template-tool

## Purpose

The tool manages an encrypted placeholder template file.

This is not real biometric enrollment yet.

It exists to test:

- encrypted-at-rest file creation
- template file path
- 0600 file permissions
- daemon template status metadata
- safe delete behavior

## Template path

Default path:

    ~/.local/share/face-unlock/template.enc

## Check status

Run:

    ./build/daemon/face-unlock-template-tool status

Possible status values:

    template_status: missing
    template_status: present

## Create placeholder

Create encrypted placeholder:

    ./build/daemon/face-unlock-template-tool create-placeholder --i-understand-placeholder

Overwrite existing placeholder:

    ./build/daemon/face-unlock-template-tool create-placeholder --i-understand-placeholder --overwrite

Important:

- this does not create a real biometric template
- the placeholder is encrypted with a random key
- the random key is intentionally discarded
- the placeholder is not decryptable later
- this only tests encrypted file creation and daemon template detection

## Delete template

Delete:

    ./build/daemon/face-unlock-template-tool delete --yes

## Daemon metadata

Daemon socket responses include template status metadata:

    "template":"present"

or:

    "template":"missing"

Auth still fails closed by default.

## Safety

The tool does not store raw images.

The placeholder contains no biometric data.

Real enrollment and matching are not implemented yet.

## Auth reason behavior

When no template exists, auth responses should include:

    reason template_missing

When a placeholder template exists, auth responses should include:

    reason matcher_not_implemented

This still does not mean real matching is implemented.

## Enrollment manifest relationship

Future enrollment will create both:

    ~/.local/share/face-unlock/template.enc
    ~/.local/share/face-unlock/enrollment.json

The planned manifest format is documented in:

    docs/enrollment-format.md

Current placeholder template creation does not yet write an enrollment manifest.

## Placeholder enrollment manifest creation

create-placeholder now writes both:

    ~/.local/share/face-unlock/template.enc
    ~/.local/share/face-unlock/enrollment.json

The manifest is placeholder-only metadata.

It records:

- format version
- user UID and username
- placeholder model metadata
- encrypted template path
- privacy flags
- placeholder status

Both files are written with mode 0600.

delete --yes removes both the encrypted placeholder template and the placeholder enrollment manifest.

## Daemon status integration

After create-placeholder, daemon socket responses should include:

    "template":"present"
    "enrollment":"placeholder"

After delete --yes, daemon socket responses should include:

    "template":"missing"
    "enrollment":"missing"

## Development key option

create-placeholder can use the local development key:

    ./build/daemon/face-unlock-template-tool create-placeholder --i-understand-placeholder --use-dev-key

Verify decryptability:

    ./build/daemon/face-unlock-template-tool verify-decrypt --use-dev-key

If --use-dev-key is not provided, the placeholder is encrypted with a random key that is discarded.

Development key management is documented in:

    docs/key-management.md

## Integration self-test

The key/template placeholder flow is tested by:

    ./scripts/test-key-template-flow.sh

This test uses a temporary HOME directory and is safe to run locally and in CI.

## Decryptability status

The status command reports decryptability metadata without decrypting:

    decrypt_status: possible_with_dev_key
    decrypt_status: key_missing
    decrypt_status: not_possible_discarded_key
    decrypt_status: template_missing
    decrypt_status: unknown

To actually verify decryptability, explicitly run:

    ./build/daemon/face-unlock-template-tool verify-decrypt --use-dev-key

The status command does not print plaintext.

## Daemon metadata relationship

The daemon reports key and decryptability metadata based on the same template/key paths used by the template and key tools.

This is metadata only.

The daemon does not decrypt templates yet.
