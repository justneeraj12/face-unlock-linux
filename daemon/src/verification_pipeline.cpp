#include "verification_pipeline.h"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace face_unlock {
namespace {

using Clock = std::chrono::steady_clock;

double milliseconds(
  const Clock::time_point& start,
  const Clock::time_point& finish
) {
  return static_cast<double>(
    std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count()
  ) / 1000.0;
}

}  // namespace

DiagnosticVerificationPipeline::DiagnosticVerificationPipeline(
  FaceDetector& detector,
  EmbeddingFunction embedding_function,
  const FaceProfile& profile,
  FrameQualityPolicy quality_policy
) :
    detector_(detector),
    embedding_function_(std::move(embedding_function)),
    profile_(profile),
    quality_policy_(quality_policy) {
  if (!embedding_function_) {
    throw std::runtime_error("embedding function is required");
  }
  validate_face_profile(profile_);
}

DiagnosticVerificationResult DiagnosticVerificationPipeline::evaluate(
  const cv::Mat& frame
) {
  DiagnosticVerificationResult result;
  const auto total_started = Clock::now();

  try {
    const auto detector_started = Clock::now();
    const DetectorResult detections = detector_.detect(frame);
    const auto detector_finished = Clock::now();
    result.detector_ms = milliseconds(detector_started, detector_finished);

    result.quality = evaluate_frame_quality(
      frame,
      detections,
      quality_policy_
    );
    if (!result.quality.approved) {
      result.status = "rejected";
      result.reason = result.quality.reason;
      result.total_ms = milliseconds(total_started, Clock::now());
      return result;
    }

    const auto embedding_started = Clock::now();
    const FaceEmbedding embedding = embedding_function_(
      frame,
      detections.boxes.front()
    );
    const auto embedding_finished = Clock::now();
    result.embedding_ms = milliseconds(embedding_started, embedding_finished);

    const auto scoring_started = Clock::now();
    result.match = profile_.score(embedding);
    const auto scoring_finished = Clock::now();
    result.scoring_ms = milliseconds(scoring_started, scoring_finished);
    result.score_available = true;
    result.status = "score_available";
    result.reason = "threshold_not_calibrated";
  } catch (const std::exception&) {
    result.status = "error";
    result.reason = "pipeline_error";
    result.score_available = false;
  }

  result.authentication_permitted = false;
  result.total_ms = milliseconds(total_started, Clock::now());
  return result;
}

}  // namespace face_unlock
