#include "camera_lease.h"
#include "detector.h"
#include "face_profile.h"
#include "frame_quality.h"
#include "recognizer.h"
#include "verification_pipeline.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

double percentile(std::vector<double> values, double fraction) {
  std::sort(values.begin(), values.end());
  const double position = fraction * static_cast<double>(values.size() - 1);
  const std::size_t index = static_cast<std::size_t>(std::ceil(position));
  return values[std::min(index, values.size() - 1)];
}

face_unlock::FaceProfile in_memory_profile(
  const face_unlock::FaceEmbedding& embedding
) {
  face_unlock::FaceProfile profile;
  profile.model_id = embedding.model;
  profile.embedding_dim = embedding.values.size();
  for (int index = 0; index < 5; ++index) {
    face_unlock::PoseTemplate pose;
    pose.pose = static_cast<face_unlock::PoseSlot>(index);
    pose.sample_count = 1;
    pose.centroid = embedding.values;
    profile.poses.push_back(std::move(pose));
  }
  face_unlock::validate_face_profile(profile);
  return profile;
}

}  // namespace

int main(int argc, char** argv) {
  std::string yunet_model;
  std::string sface_model;
  int camera_index = 0;
  int iterations = 10;

  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--yunet-model" && index + 1 < argc) {
      yunet_model = argv[++index];
    } else if (argument == "--sface-model" && index + 1 < argc) {
      sface_model = argv[++index];
    } else if (argument == "--camera" && index + 1 < argc) {
      camera_index = std::atoi(argv[++index]);
    } else if (argument == "--iterations" && index + 1 < argc) {
      iterations = std::atoi(argv[++index]);
    } else {
      std::cerr << "unknown_argument: " << argument << '\n';
      return 2;
    }
  }

  if (yunet_model.empty() || sface_model.empty() ||
      camera_index < 0 || iterations < 1 || iterations > 100) {
    std::cerr << "usage: native-verification-benchmark --yunet-model PATH "
              << "--sface-model PATH [--camera INDEX] [--iterations N]\n";
    return 2;
  }

  try {
    face_unlock::CameraLeaseManager camera(camera_index);
    const auto start = camera.start();
    if (!start.accepted) {
      throw std::runtime_error("camera lease rejected");
    }

    cv::Mat frame;
    unsigned long long frames_total = 0;
    const auto capture_deadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(3200);
    while (std::chrono::steady_clock::now() < capture_deadline) {
      if (camera.snapshot(frame, frames_total)) {
        break;
      }
      const auto status = camera.status();
      if (status.state == face_unlock::CameraLeaseState::Failed ||
          (status.state == face_unlock::CameraLeaseState::Idle &&
           status.generation > 0)) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    const auto camera_status = camera.status();
    const auto stopped = camera.stop("benchmark_frame_captured");
    if (!stopped.released) {
      throw std::runtime_error("camera did not release after capture");
    }
    if (frame.empty()) {
      std::cout << "benchmark_status: unavailable\n";
      std::cout << "reason: " << camera_status.release_reason << '\n';
      return 0;
    }

    std::unique_ptr<face_unlock::FaceDetector> detector =
      face_unlock::create_detector("yunet", yunet_model);
    face_unlock::SFaceEmbedder embedder(sface_model);

    const face_unlock::DetectorResult initial_detections =
      detector->detect(frame);
    const face_unlock::FrameQualityResult initial_quality =
      face_unlock::evaluate_frame_quality(frame, initial_detections);
    if (!initial_quality.approved) {
      std::cout << "benchmark_status: unavailable\n";
      std::cout << "reason: " << initial_quality.reason << '\n';
      std::cout << "camera_open_ms: " << camera_status.open_latency_ms << '\n';
      std::cout << "camera_first_frame_ms: "
                << camera_status.first_frame_latency_ms << '\n';
      return 0;
    }

    const face_unlock::FaceEmbedding seed = embedder.align_and_embed(
      frame,
      initial_detections.boxes.front()
    );
    const face_unlock::FaceProfile profile = in_memory_profile(seed);
    face_unlock::DiagnosticVerificationPipeline pipeline(
      *detector,
      [&embedder](
        const cv::Mat& input,
        const face_unlock::DetectionBox& detection
      ) {
        return embedder.align_and_embed(input, detection);
      },
      profile
    );

    std::vector<double> total_values;
    std::vector<double> detector_values;
    std::vector<double> embedding_values;
    for (int index = 0; index < iterations; ++index) {
      const auto result = pipeline.evaluate(frame);
      if (!result.score_available || result.authentication_permitted) {
        throw std::runtime_error("diagnostic pipeline did not remain fail closed");
      }
      total_values.push_back(result.total_ms);
      detector_values.push_back(result.detector_ms);
      embedding_values.push_back(result.embedding_ms);
    }

    std::cout << "privacy_status: frame_memory_only\n";
    std::cout << "authentication_permitted: false\n";
    std::cout << "camera_open_ms: " << camera_status.open_latency_ms << '\n';
    std::cout << "camera_first_frame_ms: "
              << camera_status.first_frame_latency_ms << '\n';
    std::cout << "verification_detector_p50_ms: "
              << percentile(detector_values, 0.50) << '\n';
    std::cout << "verification_embedding_p50_ms: "
              << percentile(embedding_values, 0.50) << '\n';
    std::cout << "verification_total_p50_ms: "
              << percentile(total_values, 0.50) << '\n';
    std::cout << "verification_total_p95_ms: "
              << percentile(total_values, 0.95) << '\n';
    std::cout << "benchmark_status: ok\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "benchmark_status: failed\nerror: " << error.what() << '\n';
    return 3;
  }
}
