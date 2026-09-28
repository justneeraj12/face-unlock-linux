#include "frame_quality.h"

#include <cmath>
#include <stdexcept>

#include <opencv2/imgproc.hpp>

namespace face_unlock {
namespace {

void validate_policy(const FrameQualityPolicy& policy) {
  if (policy.minimum_detection_score < 0.0 ||
      policy.minimum_detection_score > 1.0 ||
      policy.minimum_luma < 0.0 ||
      policy.maximum_luma > 255.0 ||
      policy.minimum_luma >= policy.maximum_luma ||
      policy.minimum_laplacian_variance < 0.0 ||
      policy.minimum_face_area_ratio <= 0.0 ||
      policy.maximum_face_area_ratio > 1.0 ||
      policy.minimum_face_area_ratio >= policy.maximum_face_area_ratio) {
    throw std::runtime_error("invalid frame quality policy");
  }
}

bool valid_landmarks(
  const DetectionBox& detection,
  int frame_width,
  int frame_height
) {
  if (detection.landmarks.size() != 5) {
    return false;
  }
  for (const cv::Point2f& point : detection.landmarks) {
    if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
        point.x < 0.0F || point.y < 0.0F ||
        point.x >= static_cast<float>(frame_width) ||
        point.y >= static_cast<float>(frame_height)) {
      return false;
    }
  }
  return true;
}

}  // namespace

FrameQualityResult evaluate_frame_quality(
  const cv::Mat& frame,
  const DetectorResult& detections,
  const FrameQualityPolicy& policy
) {
  validate_policy(policy);
  FrameQualityResult result;

  if (frame.empty() || (frame.channels() != 1 && frame.channels() != 3)) {
    result.reason = "invalid_frame";
    return result;
  }
  if (detections.boxes.empty()) {
    result.reason = "face_missing";
    return result;
  }
  if (detections.boxes.size() != 1) {
    result.reason = "multiple_faces";
    return result;
  }

  const DetectionBox& detection = detections.boxes.front();
  result.detection_score = detection.score;
  if (!std::isfinite(detection.score) ||
      detection.score < policy.minimum_detection_score) {
    result.reason = "detection_confidence_low";
    return result;
  }
  if (detection.x < 0 || detection.y < 0 ||
      detection.w <= 0 || detection.h <= 0 ||
      detection.x + detection.w > frame.cols ||
      detection.y + detection.h > frame.rows) {
    result.reason = "face_box_invalid";
    return result;
  }
  if (!valid_landmarks(detection, frame.cols, frame.rows)) {
    result.reason = "landmarks_invalid";
    return result;
  }

  const double frame_area =
    static_cast<double>(frame.cols) * static_cast<double>(frame.rows);
  result.face_area_ratio =
    static_cast<double>(detection.w) * static_cast<double>(detection.h) /
    frame_area;
  if (result.face_area_ratio < policy.minimum_face_area_ratio) {
    result.reason = "face_too_small";
    return result;
  }
  if (result.face_area_ratio > policy.maximum_face_area_ratio) {
    result.reason = "face_too_large";
    return result;
  }

  const cv::Rect face_region(
    detection.x,
    detection.y,
    detection.w,
    detection.h
  );
  cv::Mat gray;
  const cv::Mat face = frame(face_region);
  if (face.channels() == 3) {
    cv::cvtColor(face, gray, cv::COLOR_BGR2GRAY);
  } else {
    gray = face;
  }

  result.mean_luma = cv::mean(gray)[0];
  if (result.mean_luma < policy.minimum_luma) {
    result.reason = "low_light";
    return result;
  }
  if (result.mean_luma > policy.maximum_luma) {
    result.reason = "overexposed";
    return result;
  }

  cv::Mat laplacian;
  cv::Laplacian(gray, laplacian, CV_64F);
  cv::Scalar mean;
  cv::Scalar standard_deviation;
  cv::meanStdDev(laplacian, mean, standard_deviation);
  result.laplacian_variance =
    standard_deviation[0] * standard_deviation[0];
  if (!std::isfinite(result.laplacian_variance) ||
      result.laplacian_variance < policy.minimum_laplacian_variance) {
    result.reason = "blurred";
    return result;
  }

  result.approved = true;
  result.reason = "approved";
  return result;
}

}  // namespace face_unlock
