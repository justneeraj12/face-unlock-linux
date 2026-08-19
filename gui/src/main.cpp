#include <QApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QString statusText() {
  return QStringLiteral(
    "Current GUI status:\\n"
    "\\n"
    "- Consent screen scaffold only\\n"
    "- No camera access yet\\n"
    "- No face crops saved\\n"
    "- No templates written\\n"
    "- No PAM changes\\n"
    "- No authentication changes\\n"
  );
}

QString consentText() {
  return QStringLiteral(
    "face-unlock-linux is currently an experimental prototype.\\n"
    "\\n"
    "Before future enrollment, you must understand:\\n"
    "\\n"
    "1. Face data is biometric data.\\n"
    "2. Raw images should not be stored by default.\\n"
    "3. Templates must be encrypted at rest.\\n"
    "4. Password fallback must remain available.\\n"
    "5. Development auth is not real authentication.\\n"
    "6. You should be able to delete enrollment data.\\n"
    "\\n"
    "This GUI does not enroll yet. It is only a consent/status scaffold."
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
      "This is not real enrollment yet. No biometric data is saved by this GUI."
    )
  );
  warning->setStyleSheet(QStringLiteral("color: #b00020; font-weight: bold;"));

  auto* status = new QTextEdit();
  status->setReadOnly(true);
  status->setPlainText(statusText());
  status->setMinimumHeight(150);

  auto* consent = new QTextEdit();
  consent->setReadOnly(true);
  consent->setPlainText(consentText());
  consent->setMinimumHeight(240);

  auto* buttonRow = new QHBoxLayout();

  auto* understandButton = new QPushButton(QStringLiteral("I understand"));
  auto* forgetButton = new QPushButton(QStringLiteral("Forget me placeholder"));
  auto* closeButton = new QPushButton(QStringLiteral("Close"));

  buttonRow->addWidget(understandButton);
  buttonRow->addWidget(forgetButton);
  buttonRow->addStretch();
  buttonRow->addWidget(closeButton);

  root->addWidget(title);
  root->addWidget(subtitle);
  root->addWidget(warning);
  root->addWidget(status);
  root->addWidget(consent);
  root->addLayout(buttonRow);

  QObject::connect(understandButton, &QPushButton::clicked, [&window]() {
    QMessageBox::information(
      &window,
      QStringLiteral("Consent acknowledged"),
      QStringLiteral(
        "Acknowledged. Future versions will use this step before enrollment.\\n"
        "\\n"
        "No enrollment was performed."
      )
    );
  });

  QObject::connect(forgetButton, &QPushButton::clicked, [&window]() {
    QMessageBox::information(
      &window,
      QStringLiteral("Forget me placeholder"),
      QStringLiteral(
        "Future versions will securely delete enrollment metadata and encrypted templates.\\n"
        "\\n"
        "No files were changed by this placeholder."
      )
    );
  });

  QObject::connect(closeButton, &QPushButton::clicked, &window, &QWidget::close);

  window.resize(760, 620);
  window.show();

  return app.exec();
}
