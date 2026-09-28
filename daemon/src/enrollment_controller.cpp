#include "enrollment_controller.h"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace face_unlock {
namespace {

double elapsed_ms(
  const std::chrono::steady_clock::time_point& start,
  const std::chrono::steady_clock::time_point& end
) {
  return static_cast<double>(
    std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
  ) / 1000.0;
}

}  // namespace

EnrollmentController::EnrollmentController(
  CameraLeaseManager* camera,
  FaceDetector* detector,
  FaceEmbedder* embedder,
  ProfileStoragePaths storage_paths,
  EnrollmentPolicy policy,
  int camera_lease_ms
) :
    camera_(camera),
    detector_(detector),
    embedder_(embedder),
    storage_paths_(std::move(storage_paths)),
    session_(policy),
    camera_lease_ms_(camera_lease_ms) {
  if (camera_lease_ms_ < 1000 || camera_lease_ms_ > 60000) {
    throw std::runtime_error("invalid enrollment camera lease");
  }
}

EnrollmentOperationResult EnrollmentController::result(
  bool ok,
  const std::string& reason
) const {
  EnrollmentOperationResult value;
  value.ok = ok;
  value.reason = reason;
  value.enrollment = session_.status();
  return value;
}

EnrollmentOperationResult EnrollmentController::start() {
  const EnrollmentStatus current = session_.status();
  if (current.state == EnrollmentState::Collecting ||
      current.state == EnrollmentState::Ready) {
    return result(false, "enrollment_already_active");
  }
  if (camera_ == nullptr) {
    return result(false, "camera_manager_unavailable");
  }
  if (detector_ == nullptr || detector_->backend_name() != "yunet") {
    return result(false, "landmark_detector_required");
  }
  if (embedder_ == nullptr || embedder_->model_id().empty()) {
    return result(false, "recognizer_unavailable");
  }
  const CameraLeaseState camera_state = camera_->status().state;
  if (camera_state == CameraLeaseState::Opening ||
      camera_state == CameraLeaseState::Active ||
      camera_state == CameraLeaseState::Stopping) {
    return result(false, "camera_busy");
  }

  try {
    session_.start(embedder_->model_id());
  } catch (const std::exception&) {
    return result(false, "enrollment_session_start_failed");
  }

  const CameraLeaseStartResult camera_start =
    camera_->start(camera_lease_ms_);
  if (!camera_start.accepted) {
    session_.cancel();
    return result(false, camera_start.reason);
  }
  return result(true, "started");
}

EnrollmentOperationResult EnrollmentController::capture() {
  if (session_.status().state != EnrollmentState::Collecting) {
    return result(false, "enrollment_not_collecting");
  }
  if (camera_ == nullptr || detector_ == nullptr || embedder_ == nullptr) {
    return result(false, "enrollment_runtime_unavailable");
  }

  cv::Mat frame;
  unsigned long long frames_total = 0;
  if (!camera_->snapshot(frame, frames_total)) {
    EnrollmentOperationResult current = status();
    if (!current.ok) return current;
    return result(false, "camera_not_ready");
  }

  DetectorResult detections;
  const auto detector_started = std::chrono::steady_clock::now();
  try {
    detections = detector_->detect(frame);
  } catch (const std::exception&) {
    return result(false, "detector_inference_failed");
  }
  const auto detector_finished = std::chrono::steady_clock::now();

  EnrollmentOperationResult value;
  value.faces_detected = detections.boxes.size();
  value.detector_ms = elapsed_ms(detector_started, detector_finished);
  try {
    value.quality = evaluate_frame_quality(frame, detections);
  } catch (const std::exception&) {
    value.reason = "quality_evaluation_failed";
    value.enrollment = session_.status();
    return value;
  }
  if (!value.quality.approved) {
    value.reason = value.quality.reason;
    value.enrollment = session_.status();
    return value;
  }

  value.pose = classify_pose(detections.boxes.front());
  if (value.pose == PoseSlot::Unknown) {
    value.reason = "pose_unknown";
    value.enrollment = session_.status();
    return value;
  }

  FaceEmbedding embedding;
  const auto embedding_started = std::chrono::steady_clock::now();
  try {
    embedding = embedder_->align_and_embed(
      frame,
      detections.boxes.front()
    );
  } catch (const std::exception&) {
    value.reason = "embedding_inference_failed";
    value.enrollment = session_.status();
    return value;
  }
  const auto embedding_finished = std::chrono::steady_clock::now();
  value.embedding_ms = elapsed_ms(embedding_started, embedding_finished);

  const AddSampleResult added = session_.add_sample(
    embedding,
    value.pose,
    1.0
  );
  value.ok = added.accepted;
  value.sample_accepted = added.accepted;
  value.reason = added.reason;
  value.enrollment = session_.status();
  if (value.enrollment.state == EnrollmentState::Ready) {
    (void)camera_->stop("enrollment_ready");
  }
  return value;
}

EnrollmentOperationResult EnrollmentController::status() {
  if (session_.status().state == EnrollmentState::Collecting &&
      camera_ != nullptr) {
    const CameraLeaseStatus camera_status = camera_->status();
    if (camera_status.state == CameraLeaseState::Failed) {
      session_.cancel();
      return result(false, "enrollment_camera_failed");
    }
    if (camera_status.state == CameraLeaseState::Idle &&
        camera_status.release_reason != "never_started" &&
        camera_status.release_reason != "start_requested") {
      session_.cancel();
      return result(false, "enrollment_camera_lease_ended");
    }
  }
  return result(true, session_.status().last_reason);
}

EnrollmentOperationResult EnrollmentController::cancel() {
  const EnrollmentStatus cancelled = session_.cancel();
  if (camera_ != nullptr) {
    (void)camera_->stop("enrollment_cancelled");
  }
  EnrollmentOperationResult value = result(true, "cancelled");
  value.enrollment = cancelled;
  return value;
}

EnrollmentOperationResult EnrollmentController::commit() {
  if (session_.status().state != EnrollmentState::Ready) {
    return result(false, "enrollment_not_ready");
  }

  FaceProfile profile;
  try {
    profile = session_.finalize();
  } catch (const std::exception&) {
    return result(false, "profile_finalize_failed");
  }

  const ProfileStorageResult stored = commit_encrypted_face_profile(
    profile,
    storage_paths_
  );
  if (!stored.ok) {
    return result(false, stored.reason);
  }

  EnrollmentOperationResult value = result(true, "committed");
  value.key_created = stored.key_created;
  value.enrollment = session_.mark_committed();
  if (camera_ != nullptr) {
    (void)camera_->stop("enrollment_committed");
  }
  return value;
}

}  // namespace face_unlock
