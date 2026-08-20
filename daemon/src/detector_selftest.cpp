#include "detector.h"

#include <iostream>

#include <opencv2/core.hpp>

int main() {
  face_unlock::NoopFaceDetector detector;

  cv::Mat frame = cv::Mat::zeros(32, 32, CV_8UC3);

  const face_unlock::DetectorResult result = detector.detect(frame);

  std::cout << "detector_backend: " << result.backend << "\n";
  std::cout << "faces_detected: " << result.boxes.size() << "\n";

  if (result.backend != "noop") {
    std::cerr << "status: failed\n";
    return 1;
  }

  if (!result.boxes.empty()) {
    std::cerr << "status: failed\n";
    return 1;
  }

  std::cout << "status: ok\n";
  return 0;
}
