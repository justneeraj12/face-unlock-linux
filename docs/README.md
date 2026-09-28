# Documentation

The root [README](../README.md) is the project overview. This index routes
developers to the detailed design and safety documents.

## Start here

| Topic | Document |
|---|---|
| Current implementation | [Project status](project-status.md) |
| Planned work | [Roadmap](../ROADMAP.md) |
| System design | [Architecture](architecture.md) |
| Local development | [Development setup](development-setup.md) |
| Build and tests | [Testing](testing.md) |
| Full verification | [Local verification](local-verification.md) |

## CPU models and enrollment

| Topic | Document |
|---|---|
| CPU YuNet/SFace profile | [CPU face profile](cpu-face-profile.md) |
| C++ detector runtime | [Daemon detector backends](daemon-detector-scaffold.md) |
| Python detector prototype | [Detector prototype](detector-prototype.md) |
| Model candidates and licenses | [Model candidates](model-candidates.md) |
| Evaluation plan | [Model evaluation plan](model-evaluation-plan.md) |
| Threshold calibration | [Threshold calibration](threshold-calibration.md) |
| Enrollment metadata | [Enrollment format](enrollment-format.md) |
| Detector metadata | [Detector output format](detector-output-format.md) |

## Security, PAM, and sudo

| Topic | Document |
|---|---|
| Security policy | [SECURITY.md](../SECURITY.md) |
| Threat model | [Threat model](threat-model.md) |
| Lock-screen policy | [Lock-screen authentication](lock-screen-auth.md) |
| PAM rules | [PAM safety](pam-safety.md) |
| Fake PAM test | [Fake service test](pam-fake-service-test.md) |
| Root peer policy | [sudo root peer policy](sudo-root-peer-policy.md) |
| Guarded apply and recovery | [sudo apply and rollback](sudo-apply-and-rollback.md) |
| Dependency boundary | [Dependency audit](dependency-audit.md) |

Real PAM service files must not be modified casually. Development auth is not
real biometric authentication.

## Daemon, storage, and GUI

| Topic | Document |
|---|---|
| Per-user configuration | [Configuration](configuration.md) |
| User service | [systemd user service](systemd-user-service.md) |
| Template storage | [Template storage](template-storage.md) |
| Development key handling | [Key management](key-management.md) |
| Template CLI | [Template CLI](template-cli.md) |
| Qt GUI | [GUI](gui.md) |
| Camera preview design | [GUI camera preview](gui-camera-preview.md) |
| Brightness assistance | [Brightness assist](brightness-assist.md) |

## Build, CI, packaging, and releases

| Topic | Document |
|---|---|
| Build performance | [Fast builds](fast-builds.md) |
| CI design | [CI](ci.md) |
| CI packages | [CI dependencies](ci-dependencies.md) |
| Packaging | [Packaging](packaging.md) |
| Release process | [Release process](release-process.md) |
| Release artifacts | [Release artifacts](release-artifacts.md) |
| Changelog | [CHANGELOG.md](../CHANGELOG.md) |

Historical release notes live under releases/.

## Contribution workflow

| Topic | Document |
|---|---|
| Contribution rules | [CONTRIBUTING.md](../CONTRIBUTING.md) |
| Security review checklist | [Pull request review](pull-request-review.md) |
| Code ownership | [CODEOWNERS](codeowners.md) |
| Issue labels | [GitHub labels](github-labels.md) |
| Milestones | [Milestones](milestones.md) |
