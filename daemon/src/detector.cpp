#include "detector.h"

#include <cmath>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if defined(FACE_UNLOCK_HAVE_OPENCV_OBJDETECT) || defined(FACE_UNLOCK_HAVE_OPENCV_YUNET)
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#endif

#ifdef FACE_UNLOCK_HAVE_OPENCV_YUNET
#include <opencv2/dnn.hpp>
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

bool regular_file_readable(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  return file.good();
}

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

#ifdef FACE_UNLOCK_HAVE_OPENCV_YUNET

class YuNetFaceDetector::Impl {
public:
  cv::Ptr<cv::FaceDetectorYN> detector;
};

YuNetFaceDetector::YuNetFaceDetector(const std::string& model_path)
    : model_path_(model_path) {
  if (model_path_.empty()) {
    throw std::runtime_error("yunet model path required");
  }

  if (!regular_file_readable(model_path_)) {
    throw std::runtime_error("yunet model not found: " + model_path_);
  }

  impl_ = std::make_shared<Impl>();
  impl_->detector = cv::FaceDetectorYN::create(
    model_path_,
    "",
    cv::Size(320, 320),
    0.9F,
    0.3F,
    5000,
    cv::dnn::DNN_BACKEND_OPENCV,
    cv::dnn::DNN_TARGET_CPU
  );

  if (impl_->detector.empty()) {
    throw std::runtime_error("failed to create yunet detector");
  }
}

std::string YuNetFaceDetector::backend_name() const {
  return "yunet";
}

DetectorResult YuNetFaceDetector::detect(const cv::Mat& frame) {
  DetectorResult result;
  result.backend = backend_name();

  if (frame.empty()) {
    return result;
  }

  impl_->detector->setInputSize(frame.size());

  cv::Mat faces;
  impl_->detector->detect(frame, faces);

  for (int row = 0; row < faces.rows; ++row) {
    if (faces.cols < 15) {
      throw std::runtime_error("unexpected yunet detection shape");
    }

    bool finite_row = true;

    for (int column = 0; column < 15; ++column) {
      if (!std::isfinite(faces.at<float>(row, column))) {
        finite_row = false;
        break;
      }
    }

    if (!finite_row) {
      continue;
    }

    DetectionBox box;
    box.x = cvRound(faces.at<float>(row, 0));
    box.y = cvRound(faces.at<float>(row, 1));
    box.w = cvRound(faces.at<float>(row, 2));
    box.h = cvRound(faces.at<float>(row, 3));
    box.score = static_cast<double>(faces.at<float>(row, 14));

    if (box.w <= 0 || box.h <= 0) {
      continue;
    }

    for (int index = 4; index < 14; index += 2) {
      box.landmarks.emplace_back(
        faces.at<float>(row, index),
        faces.at<float>(row, index + 1)
      );
    }

    result.boxes.push_back(std::move(box));
  }

  return result;
}

#endif

std::unique_ptr<FaceDetector> create_detector(
  const std::string& backend,
  const std::string& model_path
) {
  if (backend == "noop") {
    return std::make_unique<NoopFaceDetector>();
  }

#ifdef FACE_UNLOCK_HAVE_OPENCV_OBJDETECT
  if (backend == "haar") {
    return std::make_unique<HaarFaceDetector>();
  }
#endif

#ifdef FACE_UNLOCK_HAVE_OPENCV_YUNET
  if (backend == "yunet") {
    return std::make_unique<YuNetFaceDetector>(model_path);
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

#ifdef FACE_UNLOCK_HAVE_OPENCV_YUNET
  backends.push_back("yunet");
#endif

  return backends;
}

}  // namespace face_unlock
