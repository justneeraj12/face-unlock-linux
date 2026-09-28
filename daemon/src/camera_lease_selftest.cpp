#include "camera_lease.h"

#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace {

struct FakeCameraStats {
  std::atomic<int> opens{0};
  std::atomic<int> closes{0};
  std::atomic<int> reads{0};
  bool open_succeeds = true;
  int read_delay_ms = 2;
};

class FakeCameraSource final : public face_unlock::CameraSource {
public:
  explicit FakeCameraSource(std::shared_ptr<FakeCameraStats> stats)
      : stats_(std::move(stats)) {}

  bool open(int camera_index) override {
    if (camera_index != 0) {
      return false;
    }
    ++stats_->opens;
    return stats_->open_succeeds;
  }

  bool read(cv::Mat& frame) override {
    ++stats_->reads;
    frame = cv::Mat::zeros(48, 64, CV_8UC3);
    std::this_thread::sleep_for(
      std::chrono::milliseconds(stats_->read_delay_ms)
    );
    return true;
  }

  void close() override {
    ++stats_->closes;
  }

private:
  std::shared_ptr<FakeCameraStats> stats_;
};

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Predicate>
bool wait_until(Predicate predicate, int timeout_ms = 500) {
  const auto deadline = std::chrono::steady_clock::now() +
    std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    if (predicate()) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return predicate();
}

face_unlock::CameraSourceFactory factory_for(
  const std::shared_ptr<FakeCameraStats>& stats
) {
  return [stats]() {
    return std::make_unique<FakeCameraSource>(stats);
  };
}

}  // namespace

int main() {
  try {
    using face_unlock::CameraLeaseManager;
    using face_unlock::CameraLeasePolicy;
    using face_unlock::CameraLeaseState;

    CameraLeasePolicy policy;
    policy.open_timeout_ms = 200;
    policy.active_duration_ms = 120;

    auto stats = std::make_shared<FakeCameraStats>();
    CameraLeaseManager manager(0, policy, factory_for(stats));

    const auto first = manager.start();
    require(first.accepted && first.generation == 1,
            "first camera lease was not accepted");
    const auto duplicate = manager.start();
    require(duplicate.accepted && duplicate.already_active &&
            duplicate.generation == first.generation,
            "duplicate start was not idempotent");
    require(wait_until([&manager]() {
      return manager.status().state == CameraLeaseState::Active;
    }), "camera lease did not become active");
    require(wait_until([&manager]() {
      return manager.status().frame_available;
    }), "camera lease did not publish a frame");

    cv::Mat snapshot;
    unsigned long long frames_total = 0;
    require(manager.snapshot(snapshot, frames_total) &&
            snapshot.cols == 64 && snapshot.rows == 48,
            "active camera snapshot failed");

    const auto invalid_duration = manager.start(60001);
    require(!invalid_duration.accepted &&
            invalid_duration.reason == "invalid_active_duration",
            "unsafe camera lease duration was accepted");

    const auto stopped = manager.stop("password_started");
    require(stopped.released, "camera was not released after cancellation");
    const auto stopped_status = manager.status();
    require(stopped_status.state == CameraLeaseState::Idle &&
            !stopped_status.camera_open &&
            !stopped_status.frame_available &&
            stopped_status.release_reason == "password_started",
            "cancelled camera lease retained state or frame data");
    require(stats->closes == 1, "camera close count after cancellation is wrong");

    const auto second = manager.start(300);
    require(second.accepted && second.generation == 2,
            "second camera lease generation is wrong");
    require(wait_until([&manager]() {
      return manager.status().state == CameraLeaseState::Active;
    }), "second camera lease did not become active");
    std::this_thread::sleep_for(std::chrono::milliseconds(180));
    require(manager.status().state == CameraLeaseState::Active,
            "bounded duration override was ignored");
    require(wait_until([&manager]() {
      const auto status = manager.status();
      return status.state == CameraLeaseState::Idle &&
        status.release_reason == "lease_deadline";
    }), "camera did not auto-release at the lease deadline");
    require(stats->closes == 2, "camera close count after deadline is wrong");

    auto failed_stats = std::make_shared<FakeCameraStats>();
    failed_stats->open_succeeds = false;
    CameraLeaseManager failed(0, policy, factory_for(failed_stats));
    require(failed.start().accepted, "failed camera start request rejected");
    require(wait_until([&failed]() {
      return failed.status().state == CameraLeaseState::Failed;
    }), "camera open failure did not reach failed state");
    const auto failed_status = failed.status();
    require(!failed_status.camera_open &&
            !failed_status.frame_available &&
            failed_status.release_reason == "open_failed",
            "camera open failure retained unsafe state");

    auto slow_stats = std::make_shared<FakeCameraStats>();
    slow_stats->read_delay_ms = 150;
    CameraLeasePolicy warmup_policy = policy;
    warmup_policy.first_frame_timeout_ms = 100;
    CameraLeaseManager slow(0, warmup_policy, factory_for(slow_stats));
    require(slow.start().accepted, "slow camera start request rejected");
    require(wait_until([&slow]() {
      const auto status = slow.status();
      return status.state == CameraLeaseState::Idle &&
        status.release_reason == "first_frame_timeout";
    }), "late first frame did not time out");
    const auto slow_status = slow.status();
    require(!slow_status.camera_open &&
            !slow_status.frame_available &&
            slow_status.lease_frames == 0,
            "late first frame was retained after timeout");

    bool invalid_policy_rejected = false;
    try {
      CameraLeasePolicy invalid;
      invalid.active_duration_ms = 0;
      CameraLeaseManager invalid_manager(0, invalid, factory_for(stats));
      (void)invalid_manager;
    } catch (const std::runtime_error&) {
      invalid_policy_rejected = true;
    }
    require(invalid_policy_rejected, "invalid camera lease policy accepted");

    std::cout << "camera_lease_cancel_status: ok\n";
    std::cout << "camera_lease_deadline_status: ok\n";
    std::cout << "camera_lease_duration_override_status: ok\n";
    std::cout << "camera_lease_stale_frame_status: ok\n";
    std::cout << "camera_lease_open_failure_status: ok\n";
    std::cout << "status: ok\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "status: failed\n";
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
}
