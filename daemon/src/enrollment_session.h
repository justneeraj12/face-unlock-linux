#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "face_profile.h"

namespace face_unlock {

enum class EnrollmentState {
  Idle,
  Collecting,
  Validating,
  Ready,
  Committed,
  Cancelled,
};

struct EnrollmentPolicy {
  std::size_t target_per_pose = 3;
  std::size_t maximum_per_pose = 12;
  std::size_t validation_samples_per_pose = 1;
  double duplicate_similarity = 0.9999;
  double minimum_quality = 0.5;
  double minimum_validation_similarity = 0.45;
};

struct EnrollmentStatus {
  EnrollmentState state = EnrollmentState::Idle;
  int progress_percent = 0;
  int validation_progress_percent = 0;
  std::vector<PoseSlot> missing_poses;
  std::size_t accepted_samples = 0;
  std::size_t validation_samples = 0;
  double last_validation_similarity = -1.0;
  double lowest_validation_similarity = -1.0;
  double minimum_validation_similarity = 0.45;
  std::string last_reason = "not_started";
};

class EnrollmentSession final {
public:
  explicit EnrollmentSession(
    EnrollmentPolicy policy = EnrollmentPolicy{}
  );

  EnrollmentStatus start(const std::string& model_id);
  AddSampleResult add_sample(
    const FaceEmbedding& embedding,
    PoseSlot pose,
    double quality_score
  );
  EnrollmentStatus cancel();
  FaceProfile finalize() const;
  EnrollmentStatus mark_committed();
  EnrollmentStatus status() const;

private:
  AddSampleResult add_validation_sample(
    const FaceEmbedding& embedding,
    PoseSlot pose,
    double quality_score
  );
  bool validation_complete() const;
  std::size_t validation_sample_count() const;
  std::vector<PoseSlot> missing_validation_poses() const;
  void clear_session_data();

  EnrollmentPolicy policy_;
  EnrollmentState state_ = EnrollmentState::Idle;
  std::unique_ptr<FaceProfileBuilder> builder_;
  std::optional<FaceProfile> candidate_profile_;
  std::array<std::size_t, 5> validation_counts_ {};
  std::size_t accepted_samples_ = 0;
  double last_validation_similarity_ = -1.0;
  double lowest_validation_similarity_ = -1.0;
  std::string last_reason_ = "not_started";
};

std::string enrollment_state_name(EnrollmentState state);

}  // namespace face_unlock
