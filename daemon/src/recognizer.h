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

class FaceEmbedder {
public:
  virtual ~FaceEmbedder() = default;
  virtual std::string model_id() const = 0;
  virtual FaceEmbedding align_and_embed(
    const cv::Mat& frame_bgr,
    const DetectionBox& detection
  ) = 0;
};

class SFaceEmbedder final : public FaceEmbedder {
public:
  explicit SFaceEmbedder(const std::string& model_path);
  ~SFaceEmbedder();

  SFaceEmbedder(const SFaceEmbedder&) = delete;
  SFaceEmbedder& operator=(const SFaceEmbedder&) = delete;

  FaceEmbedding embed_aligned(const cv::Mat& aligned_face_bgr);
  std::string model_id() const override;
  FaceEmbedding align_and_embed(
    const cv::Mat& frame_bgr,
    const DetectionBox& detection
  ) override;
  static double cosine_similarity(
    const FaceEmbedding& left,
    const FaceEmbedding& right
  );

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace face_unlock
