#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "face_profile.h"

namespace face_unlock {

enum class EnrollmentState {
  Idle,
  Collecting,
  Ready,
  Committed,
  Cancelled,
};

struct EnrollmentPolicy {
  std::size_t target_per_pose = 3;
  std::size_t maximum_per_pose = 12;
  double duplicate_similarity = 0.9999;
  double minimum_quality = 0.5;
};

struct EnrollmentStatus {
  EnrollmentState state = EnrollmentState::Idle;
  int progress_percent = 0;
  std::vector<PoseSlot> missing_poses;
  std::size_t accepted_samples = 0;
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
  EnrollmentPolicy policy_;
  EnrollmentState state_ = EnrollmentState::Idle;
  std::unique_ptr<FaceProfileBuilder> builder_;
  std::size_t accepted_samples_ = 0;
  std::string last_reason_ = "not_started";
};

std::string enrollment_state_name(EnrollmentState state);

}  // namespace face_unlock
