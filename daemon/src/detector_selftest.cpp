#include "detector.h"

#include <iostream>
#include <memory>
#include <vector>

#include <opencv2/core.hpp>

int main() {
  const std::vector<std::string> backends =
    face_unlock::supported_detector_backends();

  bool saw_noop = false;

  for (const std::string& backend : backends) {
    std::cout << "supported_backend: " << backend << "\n";

    if (backend == "noop") {
      saw_noop = true;
    }
  }

  if (!saw_noop) {
    std::cerr << "status: failed\n";
    return 1;
  }

  std::unique_ptr<face_unlock::FaceDetector> detector =
    face_unlock::create_detector("noop");

  cv::Mat frame = cv::Mat::zeros(32, 32, CV_8UC3);

  const face_unlock::DetectorResult result = detector->detect(frame);

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
