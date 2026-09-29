#include "daemon_client.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLocalSocket>

#include <iostream>
#include <unistd.h>

namespace face_unlock_gui {

QString runtimeSocketPath() {
  const QByteArray runtime = qgetenv("XDG_RUNTIME_DIR");
  if (!runtime.isEmpty()) {
    return QString::fromLocal8Bit(runtime) +
      QStringLiteral("/face-unlock.sock");
  }
  return QStringLiteral("/run/user/") +
    QString::number(getuid()) +
    QStringLiteral("/face-unlock.sock");
}

QString queryDaemonOperation(const QString& operation) {
  QLocalSocket socket;
  const QString path = runtimeSocketPath();
  socket.connectToServer(path);
  if (!socket.waitForConnected(700)) {
    return QStringLiteral(
      "{\"status\":\"fail\",\"reason\":\"daemon_socket_unavailable\","
      "\"socket\":\"%1\"}"
    ).arg(path);
  }

  const QByteArray request = QByteArray("{\"op\":\"") +
    operation.toUtf8() + QByteArray("\",\"client\":\"qt_gui\"}\n");
  socket.write(request);
  if (!socket.waitForBytesWritten(700)) {
    return QStringLiteral(
      "{\"status\":\"fail\",\"reason\":\"daemon_write_timeout\"}"
    );
  }
  if (!socket.waitForReadyRead(1500)) {
    return QStringLiteral(
      "{\"status\":\"fail\",\"reason\":\"daemon_response_timeout\"}"
    );
  }

  QByteArray response = socket.readAll();
  while (socket.waitForReadyRead(20)) {
    response += socket.readAll();
  }
  socket.disconnectFromServer();
  return QString::fromUtf8(response).trimmed();
}

bool parseObject(
  const QString& response,
  QJsonObject& object,
  QString& parseError
) {
  QJsonParseError error {};
  const QJsonDocument document =
    QJsonDocument::fromJson(response.toUtf8(), &error);
  if (error.error != QJsonParseError::NoError || !document.isObject()) {
    parseError = error.errorString();
    return false;
  }
  object = document.object();
  return true;
}

QString jsonStringValue(const QJsonObject& object, const QString& key) {
  const QJsonValue value = object.value(key);
  if (value.isString()) return value.toString();
  if (value.isDouble()) return QString::number(value.toDouble());
  if (value.isBool()) {
    return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
  }
  return QStringLiteral("unknown");
}

QString detectorStatusSummary(const QString& response) {
  QJsonObject object;
  QString error;
  if (!parseObject(response, object, error)) {
    return QStringLiteral("Daemon available: no\nParse error: %1\n").arg(error);
  }

  const QString status = jsonStringValue(object, QStringLiteral("status"));
  const QString operation = jsonStringValue(object, QStringLiteral("op"));
  const bool available = status == QStringLiteral("ok") &&
    operation == QStringLiteral("detector_status");

  QString summary;
  summary += QStringLiteral("Daemon available: %1\n")
    .arg(available ? "yes" : "no");
  summary += QStringLiteral("Status: %1\n").arg(status);
  summary += QStringLiteral("Reason: %1\n")
    .arg(jsonStringValue(object, QStringLiteral("reason")));
  summary += QStringLiteral("Detector: %1\n")
    .arg(jsonStringValue(object, QStringLiteral("detector")));
  summary += QStringLiteral("Faces detected: %1\n")
    .arg(jsonStringValue(object, QStringLiteral("faces_detected")));
  summary += QStringLiteral("Detector ms: %1\n")
    .arg(jsonStringValue(object, QStringLiteral("detector_ms")));
  return summary;
}

QString templateStatusSummary(const QString& response) {
  QJsonObject object;
  QString error;
  if (!parseObject(response, object, error)) {
    return QStringLiteral("Daemon available: no\nParse error: %1\n").arg(error);
  }

  const QString status = jsonStringValue(object, QStringLiteral("status"));
  const QString operation = jsonStringValue(object, QStringLiteral("op"));
  const bool available = status == QStringLiteral("ok") &&
    operation == QStringLiteral("template_status");

  QString summary;
  summary += QStringLiteral("Daemon available: %1\n")
    .arg(available ? "yes" : "no");
  summary += QStringLiteral("Status: %1\n").arg(status);
  summary += QStringLiteral("Reason: %1\n")
    .arg(jsonStringValue(object, QStringLiteral("reason")));
  summary += QStringLiteral("Template: %1\n")
    .arg(jsonStringValue(object, QStringLiteral("template")));
  summary += QStringLiteral("Enrollment: %1\n")
    .arg(jsonStringValue(object, QStringLiteral("enrollment")));
  summary += QStringLiteral("Key: %1\n")
    .arg(jsonStringValue(object, QStringLiteral("key")));
  summary += QStringLiteral("Key storage: %1\n")
    .arg(jsonStringValue(object, QStringLiteral("key_storage")));
  summary += QStringLiteral("Decryptability: %1\n")
    .arg(jsonStringValue(object, QStringLiteral("decryptability")));
  summary += QStringLiteral("Template decrypt: %1\n")
    .arg(jsonStringValue(object, QStringLiteral("template_decrypt")));
  return summary;
}

EnrollmentSnapshot parseEnrollmentResponse(const QString& response) {
  EnrollmentSnapshot snapshot;
  snapshot.raw = response;

  QJsonObject object;
  QString error;
  if (!parseObject(response, object, error)) {
    snapshot.reason = QStringLiteral("invalid_daemon_json: ") + error;
    return snapshot;
  }

  snapshot.parsed = true;
  snapshot.operation = object.value(QStringLiteral("op")).toString();
  snapshot.operationValid =
    snapshot.operation.startsWith(QStringLiteral("enrollment_"));
  snapshot.ok = object.value(QStringLiteral("status")).toString() ==
    QStringLiteral("ok");
  snapshot.reason = object.value(QStringLiteral("reason")).toString(
    QStringLiteral("reason_missing")
  );
  snapshot.state = object.value(QStringLiteral("enrollment_state")).toString(
    QStringLiteral("idle")
  );
  snapshot.pose = object.value(QStringLiteral("pose")).toString(
    QStringLiteral("unknown")
  );
  snapshot.qualityReason =
    object.value(QStringLiteral("quality_reason")).toString(
      QStringLiteral("not_evaluated")
    );
  snapshot.progress =
    object.value(QStringLiteral("progress_percent")).toInt();
  snapshot.acceptedSamples =
    object.value(QStringLiteral("accepted_samples")).toInt();
  snapshot.facesDetected =
    object.value(QStringLiteral("faces_detected")).toInt();
  snapshot.detectorMs =
    object.value(QStringLiteral("detector_ms")).toDouble();
  snapshot.embeddingMs =
    object.value(QStringLiteral("embedding_ms")).toDouble();
  snapshot.meanLuma =
    object.value(QStringLiteral("mean_luma")).toDouble();
  snapshot.sharpness =
    object.value(QStringLiteral("sharpness")).toDouble();
  snapshot.faceAreaRatio =
    object.value(QStringLiteral("face_area_ratio")).toDouble();
  snapshot.sampleAccepted =
    object.value(QStringLiteral("sample_accepted")).toBool();
  snapshot.keyCreated =
    object.value(QStringLiteral("key_created")).toBool();

  const QJsonArray missing =
    object.value(QStringLiteral("missing_poses")).toArray();
  for (const QJsonValue& value : missing) {
    if (value.isString()) snapshot.missingPoses.append(value.toString());
  }
  return snapshot;
}

QString friendlyReason(const QString& reason) {
  if (reason == QStringLiteral("started")) {
    return QStringLiteral("Camera requested. Hold still while it warms up.");
  }
  if (reason == QStringLiteral("sample_accepted")) {
    return QStringLiteral("Qualified sample added.");
  }
  if (reason == QStringLiteral("camera_not_ready")) {
    return QStringLiteral("Camera is warming up.");
  }
  if (reason == QStringLiteral("face_missing")) {
    return QStringLiteral("No face found. Face the camera.");
  }
  if (reason == QStringLiteral("multiple_faces")) {
    return QStringLiteral("Only one person can be in frame.");
  }
  if (reason == QStringLiteral("low_light")) {
    return QStringLiteral("The face is too dark. Add light.");
  }
  if (reason == QStringLiteral("overexposed")) {
    return QStringLiteral("The face is overexposed. Reduce direct light.");
  }
  if (reason == QStringLiteral("blurred")) {
    return QStringLiteral("Image is blurred. Hold still.");
  }
  if (reason == QStringLiteral("face_too_small")) {
    return QStringLiteral("Move closer to the camera.");
  }
  if (reason == QStringLiteral("face_too_large")) {
    return QStringLiteral("Move slightly away from the camera.");
  }
  if (reason == QStringLiteral("detection_confidence_low")) {
    return QStringLiteral("Face confidence is low. Look toward the camera.");
  }
  if (reason == QStringLiteral("duplicate_sample")) {
    return QStringLiteral("Turn a little farther and hold still.");
  }
  if (reason == QStringLiteral("camera_busy")) {
    return QStringLiteral("Camera is busy with another operation.");
  }
  if (reason == QStringLiteral("landmark_detector_required")) {
    return QStringLiteral("Start the daemon with the YuNet detector.");
  }
  if (reason == QStringLiteral("recognizer_unavailable")) {
    return QStringLiteral("Start the daemon with an SFace model.");
  }
  if (reason == QStringLiteral("enrollment_camera_failed")) {
    return QStringLiteral("Camera failed. Enrollment was erased.");
  }
  if (reason == QStringLiteral("enrollment_camera_lease_ended")) {
    return QStringLiteral("Enrollment timed out and was erased.");
  }
  if (reason == QStringLiteral("ready_to_commit")) {
    return QStringLiteral("Profile is ready. Save it when satisfied.");
  }
  if (reason == QStringLiteral("committed")) {
    return QStringLiteral("Encrypted face profile saved.");
  }
  if (reason == QStringLiteral("cancelled")) {
    return QStringLiteral("Enrollment cancelled and samples erased.");
  }
  if (reason == QStringLiteral("daemon_socket_unavailable")) {
    return QStringLiteral("Daemon is not running or its socket is unavailable.");
  }
  return reason;
}

QString poseInstruction(const QString& pose) {
  if (pose == QStringLiteral("center")) {
    return QStringLiteral("Look straight at the camera.");
  }
  if (pose == QStringLiteral("left")) {
    return QStringLiteral("Slowly turn your face left.");
  }
  if (pose == QStringLiteral("right")) {
    return QStringLiteral("Slowly turn your face right.");
  }
  if (pose == QStringLiteral("up")) {
    return QStringLiteral("Lift your chin slightly.");
  }
  if (pose == QStringLiteral("down")) {
    return QStringLiteral("Lower your chin slightly.");
  }
  return QStringLiteral("Hold still while the profile is checked.");
}


int enrollmentParserSelfTest() {
  const QString collecting = QStringLiteral(
    "{\"status\":\"ok\",\"op\":\"enrollment_capture\","
    "\"reason\":\"sample_accepted\",\"enrollment_state\":\"collecting\","
    "\"progress_percent\":40,\"accepted_samples\":6,"
    "\"missing_poses\":[\"right\",\"up\",\"down\"],"
    "\"sample_accepted\":true,\"pose\":\"left\","
    "\"faces_detected\":1,\"detector_ms\":4.5,\"embedding_ms\":9.5,"
    "\"quality_reason\":\"approved\",\"mean_luma\":110.0,"
    "\"sharpness\":45.0,\"face_area_ratio\":0.20,"
    "\"key_created\":false}"
  );
  const EnrollmentSnapshot first = parseEnrollmentResponse(collecting);
  if (!first.parsed || !first.operationValid || !first.ok ||
      !first.sampleAccepted || first.progress != 40 ||
      first.acceptedSamples != 6 ||
      first.missingPoses != QStringList({"right", "up", "down"}) ||
      first.pose != QStringLiteral("left")) {
    std::cerr << "enrollment_gui_parser_status: failed\n";
    return 1;
  }

  const QString committed = QStringLiteral(
    "{\"status\":\"ok\",\"op\":\"enrollment_commit\","
    "\"reason\":\"committed\",\"enrollment_state\":\"committed\","
    "\"progress_percent\":100,\"accepted_samples\":15,"
    "\"missing_poses\":[],\"sample_accepted\":false,"
    "\"pose\":\"unknown\",\"faces_detected\":0,"
    "\"detector_ms\":0,\"embedding_ms\":0,"
    "\"quality_reason\":\"not_evaluated\",\"mean_luma\":0,"
    "\"sharpness\":0,\"face_area_ratio\":0,\"key_created\":true}"
  );
  const EnrollmentSnapshot second = parseEnrollmentResponse(committed);
  if (!second.parsed || !second.operationValid || !second.ok ||
      second.state != QStringLiteral("committed") ||
      second.progress != 100 || !second.keyCreated ||
      !second.missingPoses.isEmpty()) {
    std::cerr << "enrollment_gui_parser_status: failed\n";
    return 1;
  }

  std::cout << "enrollment_gui_parser_status: ok\n";
  return 0;
}

}  // namespace face_unlock_gui
