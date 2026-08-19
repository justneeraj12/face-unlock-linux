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
