#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "detector.h"

namespace face_unlock {

struct FrameQualityPolicy {
  double minimum_detection_score = 0.80;
  double minimum_luma = 35.0;
  double maximum_luma = 225.0;
  double minimum_laplacian_variance = 20.0;
  double minimum_face_area_ratio = 0.04;
  double maximum_face_area_ratio = 0.70;
};

struct FrameQualityResult {
  bool approved = false;
  std::string reason = "not_evaluated";
  double mean_luma = 0.0;
  double laplacian_variance = 0.0;
  double face_area_ratio = 0.0;
  double detection_score = 0.0;
};

FrameQualityResult evaluate_frame_quality(
  const cv::Mat& frame,
  const DetectorResult& detections,
  const FrameQualityPolicy& policy = FrameQualityPolicy{}
);

}  // namespace face_unlock
