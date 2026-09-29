#include "enrollment_controller.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <unistd.h>

namespace {
namespace fs = std::filesystem;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

class FakeCamera final : public face_unlock::CameraSource {
public:
  bool open(int camera_index) override { return camera_index == 0; }
  bool read(cv::Mat& frame) override {
    frame = cv::Mat(100, 100, CV_8UC3);
    for (int y = 0; y < frame.rows; ++y) {
      for (int x = 0; x < frame.cols; ++x) {
        const unsigned char value = ((x / 4 + y / 4) % 2) ? 220 : 40;
        frame.at<cv::Vec3b>(y, x) = cv::Vec3b(value, value, value);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return true;
  }
  void close() override {}
};

class FailedCamera final : public face_unlock::CameraSource {
public:
  bool open(int) override { return false; }
  bool read(cv::Mat&) override { return false; }
  void close() override {}
};

class FakeDetector final : public face_unlock::FaceDetector {
public:
  std::string backend_name() const override { return "yunet"; }
  face_unlock::DetectorResult detect(const cv::Mat&) override {
    static const double nose_x[] = {30.0, 25.0, 35.0, 30.0, 30.0};
    static const double nose_y[] = {43.0, 43.0, 43.0, 37.0, 50.0};
    const std::size_t pose = calls_++ % 5;
    face_unlock::DetectionBox box;
    box.x = 10;
    box.y = 10;
    box.w = 80;
    box.h = 80;
    box.score = 0.99;
    box.landmarks = {
      cv::Point2f(20.0F, 25.0F),
      cv::Point2f(40.0F, 25.0F),
      cv::Point2f(
        static_cast<float>(nose_x[pose]),
        static_cast<float>(nose_y[pose])
      ),
      cv::Point2f(22.0F, 60.0F),
      cv::Point2f(38.0F, 60.0F),
    };
    return {"yunet", {box}};
  }
private:
  std::size_t calls_ = 0;
};

class FakeEmbedder final : public face_unlock::FaceEmbedder {
public:
  std::string model_id() const override { return "test-sface"; }
  face_unlock::FaceEmbedding align_and_embed(
    const cv::Mat&,
    const face_unlock::DetectionBox&
  ) override {
    face_unlock::FaceEmbedding embedding;
    embedding.model = model_id();
    embedding.values.assign(5, 0.0F);
    embedding.values[calls_++ % 5] = 1.0F;
    return embedding;
  }
private:
  std::size_t calls_ = 0;
};

template <typename Predicate>
bool wait_until(Predicate predicate, int timeout_ms = 1000) {
  const auto deadline = std::chrono::steady_clock::now() +
    std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    if (predicate()) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return predicate();
}

}  // namespace

int main() {
  char directory_template[] = "/tmp/face-unlock-enrollment.XXXXXX";
  char* directory = ::mkdtemp(directory_template);
  if (directory == nullptr) return 1;
  const fs::path root(directory);

  try {
    face_unlock::CameraLeasePolicy camera_policy;
    camera_policy.open_timeout_ms = 200;
    camera_policy.first_frame_timeout_ms = 200;
    camera_policy.active_duration_ms = 200;
    face_unlock::CameraLeaseManager camera(
      0,
      camera_policy,
      []() { return std::make_unique<FakeCamera>(); }
    );
    FakeDetector detector;
    FakeEmbedder embedder;
    face_unlock::EnrollmentPolicy enrollment_policy;
    enrollment_policy.target_per_pose = 1;
    const face_unlock::ProfileStoragePaths paths{
      (root / "template.enc").string(),
      (root / "template.key").string(),
      (root / "enrollment.json").string(),
    };
    face_unlock::EnrollmentController controller(
      &camera,
      &detector,
      &embedder,
      paths,
      enrollment_policy,
      2000
    );

    require(controller.start().ok, "enrollment start failed");
    require(wait_until([&camera]() {
      return camera.status().frame_available;
    }), "enrollment camera did not become ready");
    FakeDetector competing_detector;
    FakeEmbedder competing_embedder;
    face_unlock::EnrollmentController competing_controller(
      &camera,
      &competing_detector,
      &competing_embedder,
      paths,
      enrollment_policy,
      2000
    );
    require(competing_controller.start().reason == "camera_busy",
            "active camera lease was taken over by another enrollment");

    unsigned long long observed_frames = 0;
    for (int sample = 0; sample < 10; ++sample) {
      require(wait_until([&camera, observed_frames]() {
        return camera.status().frames_total > observed_frames;
      }), "fresh enrollment frame was not available");
      const auto captured = controller.capture();
      require(captured.ok && captured.sample_accepted,
              "qualified enrollment sample was rejected");
      observed_frames = camera.status().frames_total;
      if (sample < 9) {
        const auto repeated = controller.capture();
        require(!repeated.ok && !repeated.sample_accepted &&
                repeated.reason == "camera_frame_unchanged",
                "same enrollment frame was processed twice");
      }
    }
    const auto ready = controller.status();
    require(ready.enrollment.state == face_unlock::EnrollmentState::Ready &&
            ready.enrollment.progress_percent == 100 &&
            ready.enrollment.validation_progress_percent == 100 &&
            ready.enrollment.validation_samples == 5 &&
            ready.enrollment.lowest_validation_similarity > 0.99,
            "held-out validated enrollment was not ready");
    require(wait_until([&camera]() {
      return !camera.status().camera_open;
    }), "camera remained open after enrollment became ready");

    const auto committed = controller.commit();
    require(committed.ok && committed.enrollment.state ==
              face_unlock::EnrollmentState::Committed,
            "encrypted enrollment commit failed");
    const auto loaded = face_unlock::load_encrypted_face_profile(paths);
    require(loaded.model_id == "test-sface" && loaded.poses.size() == 5,
            "committed enrollment profile did not load");

    require(controller.start().ok, "second enrollment did not start");
    require(controller.cancel().enrollment.state ==
              face_unlock::EnrollmentState::Cancelled,
            "enrollment cancellation did not clear state");
    require(!controller.commit().ok,
            "cancelled enrollment was committed");

    face_unlock::CameraLeaseManager failed_camera(
      0,
      camera_policy,
      []() { return std::make_unique<FailedCamera>(); }
    );
    FakeDetector failed_detector;
    FakeEmbedder failed_embedder;
    face_unlock::EnrollmentController failed_controller(
      &failed_camera,
      &failed_detector,
      &failed_embedder,
      paths,
      enrollment_policy,
      2000
    );
    require(failed_controller.start().ok,
            "failed-camera enrollment start request was rejected");
    require(wait_until([&failed_camera]() {
      return failed_camera.status().state ==
        face_unlock::CameraLeaseState::Failed;
    }), "enrollment camera failure was not observed");
    const auto failed_status = failed_controller.status();
    require(!failed_status.ok && failed_status.enrollment.state ==
              face_unlock::EnrollmentState::Cancelled,
            "camera failure did not erase the enrollment session");

    std::cout << "enrollment_pipeline_status: ok\n";
    std::cout << "enrollment_fresh_frame_status: ok\n";
    std::cout << "enrollment_camera_release_status: ok\n";
    std::cout << "enrollment_camera_exclusivity_status: ok\n";
    std::cout << "enrollment_camera_failure_erasure_status: ok\n";
    std::cout << "enrollment_heldout_validation_status: ok\n";
    std::cout << "enrollment_encrypted_commit_status: ok\n";
    std::cout << "enrollment_cancel_fail_closed_status: ok\n";
    std::cout << "status: ok\n";
    fs::remove_all(root);
    return 0;
  } catch (const std::exception& error) {
    fs::remove_all(root);
    std::cerr << "status: failed\nerror: " << error.what() << "\n";
    return 1;
  }
}
