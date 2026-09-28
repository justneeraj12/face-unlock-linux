#include "recognizer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace {
double percentile(std::vector<double> values, double fraction) {
  std::sort(values.begin(), values.end());
  const double position = fraction * static_cast<double>(values.size() - 1);
  const std::size_t index = static_cast<std::size_t>(std::ceil(position));
  return values[std::min(index, values.size() - 1)];
}

double embedding_norm(const face_unlock::FaceEmbedding& embedding) {
  double squared = 0.0;
  for (const float value : embedding.values) {
    squared += static_cast<double>(value) * value;
  }
  return std::sqrt(squared);
}

template <typename Function>
bool throws_runtime_error(Function&& function) {
  try {
    function();
  } catch (const std::runtime_error&) {
    return true;
  }
  return false;
}
}  // namespace

int main(int argc, char** argv) {
  std::string model_path;
  int iterations = 20;
  int warmup = 3;

  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--sface-model" && index + 1 < argc) {
      model_path = argv[++index];
    } else if (argument == "--iterations" && index + 1 < argc) {
      iterations = std::atoi(argv[++index]);
    } else if (argument == "--warmup" && index + 1 < argc) {
      warmup = std::atoi(argv[++index]);
    } else {
      std::cerr << "unknown_argument: " << argument << '\n';
      return 2;
    }
  }

  const face_unlock::FaceEmbedding unit_x{"test", {1.0F, 0.0F}};
  const face_unlock::FaceEmbedding unit_y{"test", {0.0F, 1.0F}};
  const face_unlock::FaceEmbedding other_model{"other", {1.0F, 0.0F}};
  const face_unlock::FaceEmbedding wrong_size{"test", {1.0F}};

  if (std::abs(face_unlock::SFaceEmbedder::cosine_similarity(
        unit_x, unit_x
      ) - 1.0) > 0.0001 ||
      std::abs(face_unlock::SFaceEmbedder::cosine_similarity(
        unit_x, unit_y
      )) > 0.0001 ||
      !throws_runtime_error([&]() {
        (void)face_unlock::SFaceEmbedder::cosine_similarity(
          unit_x, other_model
        );
      }) ||
      !throws_runtime_error([&]() {
        (void)face_unlock::SFaceEmbedder::cosine_similarity(
          unit_x, wrong_size
        );
      })) {
    std::cerr << "error: cosine validation failed\n";
    return 3;
  }

  std::cout << "supported_recognizer: sface\n";
  std::cout << "cosine_validation_status: ok\n";
  if (model_path.empty()) {
    std::cout << "sface_status: model_not_requested\nstatus: ok\n";
    return 0;
  }
  if (iterations < 1 || warmup < 0) {
    std::cerr << "error: invalid iteration counts\n";
    return 2;
  }

  try {
    const auto load_started = std::chrono::steady_clock::now();
    face_unlock::SFaceEmbedder embedder(model_path);
    const auto load_finished = std::chrono::steady_clock::now();
    const double load_ms = static_cast<double>(
      std::chrono::duration_cast<std::chrono::microseconds>(
        load_finished - load_started
      ).count()) / 1000.0;

    const cv::Mat aligned = cv::Mat::zeros(112, 112, CV_8UC3);
    for (int index = 0; index < warmup; ++index) {
      (void)embedder.embed_aligned(aligned);
    }

    std::vector<double> elapsed_values;
    face_unlock::FaceEmbedding embedding;
    for (int index = 0; index < iterations; ++index) {
      const auto started = std::chrono::steady_clock::now();
      embedding = embedder.embed_aligned(aligned);
      const auto finished = std::chrono::steady_clock::now();
      elapsed_values.push_back(static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(
          finished - started
        ).count()) / 1000.0);
    }

    const double norm = embedding_norm(embedding);
    const double similarity =
      face_unlock::SFaceEmbedder::cosine_similarity(embedding, embedding);
    if (embedding.values.size() != 128 ||
        std::abs(norm - 1.0) > 0.0001 ||
        std::abs(similarity - 1.0) > 0.0001) {
      std::cerr << "error: invalid normalized embedding\n";
      return 3;
    }

    face_unlock::DetectionBox detection;
    detection.x = 80; detection.y = 60; detection.w = 160; detection.h = 200;
    detection.score = 1.0;
    detection.landmarks = {
      {120.0F, 125.0F}, {200.0F, 125.0F}, {160.0F, 165.0F},
      {130.0F, 210.0F}, {190.0F, 210.0F},
    };
    const auto aligned_embedding = embedder.align_and_embed(
      cv::Mat::zeros(320, 320, CV_8UC3), detection
    );
    if (aligned_embedding.values.size() != embedding.values.size()) {
      std::cerr << "error: aligned embedding dimension mismatch\n";
      return 3;
    }

    std::cout << "sface_model_load_ms: " << load_ms << '\n';
    std::cout << "sface_embedding_dim: " << embedding.values.size() << '\n';
    std::cout << "sface_embedding_norm: " << norm << '\n';
    std::cout << "sface_p50_ms: " << percentile(elapsed_values, 0.50) << '\n';
    std::cout << "sface_p95_ms: " << percentile(elapsed_values, 0.95) << '\n';
    std::cout << "sface_alignment_status: ok\n";
    std::cout << "sface_status: ok\nstatus: ok\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "sface_status: failed\nerror: " << error.what() << '\n';
    return 3;
  }
}
