#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QString templatePath() {
  return QDir::homePath() + QStringLiteral("/.local/share/face-unlock/template.enc");
}

QString enrollmentPath() {
  return QDir::homePath() + QStringLiteral("/.local/share/face-unlock/enrollment.json");
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
  text += QStringLiteral("- No camera access yet\n");
  text += QStringLiteral("- No enrollment capture yet\n");
  text += QStringLiteral("- No PAM changes\n");
  text += QStringLiteral("- No authentication changes\n\n");

  text += QStringLiteral("Pose slots:\n\n");
  text += QStringLiteral("- Center: not captured\n");
  text += QStringLiteral("- Left: not captured\n");
  text += QStringLiteral("- Right: not captured\n");
  text += QStringLiteral("- Up: not captured\n");
  text += QStringLiteral("- Down: not captured\n\n");

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

void refreshStatus(QTextEdit* status) {
  status->setPlainText(statusText());
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

}  // namespace

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  QWidget window;
  window.setWindowTitle(QStringLiteral("face-unlock-linux Enrollment"));

  auto* root = new QVBoxLayout(&window);

  auto* title = new QLabel(QStringLiteral("face-unlock-linux"));
  QFont titleFont = title->font();
  titleFont.setPointSize(22);
  titleFont.setBold(true);
  title->setFont(titleFont);

  auto* subtitle = new QLabel(
    QStringLiteral("Experimental enrollment GUI scaffold")
  );

  auto* warning = new QLabel(
    QStringLiteral(
      "This is not real enrollment yet. No camera capture is performed by this GUI."
    )
  );
  warning->setStyleSheet(QStringLiteral("color: #b00020; font-weight: bold;"));

  auto* status = new QTextEdit();
  status->setReadOnly(true);
  status->setPlainText(statusText());
  status->setMinimumHeight(210);

  auto* consent = new QTextEdit();
  consent->setReadOnly(true);
  consent->setPlainText(consentText());
  consent->setMinimumHeight(260);

  auto* poseLabel = new QLabel(QStringLiteral("Enrollment pose slots scaffold"));
  QFont poseFont = poseLabel->font();
  poseFont.setBold(true);
  poseLabel->setFont(poseFont);

  auto* poseRow = new QHBoxLayout();

  auto* centerPose = new QCheckBox(QStringLiteral("Center"));
  auto* leftPose = new QCheckBox(QStringLiteral("Left"));
  auto* rightPose = new QCheckBox(QStringLiteral("Right"));
  auto* upPose = new QCheckBox(QStringLiteral("Up"));
  auto* downPose = new QCheckBox(QStringLiteral("Down"));

  centerPose->setEnabled(false);
  leftPose->setEnabled(false);
  rightPose->setEnabled(false);
  upPose->setEnabled(false);
  downPose->setEnabled(false);

  poseRow->addWidget(centerPose);
  poseRow->addWidget(leftPose);
  poseRow->addWidget(rightPose);
  poseRow->addWidget(upPose);
  poseRow->addWidget(downPose);
  poseRow->addStretch();

  auto* poseButtonRow = new QHBoxLayout();
  auto* markPoseButton = new QPushButton(QStringLiteral("Mark demo pose complete"));
  auto* resetPoseButton = new QPushButton(QStringLiteral("Reset pose slots"));
  poseButtonRow->addWidget(markPoseButton);
  poseButtonRow->addWidget(resetPoseButton);
  poseButtonRow->addStretch();

  auto* qualityLabel = new QLabel(QStringLiteral("Enrollment quality checklist scaffold"));
  QFont qualityFont = qualityLabel->font();
  qualityFont.setBold(true);
  qualityLabel->setFont(qualityFont);

  auto* qualityRow = new QHBoxLayout();

  auto* lightingQuality = new QCheckBox(QStringLiteral("Lighting OK"));
  auto* sharpnessQuality = new QCheckBox(QStringLiteral("Sharpness OK"));
  auto* centeredQuality = new QCheckBox(QStringLiteral("Face centered"));
  auto* poseQuality = new QCheckBox(QStringLiteral("Pose coverage OK"));
  auto* templateQuality = new QCheckBox(QStringLiteral("Template ready"));

  lightingQuality->setEnabled(false);
  sharpnessQuality->setEnabled(false);
  centeredQuality->setEnabled(false);
  poseQuality->setEnabled(false);
  templateQuality->setEnabled(false);

  qualityRow->addWidget(lightingQuality);
  qualityRow->addWidget(sharpnessQuality);
  qualityRow->addWidget(centeredQuality);
  qualityRow->addWidget(poseQuality);
  qualityRow->addWidget(templateQuality);
  qualityRow->addStretch();

  auto* qualityButtonRow = new QHBoxLayout();
  auto* markQualityButton = new QPushButton(QStringLiteral("Mark demo quality OK"));
  auto* resetQualityButton = new QPushButton(QStringLiteral("Reset quality"));
  qualityButtonRow->addWidget(markQualityButton);
  qualityButtonRow->addWidget(resetQualityButton);
  qualityButtonRow->addStretch();

  auto* buttonRow = new QHBoxLayout();

  auto* understandButton = new QPushButton(QStringLiteral("I understand"));
  auto* refreshButton = new QPushButton(QStringLiteral("Refresh status"));
  auto* brightnessButton = new QPushButton(QStringLiteral("Brightness assist placeholder"));
  auto* forgetButton = new QPushButton(QStringLiteral("Forget me"));
  auto* closeButton = new QPushButton(QStringLiteral("Close"));

  buttonRow->addWidget(understandButton);
  buttonRow->addWidget(refreshButton);
  buttonRow->addWidget(brightnessButton);
  buttonRow->addWidget(forgetButton);
  buttonRow->addStretch();
  buttonRow->addWidget(closeButton);

  root->addWidget(title);
  root->addWidget(subtitle);
  root->addWidget(warning);
  root->addWidget(status);
  root->addWidget(consent);
  root->addWidget(poseLabel);
  root->addLayout(poseRow);
  root->addLayout(poseButtonRow);
  root->addWidget(qualityLabel);
  root->addLayout(qualityRow);
  root->addLayout(qualityButtonRow);
  root->addLayout(buttonRow);

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

  QObject::connect(refreshButton, &QPushButton::clicked, [status]() {
    refreshStatus(status);
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

  window.resize(820, 700);
  window.show();

  return app.exec();
}
