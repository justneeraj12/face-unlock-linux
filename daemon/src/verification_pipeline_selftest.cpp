#include "verification_pipeline.h"

#include <exception>
#include <iostream>
#include <stdexcept>

#include <opencv2/core.hpp>

namespace {

class FixedDetector final : public face_unlock::FaceDetector {
public:
  std::string backend_name() const override { return "fixed"; }

  face_unlock::DetectorResult detect(const cv::Mat&) override {
    return result;
  }

  face_unlock::DetectorResult result;
};

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

cv::Mat checker_frame() {
  cv::Mat frame(300, 320, CV_8UC3);
  for (int y = 0; y < frame.rows; ++y) {
    for (int x = 0; x < frame.cols; ++x) {
      const unsigned char value =
        ((x / 8 + y / 8) % 2 == 0) ? 60 : 180;
      frame.at<cv::Vec3b>(y, x) = cv::Vec3b(value, value, value);
    }
  }
  return frame;
}

face_unlock::DetectionBox detection() {
  face_unlock::DetectionBox box;
  box.x = 80; box.y = 60; box.w = 160; box.h = 180; box.score = 0.95;
  box.landmarks = {
    {120.0F, 120.0F}, {200.0F, 120.0F}, {160.0F, 155.0F},
    {130.0F, 195.0F}, {190.0F, 195.0F},
  };
  return box;
}

face_unlock::FaceProfile profile() {
  face_unlock::FaceProfile result;
  result.model_id = "test-model";
  result.embedding_dim = 2;
  for (int index = 0; index < 5; ++index) {
    face_unlock::PoseTemplate pose;
    pose.pose = static_cast<face_unlock::PoseSlot>(index);
    pose.sample_count = 3;
    pose.centroid = index == 0 ?
      std::vector<float>{1.0F, 0.0F} :
      std::vector<float>{0.0F, 1.0F};
    result.poses.push_back(pose);
  }
  return result;
}

}  // namespace

int main() {
  try {
    FixedDetector detector;
    detector.result.backend = "fixed";
    detector.result.boxes.push_back(detection());
    const face_unlock::FaceProfile enrolled = profile();
    int embedding_calls = 0;

    face_unlock::DiagnosticVerificationPipeline pipeline(
      detector,
      [&embedding_calls](const cv::Mat&, const face_unlock::DetectionBox&) {
        ++embedding_calls;
        return face_unlock::FaceEmbedding{
          "test-model", {1.0F, 0.0F}
        };
      },
      enrolled
    );

    const auto scored = pipeline.evaluate(checker_frame());
    require(scored.status == "score_available" &&
            scored.score_available &&
            scored.match.pose == face_unlock::PoseSlot::Center &&
            scored.match.similarity > 0.999 &&
            !scored.authentication_permitted,
            "diagnostic score pipeline failed closed incorrectly");
    require(embedding_calls == 1, "embedding call count is wrong");

    detector.result.boxes.clear();
    const auto rejected = pipeline.evaluate(checker_frame());
    require(rejected.status == "rejected" &&
            rejected.reason == "face_missing" &&
            !rejected.score_available &&
            embedding_calls == 1,
            "quality rejection reached embedding stage");

    detector.result.boxes.push_back(detection());
    face_unlock::DiagnosticVerificationPipeline failing(
      detector,
      [](const cv::Mat&, const face_unlock::DetectionBox&)
          -> face_unlock::FaceEmbedding {
        throw std::runtime_error("injected embedding failure");
      },
      enrolled
    );
    const auto error = failing.evaluate(checker_frame());
    require(error.status == "error" &&
            error.reason == "pipeline_error" &&
            !error.authentication_permitted,
            "pipeline error did not fail closed");

    std::cout << "verification_quality_boundary_status: ok\n";
    std::cout << "verification_profile_score_status: ok\n";
    std::cout << "verification_threshold_status: disabled\n";
    std::cout << "verification_authentication_permitted: false\n";
    std::cout << "status: ok\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "status: failed\nerror: " << error.what() << '\n';
    return 1;
  }
}
