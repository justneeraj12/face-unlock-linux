#include "enrollment_session.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

face_unlock::FaceEmbedding embedding_for(int pose, int sample) {
  std::vector<float> values(10, 0.0F);
  values[static_cast<std::size_t>(pose)] = 1.0F;
  values[static_cast<std::size_t>(5 + sample)] = 0.20F;
  double squared = 0.0;
  for (float value : values) squared += value * value;
  const float norm = static_cast<float>(std::sqrt(squared));
  for (float& value : values) value /= norm;
  return {"sface-2021dec", values};
}

}  // namespace

int main() {
  try {
    face_unlock::EnrollmentPolicy policy;
    policy.target_per_pose = 2;
    policy.minimum_validation_similarity = 0.8;
    face_unlock::EnrollmentSession session(policy);
    auto status = session.start("sface-2021dec");
    require(status.state == face_unlock::EnrollmentState::Collecting &&
            status.progress_percent == 0,
            "enrollment did not start cleanly");

    for (int pose = 0; pose < 5; ++pose) {
      for (int sample = 0; sample < 2; ++sample) {
        const auto result = session.add_sample(
          embedding_for(pose, sample),
          static_cast<face_unlock::PoseSlot>(pose),
          0.9
        );
        require(result.accepted, "valid training sample rejected");
      }
    }

    status = session.status();
    require(status.state == face_unlock::EnrollmentState::Validating &&
            status.progress_percent == 67 &&
            status.validation_samples == 0 &&
            status.missing_poses.size() == 5,
            "training did not transition to held-out validation");

    for (int pose = 0; pose < 5; ++pose) {
      const auto result = session.add_sample(
        embedding_for(pose, 2),
        static_cast<face_unlock::PoseSlot>(pose),
        0.9
      );
      require(result.accepted, "valid held-out sample rejected");
    }

    status = session.status();
    require(status.state == face_unlock::EnrollmentState::Ready &&
            status.progress_percent == 100 &&
            status.validation_progress_percent == 100 &&
            status.validation_samples == 5 &&
            status.lowest_validation_similarity >= 0.8 &&
            status.missing_poses.empty() &&
            status.accepted_samples == 15,
            "held-out validation did not make enrollment ready");
    const auto profile = session.finalize();
    require(profile.poses.size() == 5 && profile.embedding_dim == 10 &&
            profile.poses.front().sample_count == 2,
            "held-out samples leaked into the training profile");
    status = session.mark_committed();
    require(status.state == face_unlock::EnrollmentState::Committed &&
            status.progress_percent == 100,
            "committed state is invalid");
    require(session.add_sample(
      embedding_for(0, 0), face_unlock::PoseSlot::Center, 1.0
    ).reason == "session_not_collecting",
            "committed session accepted another sample");

    face_unlock::EnrollmentSession inconsistent(policy);
    inconsistent.start("sface-2021dec");
    for (int pose = 0; pose < 5; ++pose) {
      for (int sample = 0; sample < 2; ++sample) {
        require(inconsistent.add_sample(
          embedding_for(pose, sample),
          static_cast<face_unlock::PoseSlot>(pose),
          0.9
        ).accepted, "inconsistent-session training failed");
      }
    }
    const auto rejected = inconsistent.add_sample(
      embedding_for(4, 2),
      face_unlock::PoseSlot::Center,
      0.9
    );
    require(!rejected.accepted &&
            rejected.reason == "heldout_similarity_low" &&
            inconsistent.status().state ==
              face_unlock::EnrollmentState::Validating,
            "inconsistent held-out sample was accepted");

    status = inconsistent.cancel();
    require(status.state == face_unlock::EnrollmentState::Cancelled &&
            status.accepted_samples == 0 &&
            status.validation_samples == 0,
            "cancel did not erase enrollment state");

    bool premature_finalize_rejected = false;
    try {
      (void)inconsistent.finalize();
    } catch (const std::runtime_error&) {
      premature_finalize_rejected = true;
    }
    require(premature_finalize_rejected,
            "incomplete enrollment finalized");

    std::cout << "enrollment_pose_coverage_status: ok\n";
    std::cout << "enrollment_heldout_validation_status: ok\n";
    std::cout << "enrollment_inconsistency_rejection_status: ok\n";
    std::cout << "enrollment_cancel_erasure_status: ok\n";
    std::cout << "enrollment_commit_transition_status: ok\n";
    std::cout << "status: ok\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "status: failed\nerror: " << error.what() << '\n';
    return 1;
  }
}
