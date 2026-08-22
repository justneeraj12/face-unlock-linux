#include "detector.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef FACE_UNLOCK_HAVE_OPENCV_OBJDETECT
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#endif

namespace face_unlock {
namespace {

#ifdef FACE_UNLOCK_HAVE_OPENCV_OBJDETECT
std::string default_haar_cascade_path() {
  const std::vector<std::string> candidates = {
    "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
    "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml",
    "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
    "/usr/local/share/opencv/haarcascades/haarcascade_frontalface_default.xml",
  };

  for (const std::string& candidate : candidates) {
    cv::CascadeClassifier classifier;

    if (classifier.load(candidate)) {
      return candidate;
    }
  }

  return "";
}
#endif

}  // namespace

std::string NoopFaceDetector::backend_name() const {
  return "noop";
}

DetectorResult NoopFaceDetector::detect(const cv::Mat& frame) {
  (void)frame;

  DetectorResult result;
  result.backend = backend_name();
  return result;
}

#ifdef FACE_UNLOCK_HAVE_OPENCV_OBJDETECT

class HaarFaceDetector::Impl {
public:
  cv::CascadeClassifier classifier;
};

HaarFaceDetector::HaarFaceDetector(const std::string& cascade_path) {
  cascade_path_ = cascade_path.empty() ? default_haar_cascade_path() : cascade_path;

  if (cascade_path_.empty()) {
    throw std::runtime_error("haar cascade not found");
  }

  impl_ = std::make_shared<Impl>();

  if (!impl_->classifier.load(cascade_path_)) {
    throw std::runtime_error("failed to load haar cascade: " + cascade_path_);
  }
}

std::string HaarFaceDetector::backend_name() const {
  return "haar";
}

DetectorResult HaarFaceDetector::detect(const cv::Mat& frame) {
  DetectorResult result;
  result.backend = backend_name();

  if (frame.empty()) {
    return result;
  }

  cv::Mat gray;
  cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

  std::vector<cv::Rect> faces;

  impl_->classifier.detectMultiScale(
    gray,
    faces,
    1.1,
    5,
    0,
    cv::Size(60, 60)
  );

  for (const cv::Rect& face : faces) {
    DetectionBox box;
    box.x = face.x;
    box.y = face.y;
    box.w = face.width;
    box.h = face.height;
    box.score = 1.0;
    result.boxes.push_back(box);
  }

  return result;
}

#endif

std::unique_ptr<FaceDetector> create_detector(const std::string& backend) {
  if (backend == "noop") {
    return std::make_unique<NoopFaceDetector>();
  }

#ifdef FACE_UNLOCK_HAVE_OPENCV_OBJDETECT
  if (backend == "haar") {
    return std::make_unique<HaarFaceDetector>();
  }
#endif

  throw std::runtime_error("unsupported detector backend: " + backend);
}

std::vector<std::string> supported_detector_backends() {
  std::vector<std::string> backends = {
    "noop",
  };

#ifdef FACE_UNLOCK_HAVE_OPENCV_OBJDETECT
  backends.push_back("haar");
#endif

  return backends;
}

}  // namespace face_unlock
