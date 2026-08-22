#pragma once

#include <memory>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace face_unlock {

struct DetectionBox {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  double score = 0.0;
};

struct DetectorResult {
  std::string backend = "none";
  std::vector<DetectionBox> boxes;
};

class FaceDetector {
public:
  virtual ~FaceDetector() = default;

  virtual std::string backend_name() const = 0;
  virtual DetectorResult detect(const cv::Mat& frame) = 0;
};

class NoopFaceDetector final : public FaceDetector {
public:
  std::string backend_name() const override;
  DetectorResult detect(const cv::Mat& frame) override;
};

#ifdef FACE_UNLOCK_HAVE_OPENCV_OBJDETECT
class HaarFaceDetector final : public FaceDetector {
public:
  explicit HaarFaceDetector(const std::string& cascade_path = "");

  std::string backend_name() const override;
  DetectorResult detect(const cv::Mat& frame) override;

private:
  std::string cascade_path_;
  class Impl;
  std::shared_ptr<Impl> impl_;
};
#endif

std::unique_ptr<FaceDetector> create_detector(const std::string& backend);

std::vector<std::string> supported_detector_backends();

}  // namespace face_unlock
