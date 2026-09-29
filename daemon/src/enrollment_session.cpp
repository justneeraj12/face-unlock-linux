#include "enrollment_session.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace face_unlock {
namespace {

constexpr std::size_t kEnrollmentPoseCount = 5;

bool valid_pose(PoseSlot pose) {
  return pose != PoseSlot::Unknown &&
    static_cast<std::size_t>(pose) < kEnrollmentPoseCount;
}

}  // namespace

std::string enrollment_state_name(EnrollmentState state) {
  switch (state) {
    case EnrollmentState::Idle: return "idle";
    case EnrollmentState::Collecting: return "collecting";
    case EnrollmentState::Validating: return "validating";
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
      policy_.validation_samples_per_pose < 1 ||
      policy_.validation_samples_per_pose > 5 ||
      policy_.duplicate_similarity < -1.0 ||
      policy_.duplicate_similarity > 1.0 ||
      policy_.minimum_quality < 0.0 ||
      policy_.minimum_quality > 1.0 ||
      policy_.minimum_validation_similarity < -1.0 ||
      policy_.minimum_validation_similarity > 1.0) {
    throw std::runtime_error("invalid enrollment policy");
  }
}

void EnrollmentSession::clear_session_data() {
  builder_.reset();
  candidate_profile_.reset();
  validation_counts_.fill(0);
  accepted_samples_ = 0;
  last_validation_similarity_ = -1.0;
  lowest_validation_similarity_ = -1.0;
}

EnrollmentStatus EnrollmentSession::start(const std::string& model_id) {
  if (model_id.empty() || model_id.size() > 128) {
    throw std::runtime_error("invalid enrollment model id");
  }
  clear_session_data();
  builder_ = std::make_unique<FaceProfileBuilder>(
    model_id,
    policy_.target_per_pose,
    policy_.maximum_per_pose,
    policy_.duplicate_similarity,
    policy_.minimum_quality
  );
  state_ = EnrollmentState::Collecting;
  last_reason_ = "started";
  return status();
}

AddSampleResult EnrollmentSession::add_sample(
  const FaceEmbedding& embedding,
  PoseSlot pose,
  double quality_score
) {
  if (state_ == EnrollmentState::Validating) {
    return add_validation_sample(embedding, pose, quality_score);
  }

  AddSampleResult result;
  result.pose = pose;
  result.reason = "session_not_collecting";
  if (state_ != EnrollmentState::Collecting || !builder_) {
    return result;
  }

  result = builder_->add_sample(embedding, pose, quality_score);
  last_reason_ = result.reason;
  if (result.accepted) ++accepted_samples_;

  if (builder_->ready()) {
    try {
      candidate_profile_ = builder_->finalize();
    } catch (const std::exception&) {
      result.accepted = false;
      result.reason = "candidate_profile_finalize_failed";
      last_reason_ = result.reason;
      return result;
    }
    state_ = EnrollmentState::Validating;
    last_reason_ = "training_complete_validation_required";
    result.reason = last_reason_;
    result.progress_percent = status().progress_percent;
  }
  return result;
}

AddSampleResult EnrollmentSession::add_validation_sample(
  const FaceEmbedding& embedding,
  PoseSlot pose,
  double quality_score
) {
  AddSampleResult result;
  result.pose = pose;
  result.progress_percent = status().progress_percent;

  if (!candidate_profile_.has_value()) {
    result.reason = "candidate_profile_missing";
    last_reason_ = result.reason;
    return result;
  }
  if (!valid_pose(pose)) {
    result.reason = "unsupported_pose";
    last_reason_ = result.reason;
    return result;
  }
  if (!std::isfinite(quality_score) || quality_score < 0.0 ||
      quality_score > 1.0) {
    result.reason = "invalid_quality";
    last_reason_ = result.reason;
    return result;
  }
  if (quality_score < policy_.minimum_quality) {
    result.reason = "quality_too_low";
    last_reason_ = result.reason;
    return result;
  }
  if (embedding.model != candidate_profile_->model_id) {
    result.reason = "embedding_model_mismatch";
    last_reason_ = result.reason;
    return result;
  }

  const std::size_t index = static_cast<std::size_t>(pose);
  if (validation_counts_[index] >= policy_.validation_samples_per_pose) {
    result.reason = "validation_pose_complete";
    last_reason_ = result.reason;
    return result;
  }

  const auto pose_template = std::find_if(
    candidate_profile_->poses.begin(),
    candidate_profile_->poses.end(),
    [pose](const PoseTemplate& value) { return value.pose == pose; }
  );
  if (pose_template == candidate_profile_->poses.end()) {
    result.reason = "validation_pose_template_missing";
    last_reason_ = result.reason;
    return result;
  }

  const FaceEmbedding centroid{
    candidate_profile_->model_id,
    pose_template->centroid,
  };
  double similarity = -1.0;
  try {
    similarity = SFaceEmbedder::cosine_similarity(embedding, centroid);
  } catch (const std::exception&) {
    result.reason = "heldout_embedding_invalid";
    last_reason_ = result.reason;
    return result;
  }

  last_validation_similarity_ = similarity;
  if (similarity < policy_.minimum_validation_similarity) {
    result.reason = "heldout_similarity_low";
    last_reason_ = result.reason;
    return result;
  }

  const bool first_validation_sample = validation_sample_count() == 0;
  ++validation_counts_[index];
  ++accepted_samples_;
  if (first_validation_sample) {
    lowest_validation_similarity_ = similarity;
  } else {
    lowest_validation_similarity_ = std::min(
      lowest_validation_similarity_, similarity
    );
  }

  result.accepted = true;
  result.reason = "heldout_sample_accepted";
  last_reason_ = result.reason;
  if (validation_complete()) {
    state_ = EnrollmentState::Ready;
    last_reason_ = "heldout_validation_passed";
    result.reason = last_reason_;
  }
  result.progress_percent = status().progress_percent;
  return result;
}

std::size_t EnrollmentSession::validation_sample_count() const {
  std::size_t total = 0;
  for (const std::size_t count : validation_counts_) total += count;
  return total;
}

bool EnrollmentSession::validation_complete() const {
  return validation_sample_count() ==
    policy_.validation_samples_per_pose * kEnrollmentPoseCount;
}

std::vector<PoseSlot> EnrollmentSession::missing_validation_poses() const {
  std::vector<PoseSlot> missing;
  for (std::size_t index = 0; index < kEnrollmentPoseCount; ++index) {
    if (validation_counts_[index] < policy_.validation_samples_per_pose) {
      missing.push_back(static_cast<PoseSlot>(index));
    }
  }
  return missing;
}

EnrollmentStatus EnrollmentSession::cancel() {
  clear_session_data();
  state_ = EnrollmentState::Cancelled;
  last_reason_ = "cancelled";
  return status();
}

FaceProfile EnrollmentSession::finalize() const {
  if (state_ != EnrollmentState::Ready || !candidate_profile_.has_value()) {
    throw std::runtime_error("enrollment is not ready to finalize");
  }
  return candidate_profile_.value();
}

EnrollmentStatus EnrollmentSession::mark_committed() {
  if (state_ != EnrollmentState::Ready || !candidate_profile_.has_value()) {
    throw std::runtime_error("enrollment is not ready to commit");
  }
  builder_.reset();
  candidate_profile_.reset();
  state_ = EnrollmentState::Committed;
  last_reason_ = "committed";
  return status();
}

EnrollmentStatus EnrollmentSession::status() const {
  EnrollmentStatus result;
  result.state = state_;
  result.accepted_samples = accepted_samples_;
  result.validation_samples = validation_sample_count();
  result.last_validation_similarity = last_validation_similarity_;
  result.lowest_validation_similarity = lowest_validation_similarity_;
  result.minimum_validation_similarity =
    policy_.minimum_validation_similarity;
  result.last_reason = last_reason_;

  const std::size_t validation_required =
    policy_.validation_samples_per_pose * kEnrollmentPoseCount;
  result.validation_progress_percent = static_cast<int>(
    (result.validation_samples * 100 + validation_required / 2) /
    validation_required
  );

  if (state_ == EnrollmentState::Collecting && builder_) {
    result.missing_poses = builder_->missing_poses();
  } else if (state_ == EnrollmentState::Validating) {
    result.missing_poses = missing_validation_poses();
  }

  if (state_ == EnrollmentState::Ready ||
      state_ == EnrollmentState::Committed) {
    result.progress_percent = 100;
  } else if (builder_) {
    const std::size_t training_required =
      policy_.target_per_pose * kEnrollmentPoseCount;
    const std::size_t total_required =
      training_required + validation_required;
    const int training_progress = builder_->progress_percent();
    result.progress_percent = static_cast<int>(
      (static_cast<std::size_t>(training_progress) * training_required +
       result.validation_samples * 100 + total_required / 2) /
      total_required
    );
  }
  return result;
}

}  // namespace face_unlock
