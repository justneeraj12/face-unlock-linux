#include "recognizer.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

namespace face_unlock {
namespace {
bool regular_file_readable(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  return file.good();
}

void require_bgr_image(const cv::Mat& image, const std::string& operation) {
  if (image.empty()) {
    throw std::runtime_error(operation + " requires a non-empty image");
  }
  if (image.type() != CV_8UC3) {
    throw std::runtime_error(operation + " requires an 8-bit BGR image");
  }
}

FaceEmbedding normalize_feature(const cv::Mat& feature) {
  if (feature.empty()) {
    throw std::runtime_error("sface returned an empty embedding");
  }

  cv::Mat float_feature;
  feature.convertTo(float_feature, CV_32F);
  const cv::Mat flattened = float_feature.reshape(1, 1);
  FaceEmbedding embedding;
  embedding.model = "opencv-sface-2021dec";
  embedding.values.reserve(flattened.total());
  double squared_norm = 0.0;

  for (std::size_t index = 0; index < flattened.total(); ++index) {
    const float value = flattened.at<float>(0, static_cast<int>(index));
    if (!std::isfinite(value)) {
      throw std::runtime_error("sface returned a non-finite embedding");
    }
    embedding.values.push_back(value);
    squared_norm += static_cast<double>(value) * value;
  }

  const double norm = std::sqrt(squared_norm);
  if (!std::isfinite(norm) || norm <= 0.0) {
    throw std::runtime_error("sface returned a zero-norm embedding");
  }

  for (float& value : embedding.values) {
    value = static_cast<float>(static_cast<double>(value) / norm);
  }
  return embedding;
}

cv::Mat detection_row(const DetectionBox& detection) {
  if (detection.w <= 0 || detection.h <= 0) {
    throw std::runtime_error("sface alignment requires a positive face box");
  }
  if (detection.landmarks.size() != 5) {
    throw std::runtime_error("sface alignment requires five landmarks");
  }

  cv::Mat face = cv::Mat::zeros(1, 15, CV_32F);
  face.at<float>(0, 0) = static_cast<float>(detection.x);
  face.at<float>(0, 1) = static_cast<float>(detection.y);
  face.at<float>(0, 2) = static_cast<float>(detection.w);
  face.at<float>(0, 3) = static_cast<float>(detection.h);

  for (std::size_t index = 0; index < detection.landmarks.size(); ++index) {
    const cv::Point2f& point = detection.landmarks[index];
    if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
      throw std::runtime_error("sface received a non-finite landmark");
    }
    face.at<float>(0, 4 + static_cast<int>(index) * 2) = point.x;
    face.at<float>(0, 5 + static_cast<int>(index) * 2) = point.y;
  }

  if (!std::isfinite(detection.score)) {
    throw std::runtime_error("sface received a non-finite score");
  }
  face.at<float>(0, 14) = static_cast<float>(detection.score);
  return face;
}
}  // namespace

class SFaceEmbedder::Impl {
public:
  cv::Ptr<cv::FaceRecognizerSF> recognizer;
};

SFaceEmbedder::SFaceEmbedder(const std::string& model_path) {
  if (model_path.empty()) {
    throw std::runtime_error("sface model path required");
  }
  if (!regular_file_readable(model_path)) {
    throw std::runtime_error("sface model not found: " + model_path);
  }
  impl_ = std::make_unique<Impl>();
  impl_->recognizer = cv::FaceRecognizerSF::create(
    model_path, "", cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_CPU
  );
  if (impl_->recognizer.empty()) {
    throw std::runtime_error("failed to create sface embedder");
  }
}

SFaceEmbedder::~SFaceEmbedder() = default;

FaceEmbedding SFaceEmbedder::embed_aligned(const cv::Mat& aligned_face_bgr) {
  require_bgr_image(aligned_face_bgr, "sface embedding");
  cv::Mat input;
  if (aligned_face_bgr.size() == cv::Size(112, 112)) {
    input = aligned_face_bgr;
  } else {
    cv::resize(aligned_face_bgr, input, cv::Size(112, 112));
  }
  cv::Mat feature;
  impl_->recognizer->feature(input, feature);
  return normalize_feature(feature);
}

std::string SFaceEmbedder::model_id() const {
  return "opencv-sface-2021dec";
}

FaceEmbedding SFaceEmbedder::align_and_embed(
  const cv::Mat& frame_bgr,
  const DetectionBox& detection
) {
  require_bgr_image(frame_bgr, "sface alignment");
  cv::Mat aligned;
  impl_->recognizer->alignCrop(frame_bgr, detection_row(detection), aligned);
  return embed_aligned(aligned);
}

double SFaceEmbedder::cosine_similarity(
  const FaceEmbedding& left,
  const FaceEmbedding& right
) {
  if (left.model.empty() || left.model != right.model) {
    throw std::runtime_error("embedding models do not match");
  }
  if (left.values.empty() || left.values.size() != right.values.size()) {
    throw std::runtime_error("embedding dimensions do not match");
  }
  double dot = 0.0, left_norm = 0.0, right_norm = 0.0;
  for (std::size_t index = 0; index < left.values.size(); ++index) {
    const double l = left.values[index], r = right.values[index];
    if (!std::isfinite(l) || !std::isfinite(r)) {
      throw std::runtime_error("embedding contains non-finite values");
    }
    dot += l * r;
    left_norm += l * l;
    right_norm += r * r;
  }
  if (left_norm <= 0.0 || right_norm <= 0.0) {
    throw std::runtime_error("embedding norm must be positive");
  }
  const double similarity = dot / std::sqrt(left_norm * right_norm);
  return std::clamp(similarity, -1.0, 1.0);
}
}  // namespace face_unlock
