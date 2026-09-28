#pragma once

#include <functional>
#include <string>

#include <opencv2/core.hpp>

#include "detector.h"
#include "face_profile.h"
#include "frame_quality.h"
#include "recognizer.h"

namespace face_unlock {

using EmbeddingFunction = std::function<FaceEmbedding(
  const cv::Mat&,
  const DetectionBox&
)>;

struct DiagnosticVerificationResult {
  std::string status = "not_evaluated";
  std::string reason = "not_evaluated";
  FrameQualityResult quality;
  ProfileMatch match;
  bool score_available = false;
  bool authentication_permitted = false;
  double detector_ms = 0.0;
  double embedding_ms = 0.0;
  double scoring_ms = 0.0;
  double total_ms = 0.0;
};

class DiagnosticVerificationPipeline final {
public:
  DiagnosticVerificationPipeline(
    FaceDetector& detector,
    EmbeddingFunction embedding_function,
    const FaceProfile& profile,
    FrameQualityPolicy quality_policy = FrameQualityPolicy{}
  );

  DiagnosticVerificationResult evaluate(const cv::Mat& frame);

private:
  FaceDetector& detector_;
  EmbeddingFunction embedding_function_;
  const FaceProfile& profile_;
  FrameQualityPolicy quality_policy_;
};

}  // namespace face_unlock
