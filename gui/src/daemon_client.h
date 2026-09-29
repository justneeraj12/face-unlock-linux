#pragma once

#include <QString>
#include <QStringList>

namespace face_unlock_gui {

struct EnrollmentSnapshot {
  bool parsed = false;
  bool operationValid = false;
  bool ok = false;
  bool sampleAccepted = false;
  bool keyCreated = false;
  QString operation;
  QString reason = QStringLiteral("unavailable");
  QString state = QStringLiteral("idle");
  QString pose = QStringLiteral("unknown");
  QString qualityReason = QStringLiteral("not_evaluated");
  QStringList missingPoses;
  int progress = 0;
  int acceptedSamples = 0;
  int validationProgress = 0;
  int validationSamples = 0;
  int facesDetected = 0;
  double lastValidationSimilarity = -1.0;
  double lowestValidationSimilarity = -1.0;
  double minimumValidationSimilarity = 0.45;
  double detectorMs = 0.0;
  double embeddingMs = 0.0;
  double meanLuma = 0.0;
  double sharpness = 0.0;
  double faceAreaRatio = 0.0;
  QString raw;
};

QString runtimeSocketPath();
QString queryDaemonOperation(const QString& operation);
QString detectorStatusSummary(const QString& response);
QString templateStatusSummary(const QString& response);
EnrollmentSnapshot parseEnrollmentResponse(const QString& response);
QString friendlyReason(const QString& reason);
QString poseInstruction(const QString& pose);
int enrollmentParserSelfTest();

}  // namespace face_unlock_gui
