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
        require(result.accepted, "valid enrollment sample rejected");
      }
    }

    status = session.status();
    require(status.state == face_unlock::EnrollmentState::Ready &&
            status.progress_percent == 100 &&
            status.missing_poses.empty() &&
            status.accepted_samples == 10,
            "pose-complete enrollment did not become ready");
    const auto profile = session.finalize();
    require(profile.poses.size() == 5 && profile.embedding_dim == 10,
            "finalized profile is invalid");
    status = session.mark_committed();
    require(status.state == face_unlock::EnrollmentState::Committed &&
            status.progress_percent == 100,
            "committed state is invalid");
    require(session.add_sample(
      embedding_for(0, 0), face_unlock::PoseSlot::Center, 1.0
    ).reason == "session_not_collecting",
            "committed session accepted another sample");

    session.start("sface-2021dec");
    status = session.cancel();
    require(status.state == face_unlock::EnrollmentState::Cancelled &&
            status.accepted_samples == 0,
            "cancel did not erase enrollment state");

    bool premature_finalize_rejected = false;
    try {
      (void)session.finalize();
    } catch (const std::runtime_error&) {
      premature_finalize_rejected = true;
    }
    require(premature_finalize_rejected,
            "incomplete enrollment finalized");

    std::cout << "enrollment_pose_coverage_status: ok\n";
    std::cout << "enrollment_cancel_erasure_status: ok\n";
    std::cout << "enrollment_commit_transition_status: ok\n";
    std::cout << "status: ok\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "status: failed\nerror: " << error.what() << '\n';
    return 1;
  }
}
