#include "camera_lease.h"

#include <chrono>
#include <stdexcept>
#include <utility>

#include <opencv2/videoio.hpp>

namespace face_unlock {
namespace {

using SteadyClock = std::chrono::steady_clock;

int elapsed_ms(
  const SteadyClock::time_point& start,
  const SteadyClock::time_point& end
) {
  return static_cast<int>(
    std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
  );
}

class OpenCvCameraSource final : public CameraSource {
public:
  bool open(int camera_index) override {
    if (!camera_.open(camera_index, cv::CAP_V4L2)) {
      return false;
    }
    camera_.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    camera_.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    return true;
  }

  bool read(cv::Mat& frame) override {
    return camera_.read(frame) && !frame.empty();
  }

  void close() override {
    camera_.release();
  }

private:
  cv::VideoCapture camera_;
};

CameraSourceFactory default_source_factory() {
  return []() {
    return std::make_unique<OpenCvCameraSource>();
  };
}

}  // namespace

std::string camera_lease_state_name(CameraLeaseState state) {
  switch (state) {
    case CameraLeaseState::Idle:
      return "idle";
    case CameraLeaseState::Opening:
      return "opening";
    case CameraLeaseState::Active:
      return "active";
    case CameraLeaseState::Stopping:
      return "stopping";
    case CameraLeaseState::Failed:
      return "failed";
  }
  return "failed";
}

CameraLeaseManager::CameraLeaseManager(
  int camera_index,
  CameraLeasePolicy policy,
  CameraSourceFactory source_factory
) :
    camera_index_(camera_index),
    policy_(policy),
    source_factory_(source_factory ?
      std::move(source_factory) : default_source_factory()) {
  if (camera_index_ < 0) {
    throw std::runtime_error("invalid camera index");
  }
  if (policy_.open_timeout_ms < 100 ||
      policy_.open_timeout_ms > 5000) {
    throw std::runtime_error("invalid camera open timeout");
  }
  if (policy_.first_frame_timeout_ms < 100 ||
      policy_.first_frame_timeout_ms > 5000) {
    throw std::runtime_error("invalid first-frame timeout");
  }
  if (policy_.active_duration_ms < 100 ||
      policy_.active_duration_ms > 5000) {
    throw std::runtime_error("invalid camera active duration");
  }

  worker_ = std::thread(&CameraLeaseManager::worker_main, this);
}

CameraLeaseManager::~CameraLeaseManager() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_ = true;
    requested_ = false;
    release_reason_ = "daemon_shutdown";
  }
  condition_.notify_all();

  if (worker_.joinable()) {
    worker_.join();
  }
}

CameraLeaseStartResult CameraLeaseManager::start() {
  std::lock_guard<std::mutex> lock(mutex_);
  CameraLeaseStartResult result;

  if (requested_ &&
      (state_ == CameraLeaseState::Opening ||
       state_ == CameraLeaseState::Active)) {
    result.accepted = true;
    result.already_active = true;
    result.generation = generation_;
    result.state = state_;
    result.reason = "lease_already_active";
    return result;
  }

  if (state_ == CameraLeaseState::Stopping || open_in_progress_ ||
      state_ == CameraLeaseState::Opening) {
    result.generation = generation_;
    result.state = state_;
    result.reason = "camera_stopping";
    return result;
  }

  ++generation_;
  requested_ = true;
  state_ = CameraLeaseState::Opening;
  lease_frames_ = 0;
  open_latency_ms_ = -1;
  first_frame_latency_ms_ = -1;
  release_reason_ = "start_requested";
  clear_frame_locked();

  result.accepted = true;
  result.generation = generation_;
  result.state = state_;
  result.reason = "start_requested";
  condition_.notify_all();
  return result;
}

CameraLeaseStopResult CameraLeaseManager::stop(
  const std::string& reason,
  int wait_timeout_ms
) {
  std::unique_lock<std::mutex> lock(mutex_);
  CameraLeaseStopResult result;
  result.generation = generation_;
  const std::string effective_reason =
    reason.empty() ? "client_cancelled" : reason;

  if (!camera_open_ && !open_in_progress_ &&
      state_ != CameraLeaseState::Opening &&
      state_ != CameraLeaseState::Stopping) {
    result.released = true;
    result.state = state_;
    result.reason = release_reason_;
    return result;
  }

  if (state_ == CameraLeaseState::Opening && !open_in_progress_) {
    requested_ = false;
    state_ = CameraLeaseState::Idle;
    release_reason_ = effective_reason;
    clear_frame_locked();
    result.released = true;
    result.state = state_;
    result.reason = release_reason_;
    condition_.notify_all();
    return result;
  }

  requested_ = false;
  release_reason_ = effective_reason;
  if (state_ == CameraLeaseState::Active) {
    state_ = CameraLeaseState::Stopping;
  }
  condition_.notify_all();

  if (wait_timeout_ms > 0) {
    condition_.wait_for(
      lock,
      std::chrono::milliseconds(wait_timeout_ms),
      [this]() {
        return !camera_open_ && !open_in_progress_ &&
          state_ != CameraLeaseState::Opening &&
          state_ != CameraLeaseState::Stopping;
      }
    );
  }

  result.released = !camera_open_ && !open_in_progress_ &&
    state_ != CameraLeaseState::Opening &&
    state_ != CameraLeaseState::Stopping;
  result.state = state_;
  result.reason = release_reason_;
  return result;
}

CameraLeaseStatus CameraLeaseManager::status() const {
  std::lock_guard<std::mutex> lock(mutex_);
  CameraLeaseStatus result;
  result.state = state_;
  result.generation = generation_;
  result.frames_total = frames_total_;
  result.lease_frames = lease_frames_;
  result.camera_open = camera_open_;
  result.frame_available = has_frame_ && !latest_frame_.empty();
  if (result.frame_available) {
    result.frame_width = latest_frame_.cols;
    result.frame_height = latest_frame_.rows;
    result.frame_channels = latest_frame_.channels();
  }
  result.open_latency_ms = open_latency_ms_;
  result.first_frame_latency_ms = first_frame_latency_ms_;
  result.release_reason = release_reason_;
  return result;
}

bool CameraLeaseManager::snapshot(
  cv::Mat& out_frame,
  unsigned long long& out_frames_total
) const {
  std::lock_guard<std::mutex> lock(mutex_);
  out_frames_total = frames_total_;
  if (!has_frame_ || latest_frame_.empty() ||
      state_ != CameraLeaseState::Active) {
    return false;
  }
  latest_frame_.copyTo(out_frame);
  return true;
}

const CameraLeasePolicy& CameraLeaseManager::policy() const {
  return policy_;
}

void CameraLeaseManager::clear_frame_locked() {
  latest_frame_.release();
  has_frame_ = false;
}

void CameraLeaseManager::worker_main() {
  while (true) {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this]() {
      return shutdown_ ||
        (requested_ && state_ == CameraLeaseState::Opening &&
         !open_in_progress_);
    });

    if (shutdown_) {
      return;
    }

    const unsigned long long generation = generation_;
    open_in_progress_ = true;
    const auto request_started = SteadyClock::now();
    lock.unlock();

    std::unique_ptr<CameraSource> source;
    bool opened = false;
    try {
      source = source_factory_();
      opened = source && source->open(camera_index_);
    } catch (const std::exception&) {
      opened = false;
    }
    const auto opened_at = SteadyClock::now();
    const int open_latency = elapsed_ms(request_started, opened_at);

    lock.lock();
    open_in_progress_ = false;
    open_latency_ms_ = open_latency;

    if (!opened) {
      requested_ = false;
      camera_open_ = false;
      state_ = CameraLeaseState::Failed;
      release_reason_ = "open_failed";
      clear_frame_locked();
      condition_.notify_all();
      continue;
    }

    camera_open_ = true;
    if (shutdown_ || !requested_ || generation != generation_) {
      if (release_reason_ == "start_requested") {
        release_reason_ = shutdown_ ? "daemon_shutdown" : "cancelled_while_opening";
      }
      state_ = CameraLeaseState::Stopping;
    } else if (open_latency_ms_ > policy_.open_timeout_ms) {
      requested_ = false;
      release_reason_ = "open_timeout";
      state_ = CameraLeaseState::Stopping;
    } else {
      state_ = CameraLeaseState::Active;
      condition_.notify_all();
    }

    const auto capture_started = opened_at;
    SteadyClock::time_point recognition_started;
    bool recognition_started_set = false;
    lock.unlock();

    while (true) {
      lock.lock();
      const bool should_stop = shutdown_ || !requested_ ||
        generation != generation_ ||
        state_ != CameraLeaseState::Active;
      const auto now = SteadyClock::now();
      if (!should_stop && !recognition_started_set &&
          elapsed_ms(capture_started, now) >=
            policy_.first_frame_timeout_ms) {
        requested_ = false;
        release_reason_ = "first_frame_timeout";
        state_ = CameraLeaseState::Stopping;
      } else if (!should_stop && recognition_started_set &&
                 elapsed_ms(recognition_started, now) >=
                   policy_.active_duration_ms) {
        requested_ = false;
        release_reason_ = "lease_deadline";
        state_ = CameraLeaseState::Stopping;
      }
      const bool continue_capture = !shutdown_ && requested_ &&
        generation == generation_ &&
        state_ == CameraLeaseState::Active;
      lock.unlock();

      if (!continue_capture) {
        break;
      }

      cv::Mat frame;
      bool frame_ok = false;
      try {
        frame_ok = source->read(frame) && !frame.empty();
      } catch (const std::exception&) {
        frame_ok = false;
      }
      const auto frame_time = SteadyClock::now();

      lock.lock();
      if (frame_ok && requested_ && generation == generation_ &&
          state_ == CameraLeaseState::Active) {
        const bool first_frame_expired =
          !recognition_started_set &&
          elapsed_ms(capture_started, frame_time) >=
            policy_.first_frame_timeout_ms;
        const bool recognition_expired =
          recognition_started_set &&
          elapsed_ms(recognition_started, frame_time) >=
            policy_.active_duration_ms;

        if (first_frame_expired || recognition_expired) {
          requested_ = false;
          release_reason_ = first_frame_expired ?
            "first_frame_timeout" : "lease_deadline";
          state_ = CameraLeaseState::Stopping;
        } else {
          frame.copyTo(latest_frame_);
          has_frame_ = true;
          ++frames_total_;
          ++lease_frames_;
          if (first_frame_latency_ms_ < 0) {
            first_frame_latency_ms_ =
              elapsed_ms(request_started, frame_time);
            recognition_started = frame_time;
            recognition_started_set = true;
          }
          condition_.notify_all();
        }
      }
      lock.unlock();

      if (!frame_ok) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }

    try {
      source->close();
    } catch (const std::exception&) {
      // The lease state still transitions to closed and fails safely.
    }

    lock.lock();
    camera_open_ = false;
    requested_ = false;
    open_in_progress_ = false;
    clear_frame_locked();
    if (release_reason_ == "start_requested") {
      release_reason_ = shutdown_ ? "daemon_shutdown" : "camera_stopped";
    }
    state_ = CameraLeaseState::Idle;
    condition_.notify_all();
    lock.unlock();

    if (shutdown_) {
      return;
    }
  }
}

}  // namespace face_unlock
