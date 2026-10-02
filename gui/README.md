# Enrollment GUI

`gui/` contains the optional Qt6 enrollment client.

## Implemented

The GUI now drives the daemon-owned native enrollment flow:

- explicit local biometric-processing consent
- automatic qualified-frame capture every 300 ms
- center, left, right, up, and down guidance
- real training and held-out validation progress
- last-frame quality and inference latency feedback
- cancel with in-memory sample erasure
- separate confirmation before encrypted profile commit
- Forget Me deletion of the profile, manifest, and local key
- a dot-matrix scanner that grows with enrollment progress

Camera frames remain inside the daemon. The GUI receives status and metrics,
not images. Live preview is still pending.

The scanner precomputes its face point cloud, batches point draws by depth, and
uses a coarse 20 FPS timer only while collecting or validating. It stops while
idle or hidden and requires no GPU API. Disable motion with
`FACE_UNLOCK_REDUCE_MOTION=1`.

Authentication acceptance, liveness protection, PAM installation, and desktop
unlock integration remain disabled.

## Run

Download the pinned CPU models, build the daemon and GUI, then start the daemon:

    ./scripts/download-cpu-models.sh
    ./scripts/build.sh
    ./scripts/build-gui.sh
    ./build/daemon/face-unlockd \
      --camera 0 \
      --detector yunet \
      --detector-model models/face_detection_yunet.onnx \
      --recognizer-model models/face_recognition_sface_2021dec.onnx \
      --daemon

In another terminal:

    ./build-gui/gui/face-unlock-enroll

## Test

    ./build-gui/gui/face-unlock-enroll --self-test-enrollment-json
    env -u QT_QPA_PLATFORMTHEME QT_QPA_PLATFORM=offscreen QT_STYLE_OVERRIDE=Fusion \
      ./build-gui/gui/face-unlock-enroll --self-test-scanner
    env -u QT_QPA_PLATFORMTHEME QT_QPA_PLATFORM=offscreen QT_STYLE_OVERRIDE=Fusion \
      ./build-gui/gui/face-unlock-enroll --render-scanner scanner.png
    env -u QT_QPA_PLATFORMTHEME QT_QPA_PLATFORM=offscreen QT_STYLE_OVERRIDE=Fusion \
      ./build-gui/gui/face-unlock-enroll --benchmark-scanner 300

The parser and offscreen scanner tests are registered with CTest when
`BUILD_GUI=ON`.

## Safety

The GUI never edits PAM configuration. Enrollment and deletion require explicit
user actions. Closing the GUI during automatic capture cancels the daemon
session and releases the camera.
