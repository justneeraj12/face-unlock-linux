#include "detector.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

int main(int argc, char** argv) {
  std::string yunet_model;

  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];

    if (argument == "--yunet-model" && index + 1 < argc) {
      yunet_model = argv[++index];
    } else {
      std::cerr << "unknown_argument: " << argument << '\n';
      return 2;
    }
  }

  const std::vector<std::string> backends =
    face_unlock::supported_detector_backends();

  bool saw_noop = false;
  bool saw_yunet = false;

  for (const std::string& backend : backends) {
    std::cout << "supported_backend: " << backend << '\n';

    if (backend == "noop") {
      saw_noop = true;
    } else if (backend == "yunet") {
      saw_yunet = true;
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

  std::cout << "detector_backend: " << result.backend << '\n';
  std::cout << "faces_detected: " << result.boxes.size() << '\n';

  if (result.backend != "noop" || !result.boxes.empty()) {
    std::cerr << "status: failed\n";
    return 1;
  }

  if (!yunet_model.empty()) {
    if (!saw_yunet) {
      std::cerr << "yunet_status: not_compiled\n";
      return 3;
    }

    try {
      detector = face_unlock::create_detector("yunet", yunet_model);
      frame = cv::Mat::zeros(320, 320, CV_8UC3);

      const auto started = std::chrono::steady_clock::now();
      const face_unlock::DetectorResult yunet_result = detector->detect(frame);
      const auto finished = std::chrono::steady_clock::now();
      const double elapsed_ms = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(
          finished - started
        ).count()
      ) / 1000.0;

      if (yunet_result.backend != "yunet" || !yunet_result.boxes.empty()) {
        std::cerr << "yunet_status: failed\n";
        return 4;
      }

      std::cout << "yunet_ms: " << elapsed_ms << '\n';
      std::cout << "yunet_status: ok\n";
    } catch (const std::exception& error) {
      std::cerr << "yunet_status: failed\n";
      std::cerr << "error: " << error.what() << '\n';
      return 4;
    }
  }

  std::cout << "status: ok\n";
  return 0;
}
