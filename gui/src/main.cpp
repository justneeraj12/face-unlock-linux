#include "daemon_client.h"

#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <string>

namespace {

using face_unlock_gui::EnrollmentSnapshot;
using face_unlock_gui::detectorStatusSummary;
using face_unlock_gui::friendlyReason;
using face_unlock_gui::parseEnrollmentResponse;
using face_unlock_gui::poseInstruction;
using face_unlock_gui::queryDaemonOperation;
using face_unlock_gui::runtimeSocketPath;
using face_unlock_gui::templateStatusSummary;

QString templatePath() {
  return QDir::homePath() +
    QStringLiteral("/.local/share/face-unlock/template.enc");
}

QString keyPath() {
  return QDir::homePath() +
    QStringLiteral("/.local/share/face-unlock/template.key");
}

QString enrollmentPath() {
  return QDir::homePath() +
    QStringLiteral("/.local/share/face-unlock/enrollment.json");
}

QString fileStatusLine(const QString& label, const QString& path) {
  const QFileInfo info(path);
  if (!info.exists()) {
    return label + QStringLiteral(": missing\n  ") + path +
      QStringLiteral("\n");
  }
  return label + QStringLiteral(": present\n  ") + path +
    QStringLiteral("\n  size: ") + QString::number(info.size()) +
    QStringLiteral(" bytes\n");
}

QString statusText() {
  QString text;
  text += QStringLiteral("Current GUI status:\n\n");
  text += QStringLiteral("- Native daemon enrollment controls enabled\n");
  text += QStringLiteral("- Automatic qualified-sample collection enabled\n");
  text += QStringLiteral("- Five-pose progress and quality feedback enabled\n");
  text += QStringLiteral("- Encrypted profile commit requires explicit action\n");
  text += QStringLiteral("- Camera frames remain inside the daemon\n");
  text += QStringLiteral("- Live preview is not connected yet\n");
  text += QStringLiteral("- Authentication acceptance remains disabled\n");
  text += QStringLiteral("- No PAM changes are made by this GUI\n\n");
  text += QStringLiteral("User data status:\n\n");
  text += fileStatusLine(QStringLiteral("Encrypted template"), templatePath());
  text += fileStatusLine(QStringLiteral("Template key"), keyPath());
  text += fileStatusLine(
    QStringLiteral("Enrollment manifest"),
    enrollmentPath()
  );
  return text;
}

QString consentText() {
  return QStringLiteral(
    "face-unlock-linux is experimental biometric software.\n"
    "\n"
    "Before enrollment, understand that:\n"
    "\n"
    "1. Face embeddings are biometric data.\n"
    "2. Camera frames stay in daemon memory and are not saved by this flow.\n"
    "3. The generated profile is encrypted at rest.\n"
    "4. The current key is a local development key file.\n"
    "5. Password fallback must remain available.\n"
    "6. RGB camera enrollment is not liveness protection.\n"
    "7. Authentication acceptance is still disabled.\n"
    "8. Forget Me deletes the profile, manifest, and local key.\n"
    "\n"
    "Starting enrollment explicitly consents to local camera processing and "
    "in-memory face embedding generation. Saving the encrypted profile requires "
    "a separate confirmation."
  );
}

struct EnrollmentUi {
  QCheckBox* consent = nullptr;
  QProgressBar* progress = nullptr;
  QLabel* state = nullptr;
  QLabel* guidance = nullptr;
  QLabel* metrics = nullptr;
  QLabel* previewText = nullptr;
  QTextEdit* response = nullptr;
  QCheckBox* center = nullptr;
  QCheckBox* left = nullptr;
  QCheckBox* right = nullptr;
  QCheckBox* up = nullptr;
  QCheckBox* down = nullptr;
  QCheckBox* lighting = nullptr;
  QCheckBox* sharp = nullptr;
  QCheckBox* face = nullptr;
  QCheckBox* coverage = nullptr;
  QCheckBox* saved = nullptr;
  QPushButton* start = nullptr;
  QPushButton* capture = nullptr;
  QPushButton* refresh = nullptr;
  QPushButton* cancel = nullptr;
  QPushButton* commit = nullptr;
};

bool poseComplete(
  const EnrollmentSnapshot& snapshot,
  const QString& pose
) {
  const bool sessionHasCoverage =
    snapshot.state == QStringLiteral("collecting") ||
    snapshot.state == QStringLiteral("ready") ||
    snapshot.state == QStringLiteral("committed");
  return sessionHasCoverage && !snapshot.missingPoses.contains(pose);
}

void applyEnrollmentSnapshot(
  const EnrollmentSnapshot& snapshot,
  const EnrollmentUi& ui
) {
  ui.response->setPlainText(
    QStringLiteral("Socket: %1\n\n%2")
      .arg(runtimeSocketPath(), snapshot.raw)
  );
  ui.progress->setValue(snapshot.progress);
  ui.state->setText(
    QStringLiteral("State: %1 - Samples: %2 - %3")
      .arg(
        snapshot.state,
        QString::number(snapshot.acceptedSamples),
        friendlyReason(snapshot.reason)
      )
  );

  QString guidance = friendlyReason(snapshot.reason);
  if (snapshot.state == QStringLiteral("collecting") &&
      !snapshot.missingPoses.isEmpty()) {
    guidance = poseInstruction(snapshot.missingPoses.front()) +
      QStringLiteral("\n") + guidance;
  } else if (snapshot.state == QStringLiteral("ready")) {
    guidance = QStringLiteral(
      "All five poses are covered. The camera is closed. "
      "Save the encrypted profile to finish."
    );
  } else if (snapshot.state == QStringLiteral("committed")) {
    guidance = QStringLiteral(
      "Enrollment is saved. Authentication remains disabled until "
      "verification is calibrated and reviewed."
    );
  }
  ui.guidance->setText(guidance);

  ui.center->setChecked(
    poseComplete(snapshot, QStringLiteral("center"))
  );
  ui.left->setChecked(
    poseComplete(snapshot, QStringLiteral("left"))
  );
  ui.right->setChecked(
    poseComplete(snapshot, QStringLiteral("right"))
  );
  ui.up->setChecked(
    poseComplete(snapshot, QStringLiteral("up"))
  );
  ui.down->setChecked(
    poseComplete(snapshot, QStringLiteral("down"))
  );

  const bool qualityApproved =
    snapshot.qualityReason == QStringLiteral("approved");
  ui.lighting->setChecked(qualityApproved);
  ui.sharp->setChecked(qualityApproved);
  ui.face->setChecked(qualityApproved);
  ui.coverage->setChecked(
    snapshot.state == QStringLiteral("ready") ||
    snapshot.state == QStringLiteral("committed")
  );
  ui.saved->setChecked(
    snapshot.state == QStringLiteral("committed")
  );

  ui.metrics->setText(
    QStringLiteral(
      "Last frame: faces %1 - pose %2 - quality %3 - "
      "luma %4 - sharpness %5 - detector %6 ms - embedding %7 ms"
    ).arg(
      QString::number(snapshot.facesDetected),
      snapshot.pose,
      snapshot.qualityReason,
      QString::number(snapshot.meanLuma, 'f', 1),
      QString::number(snapshot.sharpness, 'f', 1),
      QString::number(snapshot.detectorMs, 'f', 1),
      QString::number(snapshot.embeddingMs, 'f', 1)
    )
  );

  const bool collecting = snapshot.state == QStringLiteral("collecting");
  const bool ready = snapshot.state == QStringLiteral("ready");
  ui.start->setEnabled(
    ui.consent->isChecked() && !collecting && !ready
  );
  ui.capture->setEnabled(collecting);
  ui.cancel->setEnabled(collecting || ready);
  ui.commit->setEnabled(ready);

  if (collecting) {
    ui.previewText->setText(QStringLiteral(
      "The daemon camera is active.\n"
      "Follow the pose guidance below.\n"
      "Frames stay inside the daemon and are not saved."
    ));
  } else {
    ui.previewText->setText(QStringLiteral(
      "The GUI does not receive camera frames.\n"
      "The daemon opens the camera only during enrollment.\n"
      "No raw images are saved."
    ));
  }
}

void refreshStatus(QTextEdit* status) {
  status->setPlainText(statusText());
}

void refreshDaemonPanels(
  QTextEdit* daemonSummary,
  QTextEdit* templateSummary,
  QTextEdit* authSummary,
  QTextEdit* daemonResponse
) {
  const QString detectorResponse =
    queryDaemonOperation(QStringLiteral("detector_status"));
  const QString templateResponse =
    queryDaemonOperation(QStringLiteral("template_status"));

  daemonSummary->setPlainText(detectorStatusSummary(detectorResponse));
  templateSummary->setPlainText(templateStatusSummary(templateResponse));
  authSummary->setPlainText(QStringLiteral(
    "Auth is not queried automatically.\n"
    "An auth request consumes a retry and cannot currently succeed."
  ));
  daemonResponse->setPlainText(
    QStringLiteral(
      "Socket: %1\n\n"
      "detector_status:\n%2\n\n"
      "template_status:\n%3"
    ).arg(runtimeSocketPath(), detectorResponse, templateResponse)
  );
}

bool removeIfExists(
  const QString& path,
  QStringList& removed,
  QStringList& failed
) {
  QFile file(path);
  if (!file.exists()) return true;
  if (file.remove()) {
    removed << path;
    return true;
  }
  failed << path;
  return false;
}

void forgetMe(QWidget* parent, QTextEdit* status) {
  const QString message = QStringLiteral(
    "This deletes the encrypted face profile, enrollment manifest, and "
    "local development key:\n\n%1\n%2\n%3\n\n"
    "A deleted local key cannot be recovered. This does not modify PAM, "
    "sudo, login, or lock-screen settings.\n\nContinue?"
  ).arg(templatePath(), enrollmentPath(), keyPath());

  const QMessageBox::StandardButton answer = QMessageBox::warning(
    parent,
    QStringLiteral("Confirm Forget Me"),
    message,
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No
  );
  if (answer != QMessageBox::Yes) return;

  (void)queryDaemonOperation(QStringLiteral("enrollment_cancel"));

  QStringList removed;
  QStringList failed;
  removeIfExists(templatePath(), removed, failed);
  removeIfExists(enrollmentPath(), removed, failed);
  removeIfExists(keyPath(), removed, failed);
  refreshStatus(status);

  if (!failed.isEmpty()) {
    QMessageBox::critical(
      parent,
      QStringLiteral("Forget Me failed"),
      QStringLiteral("Failed to remove:\n\n") +
        failed.join(QStringLiteral("\n"))
    );
    return;
  }

  const QString result = removed.isEmpty()
    ? QStringLiteral("No enrollment files were present.")
    : QStringLiteral("Removed:\n\n") + removed.join(QStringLiteral("\n"));
  QMessageBox::information(
    parent,
    QStringLiteral("Forget Me complete"),
    result
  );
}

QTextEdit* readOnlyTextEdit(
  const QString& text,
  int minimumHeight = 120
) {
  auto* edit = new QTextEdit();
  edit->setReadOnly(true);
  edit->setPlainText(text);
  edit->setMinimumHeight(minimumHeight);
  return edit;
}

QWidget* scrollable(QWidget* content) {
  auto* area = new QScrollArea();
  area->setWidgetResizable(true);
  area->setWidget(content);
  return area;
}



}  // namespace

int main(int argc, char* argv[]) {
  if (argc == 2 &&
      std::string(argv[1]) == "--self-test-enrollment-json") {
    QCoreApplication application(argc, argv);
    return face_unlock_gui::enrollmentParserSelfTest();
  }

  QApplication app(argc, argv);
  QWidget window;
  window.setWindowTitle(QStringLiteral("face-unlock-linux Enrollment"));
  auto* root = new QVBoxLayout(&window);

  auto* title = new QLabel(QStringLiteral("face-unlock-linux"));
  QFont titleFont = title->font();
  titleFont.setPointSize(20);
  titleFont.setBold(true);
  title->setFont(titleFont);

  auto* subtitle = new QLabel(QStringLiteral("Native CPU enrollment client"));
  auto* warning = new QLabel(QStringLiteral(
    "Enrollment is implemented. Authentication acceptance and liveness "
    "protection are not."
  ));
  warning->setWordWrap(true);
  warning->setStyleSheet(
    QStringLiteral("color: #b00020; font-weight: bold;")
  );

  root->addWidget(title);
  root->addWidget(subtitle);
  root->addWidget(warning);

  auto* tabs = new QTabWidget();
  root->addWidget(tabs);

  auto* statusTab = new QWidget();
  auto* statusLayout = new QVBoxLayout(statusTab);
  auto* status = readOnlyTextEdit(statusText(), 190);
  auto* daemonSummary = readOnlyTextEdit(
    QStringLiteral("No detector_status query yet."), 125
  );
  auto* templateSummary = readOnlyTextEdit(
    QStringLiteral("No template_status query yet."), 165
  );
  auto* authSummary = readOnlyTextEdit(
    QStringLiteral(
      "Auth is not queried automatically because it consumes retries."
    ), 85
  );
  auto* daemonResponse = readOnlyTextEdit(
    QStringLiteral("Socket: %1\n\nNo daemon query yet.")
      .arg(runtimeSocketPath()), 140
  );

  auto* statusButtons = new QHBoxLayout();
  auto* refreshButton = new QPushButton(QStringLiteral("Refresh local status"));
  auto* refreshDaemonButton =
    new QPushButton(QStringLiteral("Refresh daemon panels"));
  auto* detectorButton =
    new QPushButton(QStringLiteral("Query detector_status"));
  auto* templateButton =
    new QPushButton(QStringLiteral("Query template_status"));
  statusButtons->addWidget(refreshButton);
  statusButtons->addWidget(refreshDaemonButton);
  statusButtons->addWidget(detectorButton);
  statusButtons->addWidget(templateButton);
  statusButtons->addStretch();

  auto addSection = [statusLayout](const QString& name, QWidget* widget) {
    auto* label = new QLabel(name);
    QFont font = label->font();
    font.setBold(true);
    label->setFont(font);
    statusLayout->addWidget(label);
    statusLayout->addWidget(widget);
  };

  statusLayout->addWidget(status);
  statusLayout->addLayout(statusButtons);
  addSection(QStringLiteral("Daemon detector summary"), daemonSummary);
  addSection(QStringLiteral("Template status summary"), templateSummary);
  addSection(QStringLiteral("Auth safety status"), authSummary);
  addSection(QStringLiteral("Raw daemon response"), daemonResponse);
  statusLayout->addStretch();
  tabs->addTab(scrollable(statusTab), QStringLiteral("Status"));

  auto* enrollmentTab = new QWidget();
  auto* enrollmentLayout = new QVBoxLayout(enrollmentTab);
  auto* enrollmentConsent = new QCheckBox(QStringLiteral(
    "I consent to local camera processing and in-memory face embeddings."
  ));
  enrollmentConsent->setToolTip(QStringLiteral(
    "Saving the encrypted profile requires a second confirmation."
  ));

  auto* previewFrame = new QFrame();
  previewFrame->setFrameShape(QFrame::StyledPanel);
  previewFrame->setMinimumHeight(145);
  previewFrame->setStyleSheet(QStringLiteral(
    "QFrame { background-color: #202124; border: 1px solid #555; "
    "border-radius: 6px; } QLabel { color: #eeeeee; }"
  ));
  auto* previewLayout = new QVBoxLayout(previewFrame);
  auto* previewTitle = new QLabel(QStringLiteral("Privacy-safe camera session"));
  QFont previewFont = previewTitle->font();
  previewFont.setBold(true);
  previewFont.setPointSize(13);
  previewTitle->setFont(previewFont);
  auto* previewText = new QLabel(QStringLiteral(
    "The GUI does not receive camera frames.\n"
    "The daemon opens the camera only during enrollment.\n"
    "No raw images are saved."
  ));
  previewText->setAlignment(Qt::AlignCenter);
  previewText->setWordWrap(true);
  previewLayout->addWidget(previewTitle);
  previewLayout->addStretch();
  previewLayout->addWidget(previewText);
  previewLayout->addStretch();

  auto* enrollmentProgress = new QProgressBar();
  enrollmentProgress->setRange(0, 100);
  enrollmentProgress->setValue(0);
  enrollmentProgress->setFormat(QStringLiteral("Enrollment profile: %p%"));
  auto* enrollmentState = new QLabel(QStringLiteral("State: idle"));
  auto* guidance = new QLabel(QStringLiteral(
    "Start the daemon with CPU YuNet and SFace, then begin enrollment."
  ));
  guidance->setWordWrap(true);
  QFont guidanceFont = guidance->font();
  guidanceFont.setPointSize(12);
  guidanceFont.setBold(true);
  guidance->setFont(guidanceFont);

  auto* enrollmentButtons = new QHBoxLayout();
  auto* startEnrollment = new QPushButton(QStringLiteral("Start enrollment"));
  auto* captureSample = new QPushButton(QStringLiteral("Capture now"));
  auto* refreshEnrollment = new QPushButton(QStringLiteral("Refresh progress"));
  auto* cancelEnrollment =
    new QPushButton(QStringLiteral("Cancel and erase samples"));
  auto* commitEnrollment =
    new QPushButton(QStringLiteral("Save encrypted profile"));
  startEnrollment->setEnabled(false);
  captureSample->setEnabled(false);
  cancelEnrollment->setEnabled(false);
  commitEnrollment->setEnabled(false);
  enrollmentButtons->addWidget(startEnrollment);
  enrollmentButtons->addWidget(captureSample);
  enrollmentButtons->addWidget(refreshEnrollment);
  enrollmentButtons->addWidget(cancelEnrollment);
  enrollmentButtons->addWidget(commitEnrollment);
  enrollmentButtons->addStretch();

  auto* poseLabel = new QLabel(QStringLiteral("Pose coverage"));
  QFont sectionFont = poseLabel->font();
  sectionFont.setBold(true);
  poseLabel->setFont(sectionFont);
  auto* poseRow = new QHBoxLayout();
  auto* centerPose = new QCheckBox(QStringLiteral("Center"));
  auto* leftPose = new QCheckBox(QStringLiteral("Left"));
  auto* rightPose = new QCheckBox(QStringLiteral("Right"));
  auto* upPose = new QCheckBox(QStringLiteral("Up"));
  auto* downPose = new QCheckBox(QStringLiteral("Down"));
  for (QCheckBox* box : {centerPose, leftPose, rightPose, upPose, downPose}) {
    box->setEnabled(false);
    poseRow->addWidget(box);
  }
  poseRow->addStretch();

  auto* qualityLabel = new QLabel(QStringLiteral("Last-frame quality"));
  qualityLabel->setFont(sectionFont);
  auto* qualityRow = new QHBoxLayout();
  auto* lightingQuality = new QCheckBox(QStringLiteral("Exposure OK"));
  auto* sharpnessQuality = new QCheckBox(QStringLiteral("Sharpness OK"));
  auto* faceQuality = new QCheckBox(QStringLiteral("Single face + size OK"));
  auto* poseQuality = new QCheckBox(QStringLiteral("All poses ready"));
  auto* templateQuality =
    new QCheckBox(QStringLiteral("Encrypted profile saved"));
  for (QCheckBox* box :
       {lightingQuality, sharpnessQuality, faceQuality,
        poseQuality, templateQuality}) {
    box->setEnabled(false);
    qualityRow->addWidget(box);
  }
  qualityRow->addStretch();

  auto* metrics = new QLabel(QStringLiteral(
    "No enrollment frame evaluated yet."
  ));
  metrics->setWordWrap(true);
  auto* enrollmentResponse = readOnlyTextEdit(
    QStringLiteral("No enrollment operation yet."), 130
  );

  enrollmentLayout->addWidget(enrollmentConsent);
  enrollmentLayout->addWidget(previewFrame);
  enrollmentLayout->addWidget(enrollmentProgress);
  enrollmentLayout->addWidget(enrollmentState);
  enrollmentLayout->addWidget(guidance);
  enrollmentLayout->addLayout(enrollmentButtons);
  enrollmentLayout->addWidget(poseLabel);
  enrollmentLayout->addLayout(poseRow);
  enrollmentLayout->addWidget(qualityLabel);
  enrollmentLayout->addLayout(qualityRow);
  enrollmentLayout->addWidget(metrics);
  enrollmentLayout->addWidget(enrollmentResponse);
  enrollmentLayout->addStretch();
  tabs->addTab(scrollable(enrollmentTab), QStringLiteral("Enrollment"));

  auto* privacyTab = new QWidget();
  auto* privacyLayout = new QVBoxLayout(privacyTab);
  auto* consent = readOnlyTextEdit(consentText(), 300);
  auto* privacyButtons = new QHBoxLayout();
  auto* understandButton = new QPushButton(QStringLiteral("I understand"));
  auto* brightnessButton =
    new QPushButton(QStringLiteral("Brightness assist information"));
  auto* forgetButton = new QPushButton(QStringLiteral("Forget me"));
  auto* closeButton = new QPushButton(QStringLiteral("Close"));
  privacyButtons->addWidget(understandButton);
  privacyButtons->addWidget(brightnessButton);
  privacyButtons->addWidget(forgetButton);
  privacyButtons->addStretch();
  privacyButtons->addWidget(closeButton);
  privacyLayout->addWidget(consent);
  privacyLayout->addLayout(privacyButtons);
  privacyLayout->addStretch();
  tabs->addTab(scrollable(privacyTab), QStringLiteral("Privacy"));

  const EnrollmentUi enrollmentUi{
    enrollmentConsent,
    enrollmentProgress,
    enrollmentState,
    guidance,
    metrics,
    previewText,
    enrollmentResponse,
    centerPose,
    leftPose,
    rightPose,
    upPose,
    downPose,
    lightingQuality,
    sharpnessQuality,
    faceQuality,
    poseQuality,
    templateQuality,
    startEnrollment,
    captureSample,
    refreshEnrollment,
    cancelEnrollment,
    commitEnrollment,
  };

  auto* captureTimer = new QTimer(&window);
  captureTimer->setInterval(300);

  auto processEnrollment = [
    enrollmentUi,
    captureTimer,
    status,
    templateSummary
  ](const QString& response) {
    const EnrollmentSnapshot snapshot = parseEnrollmentResponse(response);
    applyEnrollmentSnapshot(snapshot, enrollmentUi);

    const bool keepCapturing = snapshot.operationValid &&
      snapshot.state == QStringLiteral("collecting") &&
      snapshot.reason != QStringLiteral("enrollment_camera_failed") &&
      snapshot.reason != QStringLiteral("enrollment_camera_lease_ended");
    if (!keepCapturing) captureTimer->stop();

    if (snapshot.state == QStringLiteral("committed")) {
      refreshStatus(status);
      const QString templateResponse =
        queryDaemonOperation(QStringLiteral("template_status"));
      templateSummary->setPlainText(templateStatusSummary(templateResponse));
    }
  };

  QObject::connect(captureTimer, &QTimer::timeout, [processEnrollment]() {
    processEnrollment(
      queryDaemonOperation(QStringLiteral("enrollment_capture"))
    );
  });

  QObject::connect(startEnrollment, &QPushButton::clicked, [
    enrollmentConsent,
    captureTimer,
    processEnrollment,
    &window
  ]() {
    if (!enrollmentConsent->isChecked()) {
      QMessageBox::warning(
        &window,
        QStringLiteral("Consent required"),
        QStringLiteral(
          "Read the Privacy tab and confirm local biometric processing first."
        )
      );
      return;
    }
    const QString response =
      queryDaemonOperation(QStringLiteral("enrollment_start"));
    const EnrollmentSnapshot snapshot = parseEnrollmentResponse(response);
    processEnrollment(response);
    if (snapshot.ok && snapshot.state == QStringLiteral("collecting")) {
      captureTimer->start();
    }
  });

  QObject::connect(captureSample, &QPushButton::clicked, [processEnrollment]() {
    processEnrollment(
      queryDaemonOperation(QStringLiteral("enrollment_capture"))
    );
  });

  QObject::connect(refreshEnrollment, &QPushButton::clicked, [
    captureTimer,
    processEnrollment
  ]() {
    const QString response =
      queryDaemonOperation(QStringLiteral("enrollment_status"));
    const EnrollmentSnapshot snapshot = parseEnrollmentResponse(response);
    processEnrollment(response);
    if (snapshot.ok && snapshot.state == QStringLiteral("collecting")) {
      captureTimer->start();
    }
  });

  QObject::connect(cancelEnrollment, &QPushButton::clicked, [
    captureTimer,
    processEnrollment
  ]() {
    captureTimer->stop();
    processEnrollment(
      queryDaemonOperation(QStringLiteral("enrollment_cancel"))
    );
  });

  QObject::connect(commitEnrollment, &QPushButton::clicked, [
    processEnrollment,
    &window
  ]() {
    const QMessageBox::StandardButton answer = QMessageBox::question(
      &window,
      QStringLiteral("Save encrypted face profile"),
      QStringLiteral(
        "Save the completed biometric profile using the current local "
        "development key?\n\nAuthentication acceptance remains disabled."
      ),
      QMessageBox::Save | QMessageBox::Cancel,
      QMessageBox::Cancel
    );
    if (answer != QMessageBox::Save) return;
    processEnrollment(
      queryDaemonOperation(QStringLiteral("enrollment_commit"))
    );
  });

  QObject::connect(enrollmentConsent, &QCheckBox::toggled, [
    startEnrollment,
    captureSample,
    commitEnrollment
  ](bool checked) {
    const bool workflowInactive =
      !captureSample->isEnabled() && !commitEnrollment->isEnabled();
    startEnrollment->setEnabled(checked && workflowInactive);
  });

  QObject::connect(refreshButton, &QPushButton::clicked, [status]() {
    refreshStatus(status);
  });

  QObject::connect(refreshDaemonButton, &QPushButton::clicked, [
    daemonSummary,
    templateSummary,
    authSummary,
    daemonResponse
  ]() {
    refreshDaemonPanels(
      daemonSummary,
      templateSummary,
      authSummary,
      daemonResponse
    );
  });

  QObject::connect(detectorButton, &QPushButton::clicked, [
    daemonSummary,
    daemonResponse
  ]() {
    const QString response =
      queryDaemonOperation(QStringLiteral("detector_status"));
    daemonSummary->setPlainText(detectorStatusSummary(response));
    daemonResponse->setPlainText(
      QStringLiteral("Socket: %1\n\nOperation: detector_status\n\n%2")
        .arg(runtimeSocketPath(), response)
    );
  });

  QObject::connect(templateButton, &QPushButton::clicked, [
    templateSummary,
    daemonResponse
  ]() {
    const QString response =
      queryDaemonOperation(QStringLiteral("template_status"));
    templateSummary->setPlainText(templateStatusSummary(response));
    daemonResponse->setPlainText(
      QStringLiteral("Socket: %1\n\nOperation: template_status\n\n%2")
        .arg(runtimeSocketPath(), response)
    );
  });

  QObject::connect(understandButton, &QPushButton::clicked, [
    enrollmentConsent,
    tabs
  ]() {
    enrollmentConsent->setChecked(true);
    tabs->setCurrentIndex(1);
  });

  QObject::connect(brightnessButton, &QPushButton::clicked, [&window]() {
    QMessageBox::information(
      &window,
      QStringLiteral("Brightness assistance"),
      QStringLiteral(
        "Brightness assistance is not implemented.\n\n"
        "A future lock-screen integration may request temporary screen "
        "illumination only with explicit ownership, bounded duration, and "
        "guaranteed restoration."
      )
    );
  });

  QObject::connect(forgetButton, &QPushButton::clicked, [
    &window,
    status,
    templateSummary,
    captureTimer,
    processEnrollment
  ]() {
    captureTimer->stop();
    forgetMe(&window, status);
    processEnrollment(
      queryDaemonOperation(QStringLiteral("enrollment_status"))
    );
    const QString response =
      queryDaemonOperation(QStringLiteral("template_status"));
    templateSummary->setPlainText(templateStatusSummary(response));
  });

  QObject::connect(closeButton, &QPushButton::clicked, &window, &QWidget::close);

  QObject::connect(&app, &QCoreApplication::aboutToQuit, [captureTimer]() {
    if (captureTimer->isActive()) {
      captureTimer->stop();
      (void)queryDaemonOperation(QStringLiteral("enrollment_cancel"));
    }
  });

  window.resize(1040, 760);
  window.setMinimumSize(820, 600);
  window.show();
  return app.exec();
}
