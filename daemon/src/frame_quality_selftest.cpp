#include "frame_quality.h"

#include <exception>
#include <iostream>
#include <stdexcept>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

face_unlock::DetectionBox valid_box() {
  face_unlock::DetectionBox box;
  box.x = 80;
  box.y = 60;
  box.w = 160;
  box.h = 180;
  box.score = 0.95;
  box.landmarks = {
    {120.0F, 120.0F}, {200.0F, 120.0F}, {160.0F, 155.0F},
    {130.0F, 195.0F}, {190.0F, 195.0F},
  };
  return box;
}

cv::Mat checker_frame(int low = 60, int high = 180) {
  cv::Mat frame(300, 320, CV_8UC3);
  for (int y = 0; y < frame.rows; ++y) {
    for (int x = 0; x < frame.cols; ++x) {
      const int value = ((x / 8 + y / 8) % 2 == 0) ? low : high;
      frame.at<cv::Vec3b>(y, x) = cv::Vec3b(value, value, value);
    }
  }
  return frame;
}

}  // namespace

int main() {
  try {
    face_unlock::DetectorResult detections;
    detections.backend = "test";
    detections.boxes.push_back(valid_box());

    const auto approved = face_unlock::evaluate_frame_quality(
      checker_frame(), detections
    );
    require(approved.approved && approved.reason == "approved",
            "valid frame rejected");

    const auto dark = face_unlock::evaluate_frame_quality(
      cv::Mat::zeros(300, 320, CV_8UC3), detections
    );
    require(!dark.approved && dark.reason == "low_light",
            "dark frame accepted");

    const auto bright = face_unlock::evaluate_frame_quality(
      cv::Mat(300, 320, CV_8UC3, cv::Scalar(250, 250, 250)), detections
    );
    require(!bright.approved && bright.reason == "overexposed",
            "overexposed frame accepted");

    const auto blurred = face_unlock::evaluate_frame_quality(
      cv::Mat(300, 320, CV_8UC3, cv::Scalar(120, 120, 120)), detections
    );
    require(!blurred.approved && blurred.reason == "blurred",
            "blurred frame accepted");

    face_unlock::DetectorResult multiple = detections;
    multiple.boxes.push_back(valid_box());
    require(face_unlock::evaluate_frame_quality(
      checker_frame(), multiple
    ).reason == "multiple_faces", "multiple faces accepted");

    face_unlock::DetectorResult small = detections;
    small.boxes.front().w = 20;
    small.boxes.front().h = 20;
    require(face_unlock::evaluate_frame_quality(
      checker_frame(), small
    ).reason == "face_too_small", "small face accepted");

    face_unlock::DetectorResult no_landmarks = detections;
    no_landmarks.boxes.front().landmarks.clear();
    require(face_unlock::evaluate_frame_quality(
      checker_frame(), no_landmarks
    ).reason == "landmarks_invalid", "missing landmarks accepted");

    bool invalid_policy_rejected = false;
    try {
      face_unlock::FrameQualityPolicy invalid;
      invalid.minimum_luma = 230.0;
      invalid.maximum_luma = 220.0;
      (void)face_unlock::evaluate_frame_quality(
        checker_frame(), detections, invalid
      );
    } catch (const std::runtime_error&) {
      invalid_policy_rejected = true;
    }
    require(invalid_policy_rejected, "invalid quality policy accepted");

    std::cout << "quality_gate_single_face_status: ok\n";
    std::cout << "quality_gate_lighting_status: ok\n";
    std::cout << "quality_gate_sharpness_status: ok\n";
    std::cout << "quality_gate_geometry_status: ok\n";
    std::cout << "status: ok\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "status: failed\nerror: " << error.what() << '\n';
    return 1;
  }
}
