# GitHub Labels

This document describes project issue labels.

## Bootstrap script

Script:

    scripts/bootstrap-labels.sh

Default repo:

    justneeraj12/face-unlock-linux

Run:

    ./scripts/bootstrap-labels.sh

Or specify repo:

    ./scripts/bootstrap-labels.sh owner/repo

## Requirements

The script requires GitHub CLI:

    gh

Install:

    sudo apt install gh

Authenticate:

    gh auth login

## Safety

The script only creates or updates GitHub labels.

It does not modify:

- PAM files
- sudo configuration
- systemd services
- local authentication settings

## Label groups

Area labels:

- area/daemon
- area/pam
- area/gui
- area/python
- area/models
- area/packaging
- area/docs
- area/ci

Security/privacy labels:

- security
- privacy
- pam-safety

Workflow labels:

- needs-review
- blocked
- testing
- release
- good first issue
- help wanted

Model/research labels:

- model
- research

Standard labels:

- bug
- enhancement
