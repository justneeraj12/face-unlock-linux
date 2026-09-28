#include "enrollment_session.h"

#include <stdexcept>

namespace face_unlock {

std::string enrollment_state_name(EnrollmentState state) {
  switch (state) {
    case EnrollmentState::Idle: return "idle";
    case EnrollmentState::Collecting: return "collecting";
    case EnrollmentState::Ready: return "ready";
    case EnrollmentState::Committed: return "committed";
    case EnrollmentState::Cancelled: return "cancelled";
  }
  return "idle";
}

EnrollmentSession::EnrollmentSession(EnrollmentPolicy policy)
    : policy_(policy) {
  if (policy_.target_per_pose < 1 || policy_.target_per_pose > 20 ||
      policy_.maximum_per_pose < policy_.target_per_pose ||
      policy_.maximum_per_pose > 50 ||
      policy_.duplicate_similarity < -1.0 ||
      policy_.duplicate_similarity > 1.0 ||
      policy_.minimum_quality < 0.0 ||
      policy_.minimum_quality > 1.0) {
    throw std::runtime_error("invalid enrollment policy");
  }
}

EnrollmentStatus EnrollmentSession::start(const std::string& model_id) {
  if (model_id.empty() || model_id.size() > 128) {
    throw std::runtime_error("invalid enrollment model id");
  }
  builder_ = std::make_unique<FaceProfileBuilder>(
    model_id,
    policy_.target_per_pose,
    policy_.maximum_per_pose,
    policy_.duplicate_similarity,
    policy_.minimum_quality
  );
  state_ = EnrollmentState::Collecting;
  accepted_samples_ = 0;
  last_reason_ = "started";
  return status();
}

AddSampleResult EnrollmentSession::add_sample(
  const FaceEmbedding& embedding,
  PoseSlot pose,
  double quality_score
) {
  AddSampleResult result;
  result.pose = pose;
  result.reason = "session_not_collecting";
  if (state_ != EnrollmentState::Collecting || !builder_) {
    return result;
  }

  result = builder_->add_sample(embedding, pose, quality_score);
  last_reason_ = result.reason;
  if (result.accepted) {
    ++accepted_samples_;
  }
  if (builder_->ready()) {
    state_ = EnrollmentState::Ready;
    last_reason_ = "ready_to_commit";
    result.progress_percent = 100;
  }
  return result;
}

EnrollmentStatus EnrollmentSession::cancel() {
  builder_.reset();
  state_ = EnrollmentState::Cancelled;
  accepted_samples_ = 0;
  last_reason_ = "cancelled";
  return status();
}

FaceProfile EnrollmentSession::finalize() const {
  if (state_ != EnrollmentState::Ready || !builder_) {
    throw std::runtime_error("enrollment is not ready to finalize");
  }
  return builder_->finalize();
}

EnrollmentStatus EnrollmentSession::mark_committed() {
  if (state_ != EnrollmentState::Ready || !builder_) {
    throw std::runtime_error("enrollment is not ready to commit");
  }
  builder_.reset();
  state_ = EnrollmentState::Committed;
  last_reason_ = "committed";
  return status();
}

EnrollmentStatus EnrollmentSession::status() const {
  EnrollmentStatus result;
  result.state = state_;
  result.accepted_samples = accepted_samples_;
  result.last_reason = last_reason_;
  if (builder_) {
    result.progress_percent = builder_->progress_percent();
    result.missing_poses = builder_->missing_poses();
  } else if (state_ == EnrollmentState::Committed) {
    result.progress_percent = 100;
  }
  return result;
}

}  // namespace face_unlock
