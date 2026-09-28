#pragma once

#include <memory>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "detector.h"

namespace face_unlock {

struct FaceEmbedding {
  std::string model;
  std::vector<float> values;
};

class SFaceEmbedder final {
public:
  explicit SFaceEmbedder(const std::string& model_path);
  ~SFaceEmbedder();

  SFaceEmbedder(const SFaceEmbedder&) = delete;
  SFaceEmbedder& operator=(const SFaceEmbedder&) = delete;

  FaceEmbedding embed_aligned(const cv::Mat& aligned_face_bgr);
  FaceEmbedding align_and_embed(
    const cv::Mat& frame_bgr,
    const DetectionBox& detection
  );
  static double cosine_similarity(
    const FaceEmbedding& left,
    const FaceEmbedding& right
  );

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace face_unlock
