#include "detector.h"

namespace face_unlock {

std::string NoopFaceDetector::backend_name() const {
  return "noop";
}

DetectorResult NoopFaceDetector::detect(const cv::Mat& frame) {
  (void)frame;

  DetectorResult result;
  result.backend = backend_name();
  return result;
}

}  // namespace face_unlock
