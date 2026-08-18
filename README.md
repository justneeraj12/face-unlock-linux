# face-unlock-linux

> Experimental open-source face recognition unlock utility for Ubuntu Linux.

`face-unlock-linux` aims to provide a secure, low-latency, user-controlled face unlock system for Ubuntu 24.04 and compatible Linux desktops.

The long-term goal is to support:

- lock-screen authentication
- `sudo` authentication
- desktop session unlock
- optional greeter/login support
- X11 and Wayland environments
- encrypted local face templates
- user consent, enrollment, and deletion controls

This project is currently in early development.

## Project status

**Status:** design and prototype phase.

Do **not** use this project yet as your only authentication method.

The first stable milestones are:

1. documented architecture and threat model
2. minimal user daemon
3. minimal PAM bridge
4. safe local testing flow
5. encrypted enrollment templates
6. Qt enrollment GUI
7. packaging for Ubuntu 24.04

## Target platform

Primary target:

- Ubuntu 24.04 LTS
- Intel x86_64 laptops/desktops
- internal webcam or USB webcam
- GNOME, KDE Plasma, or LightDM-based desktops

Development target machine:

- MSI GF66
- Intel i5-12500H
- NVIDIA RTX 3050 Ti
- 16 GB RAM
- Ubuntu 24.04 with OEM kernel
- working NVIDIA drivers and CUDA

## Planned architecture

The project is split into small components:

```text
+--------------------+
| PAM module         |
| tiny C IPC client  |
+---------+----------+
          |
          | UNIX socket
          v
+--------------------+
| user daemon        |
| C++17              |
| OpenCV + Torch     |
+---------+----------+
          |
          v
+--------------------+
| camera + model     |
| local only         |
+--------------------+

+--------------------+
| enrollment GUI     |
| Qt6                |
+--------------------+
