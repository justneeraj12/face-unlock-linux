#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLocalSocket>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <unistd.h>

namespace {

QString templatePath() {
  return QDir::homePath() + QStringLiteral("/.local/share/face-unlock/template.enc");
}

QString enrollmentPath() {
  return QDir::homePath() + QStringLiteral("/.local/share/face-unlock/enrollment.json");
}

QString runtimeSocketPath() {
  const QByteArray runtime = qgetenv("XDG_RUNTIME_DIR");

  if (!runtime.isEmpty()) {
    return QString::fromLocal8Bit(runtime) + QStringLiteral("/face-unlock.sock");
  }

  return QStringLiteral("/run/user/") +
    QString::number(getuid()) +
    QStringLiteral("/face-unlock.sock");
}

QString fileStatusLine(const QString& label, const QString& path) {
  const QFileInfo info(path);

  if (!info.exists()) {
    return label + QStringLiteral(": missing\n  ") + path + QStringLiteral("\n");
  }

  return label +
    QStringLiteral(": present\n  ") +
    path +
    QStringLiteral("\n  size: ") +
    QString::number(info.size()) +
    QStringLiteral(" bytes\n");
}

QString statusText() {
  QString text;

  text += QStringLiteral("Current GUI status:\n\n");
  text += QStringLiteral("- Consent/status scaffold\n");
  text += QStringLiteral("- Template status display enabled\n");
  text += QStringLiteral("- Forget-me deletion enabled for placeholder files\n");
  text += QStringLiteral("- Brightness assist placeholder only\n");
  text += QStringLiteral("- Pose slots scaffold only\n");
  text += QStringLiteral("- Quality checklist scaffold only\n");
  text += QStringLiteral("- Camera preview placeholder only\n");
  text += QStringLiteral("- Daemon detector_status query button\n");
  text += QStringLiteral("- No camera access yet\n");
  text += QStringLiteral("- No enrollment capture yet\n");
  text += QStringLiteral("- No PAM changes\n");
  text += QStringLiteral("- No authentication changes\n\n");

  text += QStringLiteral("User data status:\n\n");
  text += fileStatusLine(QStringLiteral("Encrypted template"), templatePath());
  text += fileStatusLine(QStringLiteral("Enrollment manifest"), enrollmentPath());

  return text;
}

QString consentText() {
  return QStringLiteral(
    "face-unlock-linux is currently an experimental prototype.\n"
    "\n"
    "Before future enrollment, you must understand:\n"
    "\n"
    "1. Face data is biometric data.\n"
    "2. Raw images should not be stored by default.\n"
    "3. Templates must be encrypted at rest.\n"
    "4. Password fallback must remain available.\n"
    "5. Development auth is not real authentication.\n"
    "6. You should be able to delete enrollment data.\n"
    "\n"
    "This GUI does not enroll yet.\n"
    "\n"
    "The Forget Me button currently deletes only the placeholder template and "
    "placeholder enrollment manifest paths used by this prototype."
  );
}

QString queryDaemonOperation(const QString& operation) {
  QLocalSocket socket;
  const QString path = runtimeSocketPath();

  socket.connectToServer(path);

  if (!socket.waitForConnected(700)) {
    return QStringLiteral(
      "{\"status\":\"fail\",\"reason\":\"daemon_socket_unavailable\",\"socket\":\"%1\"}"
    ).arg(path);
  }

  const QByteArray request =
    QByteArray("{\"op\":\"") +
    operation.toUtf8() +
    QByteArray("\",\"client\":\"qt_gui\"}\n");

  socket.write(request);

  if (!socket.waitForBytesWritten(700)) {
    return QStringLiteral(
      "{\"status\":\"fail\",\"reason\":\"daemon_write_timeout\"}"
    );
  }

  if (!socket.waitForReadyRead(1000)) {
    return QStringLiteral(
      "{\"status\":\"fail\",\"reason\":\"daemon_response_timeout\"}"
    );
  }

  const QByteArray response = socket.readAll();
  socket.disconnectFromServer();

  return QString::fromUtf8(response).trimmed();
}

QString jsonStringValue(const QJsonObject& object, const QString& key) {
  const QJsonValue value = object.value(key);

  if (value.isString()) {
    return value.toString();
  }

  if (value.isDouble()) {
    return QString::number(value.toInt());
  }

  if (value.isBool()) {
    return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
  }

  return QStringLiteral("unknown");
}

QString detectorStatusSummary(const QString& response) {
  QJsonParseError error {};
  const QJsonDocument document = QJsonDocument::fromJson(response.toUtf8(), &error);

  if (error.error != QJsonParseError::NoError || !document.isObject()) {
    QString summary;
    summary += QStringLiteral("Daemon available: no\n");
    summary += QStringLiteral("Parse error: %1\n").arg(error.errorString());
    return summary;
  }

  const QJsonObject object = document.object();

  const QString status = jsonStringValue(object, QStringLiteral("status"));
  const QString reason = jsonStringValue(object, QStringLiteral("reason"));
  const QString op = jsonStringValue(object, QStringLiteral("op"));
  const QString detector = jsonStringValue(object, QStringLiteral("detector"));
  const QString faces = jsonStringValue(object, QStringLiteral("faces_detected"));

  const bool available = status == QStringLiteral("ok") &&
                         op == QStringLiteral("detector_status");

  QString summary;
  summary += QStringLiteral("Daemon available: %1\n").arg(available ? "yes" : "no");
  summary += QStringLiteral("Status: %1\n").arg(status);
  summary += QStringLiteral("Reason: %1\n").arg(reason);
  summary += QStringLiteral("Operation: %1\n").arg(op);
  summary += QStringLiteral("Detector: %1\n").arg(detector);
  summary += QStringLiteral("Faces detected: %1\n").arg(faces);

  return summary;
}

void refreshStatus(QTextEdit* status) {
  status->setPlainText(statusText());
}

bool removeIfExists(const QString& path, QStringList& removed, QStringList& failed) {
  QFile file(path);

  if (!file.exists()) {
    return true;
  }

  if (file.remove()) {
    removed << path;
    return true;
  }

  failed << path;
  return false;
}

void forgetMe(QWidget* parent, QTextEdit* status) {
  const QString message =
    QStringLiteral(
      "This will delete prototype enrollment files if they exist:\n\n"
    ) +
    templatePath() +
    QStringLiteral("\n") +
    enrollmentPath() +
    QStringLiteral(
      "\n\n"
      "This does not modify PAM, sudo, login, or lock-screen settings.\n\n"
      "Continue?"
    );

  const QMessageBox::StandardButton answer = QMessageBox::warning(
    parent,
    QStringLiteral("Confirm Forget Me"),
    message,
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No
  );

  if (answer != QMessageBox::Yes) {
    return;
  }

  QStringList removed;
  QStringList failed;

  removeIfExists(templatePath(), removed, failed);
  removeIfExists(enrollmentPath(), removed, failed);

  refreshStatus(status);

  if (!failed.isEmpty()) {
    QMessageBox::critical(
      parent,
      QStringLiteral("Forget Me failed"),
      QStringLiteral("Failed to remove:\n\n") + failed.join(QStringLiteral("\n"))
    );
    return;
  }

  QString result = QStringLiteral("Forget Me completed.\n\n");

  if (removed.isEmpty()) {
    result += QStringLiteral("No prototype enrollment files were present.");
  } else {
    result += QStringLiteral("Removed:\n\n") + removed.join(QStringLiteral("\n"));
  }

  QMessageBox::information(
    parent,
    QStringLiteral("Forget Me complete"),
    result
  );
}

QTextEdit* readOnlyTextEdit(const QString& text, int minHeight = 120) {
  auto* edit = new QTextEdit();
  edit->setReadOnly(true);
  edit->setPlainText(text);
  edit->setMinimumHeight(minHeight);
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
  QApplication app(argc, argv);

  QWidget window;
  window.setWindowTitle(QStringLiteral("face-unlock-linux Enrollment"));

  auto* root = new QVBoxLayout(&window);

  auto* title = new QLabel(QStringLiteral("face-unlock-linux"));
  QFont titleFont = title->font();
  titleFont.setPointSize(20);
  titleFont.setBold(true);
  title->setFont(titleFont);

  auto* subtitle = new QLabel(QStringLiteral("Experimental enrollment GUI scaffold"));

  auto* warning = new QLabel(
    QStringLiteral("Not real enrollment yet. No GUI camera capture is performed.")
  );
  warning->setStyleSheet(QStringLiteral("color: #b00020; font-weight: bold;"));

  root->addWidget(title);
  root->addWidget(subtitle);
  root->addWidget(warning);

  auto* tabs = new QTabWidget();
  root->addWidget(tabs);

  // Status tab
  auto* statusTab = new QWidget();
  auto* statusLayout = new QVBoxLayout(statusTab);

  auto* status = readOnlyTextEdit(statusText(), 180);

  auto* daemonSummary = readOnlyTextEdit(QStringLiteral("No daemon query yet."), 110);
  auto* daemonResponse = readOnlyTextEdit(
    QStringLiteral("Socket: %1\n\nNo daemon query yet.").arg(runtimeSocketPath()),
    140
  );

  auto* statusButtonRow = new QHBoxLayout();
  auto* refreshButton = new QPushButton(QStringLiteral("Refresh local status"));
  auto* daemonStatusButton = new QPushButton(QStringLiteral("Query daemon detector_status"));
  statusButtonRow->addWidget(refreshButton);
  statusButtonRow->addWidget(daemonStatusButton);
  statusButtonRow->addStretch();

  auto* daemonSummaryLabel = new QLabel(QStringLiteral("Daemon detector summary"));
  QFont daemonSummaryFont = daemonSummaryLabel->font();
  daemonSummaryFont.setBold(true);
  daemonSummaryLabel->setFont(daemonSummaryFont);

  auto* daemonResponseLabel = new QLabel(QStringLiteral("Raw daemon response"));
  QFont daemonResponseFont = daemonResponseLabel->font();
  daemonResponseFont.setBold(true);
  daemonResponseLabel->setFont(daemonResponseFont);

  statusLayout->addWidget(status);
  statusLayout->addLayout(statusButtonRow);
  statusLayout->addWidget(daemonSummaryLabel);
  statusLayout->addWidget(daemonSummary);
  statusLayout->addWidget(daemonResponseLabel);
  statusLayout->addWidget(daemonResponse);
  statusLayout->addStretch();

  tabs->addTab(scrollable(statusTab), QStringLiteral("Status"));

  // Enrollment scaffold tab
  auto* enrollmentTab = new QWidget();
  auto* enrollmentLayout = new QVBoxLayout(enrollmentTab);

  auto* previewFrame = new QFrame();
  previewFrame->setFrameShape(QFrame::StyledPanel);
  previewFrame->setMinimumHeight(150);
  previewFrame->setStyleSheet(QStringLiteral(
    "QFrame { background-color: #202124; border: 1px solid #555; border-radius: 6px; }"
    "QLabel { color: #eeeeee; }"
  ));

  auto* previewLayout = new QVBoxLayout(previewFrame);
  auto* previewTitle = new QLabel(QStringLiteral("Camera preview placeholder"));
  QFont previewTitleFont = previewTitle->font();
  previewTitleFont.setBold(true);
  previewTitleFont.setPointSize(13);
  previewTitle->setFont(previewTitleFont);

  auto* previewText = new QLabel(QStringLiteral(
    "Live camera preview is not connected yet.\n"
    "No camera frames are read by the GUI.\n"
    "No images are saved."
  ));
  previewText->setAlignment(Qt::AlignCenter);

  previewLayout->addWidget(previewTitle);
  previewLayout->addStretch();
  previewLayout->addWidget(previewText);
  previewLayout->addStretch();

  auto* poseLabel = new QLabel(QStringLiteral("Pose slots scaffold"));
  QFont sectionFont = poseLabel->font();
  sectionFont.setBold(true);
  poseLabel->setFont(sectionFont);

  auto* poseRow = new QHBoxLayout();
  auto* centerPose = new QCheckBox(QStringLiteral("Center"));
  auto* leftPose = new QCheckBox(QStringLiteral("Left"));
  auto* rightPose = new QCheckBox(QStringLiteral("Right"));
  auto* upPose = new QCheckBox(QStringLiteral("Up"));
  auto* downPose = new QCheckBox(QStringLiteral("Down"));

  for (auto* box : {centerPose, leftPose, rightPose, upPose, downPose}) {
    box->setEnabled(false);
    poseRow->addWidget(box);
  }
  poseRow->addStretch();

  auto* poseButtonRow = new QHBoxLayout();
  auto* markPoseButton = new QPushButton(QStringLiteral("Mark demo pose complete"));
  auto* resetPoseButton = new QPushButton(QStringLiteral("Reset pose slots"));
  poseButtonRow->addWidget(markPoseButton);
  poseButtonRow->addWidget(resetPoseButton);
  poseButtonRow->addStretch();

  auto* qualityLabel = new QLabel(QStringLiteral("Quality checklist scaffold"));
  QFont qualityFont = qualityLabel->font();
  qualityFont.setBold(true);
  qualityLabel->setFont(qualityFont);

  auto* qualityRow = new QHBoxLayout();
  auto* lightingQuality = new QCheckBox(QStringLiteral("Lighting OK"));
  auto* sharpnessQuality = new QCheckBox(QStringLiteral("Sharpness OK"));
  auto* centeredQuality = new QCheckBox(QStringLiteral("Face centered"));
  auto* poseQuality = new QCheckBox(QStringLiteral("Pose coverage OK"));
  auto* templateQuality = new QCheckBox(QStringLiteral("Template ready"));

  for (auto* box : {lightingQuality, sharpnessQuality, centeredQuality, poseQuality, templateQuality}) {
    box->setEnabled(false);
    qualityRow->addWidget(box);
  }
  qualityRow->addStretch();

  auto* qualityButtonRow = new QHBoxLayout();
  auto* markQualityButton = new QPushButton(QStringLiteral("Mark demo quality OK"));
  auto* resetQualityButton = new QPushButton(QStringLiteral("Reset quality"));
  qualityButtonRow->addWidget(markQualityButton);
  qualityButtonRow->addWidget(resetQualityButton);
  qualityButtonRow->addStretch();

  enrollmentLayout->addWidget(previewFrame);
  enrollmentLayout->addWidget(poseLabel);
  enrollmentLayout->addLayout(poseRow);
  enrollmentLayout->addLayout(poseButtonRow);
  enrollmentLayout->addWidget(qualityLabel);
  enrollmentLayout->addLayout(qualityRow);
  enrollmentLayout->addLayout(qualityButtonRow);
  enrollmentLayout->addStretch();

  tabs->addTab(scrollable(enrollmentTab), QStringLiteral("Enrollment"));

  // Privacy tab
  auto* privacyTab = new QWidget();
  auto* privacyLayout = new QVBoxLayout(privacyTab);

  auto* consent = readOnlyTextEdit(consentText(), 260);

  auto* privacyButtonRow = new QHBoxLayout();
  auto* understandButton = new QPushButton(QStringLiteral("I understand"));
  auto* brightnessButton = new QPushButton(QStringLiteral("Brightness assist placeholder"));
  auto* forgetButton = new QPushButton(QStringLiteral("Forget me"));
  auto* closeButton = new QPushButton(QStringLiteral("Close"));

  privacyButtonRow->addWidget(understandButton);
  privacyButtonRow->addWidget(brightnessButton);
  privacyButtonRow->addWidget(forgetButton);
  privacyButtonRow->addStretch();
  privacyButtonRow->addWidget(closeButton);

  privacyLayout->addWidget(consent);
  privacyLayout->addLayout(privacyButtonRow);
  privacyLayout->addStretch();

  tabs->addTab(scrollable(privacyTab), QStringLiteral("Privacy"));

  QObject::connect(refreshButton, &QPushButton::clicked, [status]() {
    refreshStatus(status);
  });

  QObject::connect(daemonStatusButton, &QPushButton::clicked, [daemonSummary, daemonResponse]() {
    const QString response = queryDaemonOperation(QStringLiteral("detector_status"));

    daemonSummary->setPlainText(detectorStatusSummary(response));

    daemonResponse->setPlainText(
      QStringLiteral("Socket: %1\n\nOperation: detector_status\n\nResponse:\n%2")
        .arg(runtimeSocketPath(), response)
    );
  });

  QObject::connect(markPoseButton, &QPushButton::clicked, [centerPose, leftPose, rightPose, upPose, downPose]() {
    if (!centerPose->isChecked()) {
      centerPose->setChecked(true);
      return;
    }
    if (!leftPose->isChecked()) {
      leftPose->setChecked(true);
      return;
    }
    if (!rightPose->isChecked()) {
      rightPose->setChecked(true);
      return;
    }
    if (!upPose->isChecked()) {
      upPose->setChecked(true);
      return;
    }
    if (!downPose->isChecked()) {
      downPose->setChecked(true);
      return;
    }
  });

  QObject::connect(resetPoseButton, &QPushButton::clicked, [centerPose, leftPose, rightPose, upPose, downPose]() {
    centerPose->setChecked(false);
    leftPose->setChecked(false);
    rightPose->setChecked(false);
    upPose->setChecked(false);
    downPose->setChecked(false);
  });

  QObject::connect(markQualityButton, &QPushButton::clicked, [lightingQuality, sharpnessQuality, centeredQuality, poseQuality, templateQuality]() {
    lightingQuality->setChecked(true);
    sharpnessQuality->setChecked(true);
    centeredQuality->setChecked(true);
    poseQuality->setChecked(true);
    templateQuality->setChecked(true);
  });

  QObject::connect(resetQualityButton, &QPushButton::clicked, [lightingQuality, sharpnessQuality, centeredQuality, poseQuality, templateQuality]() {
    lightingQuality->setChecked(false);
    sharpnessQuality->setChecked(false);
    centeredQuality->setChecked(false);
    poseQuality->setChecked(false);
    templateQuality->setChecked(false);
  });

  QObject::connect(understandButton, &QPushButton::clicked, [&window]() {
    QMessageBox::information(
      &window,
      QStringLiteral("Consent acknowledged"),
      QStringLiteral(
        "Acknowledged. Future versions will use this step before enrollment.\n\n"
        "No enrollment was performed."
      )
    );
  });

  QObject::connect(brightnessButton, &QPushButton::clicked, [&window]() {
    QMessageBox::information(
      &window,
      QStringLiteral("Brightness assist placeholder"),
      QStringLiteral(
        "Brightness assist is not implemented yet.\n\n"
        "Future versions may temporarily increase screen brightness during enrollment snapshots.\n\n"
        "Required safety behavior:\n"
        "- ask for explicit consent\n"
        "- remember previous brightness\n"
        "- restore previous brightness immediately\n"
        "- never change brightness silently\n"
        "- fail safely if brightness control is unavailable"
      )
    );
  });

  QObject::connect(forgetButton, &QPushButton::clicked, [&window, status]() {
    forgetMe(&window, status);
  });

  QObject::connect(closeButton, &QPushButton::clicked, &window, &QWidget::close);

  window.resize(900, 680);
  window.setMinimumSize(760, 520);
  window.show();

  return app.exec();
}
