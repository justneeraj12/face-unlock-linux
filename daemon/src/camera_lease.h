#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <opencv2/core.hpp>

namespace face_unlock {

struct CameraLeasePolicy {
  int open_timeout_ms = 1500;
  int first_frame_timeout_ms = 1500;
  int active_duration_ms = 1000;
};

enum class CameraLeaseState {
  Idle,
  Opening,
  Active,
  Stopping,
  Failed,
};

struct CameraLeaseStatus {
  CameraLeaseState state = CameraLeaseState::Idle;
  unsigned long long generation = 0;
  unsigned long long frames_total = 0;
  unsigned long long lease_frames = 0;
  bool camera_open = false;
  bool frame_available = false;
  int frame_width = 0;
  int frame_height = 0;
  int frame_channels = 0;
  int open_latency_ms = -1;
  int first_frame_latency_ms = -1;
  std::string release_reason = "never_started";
};

struct CameraLeaseStartResult {
  bool accepted = false;
  bool already_active = false;
  unsigned long long generation = 0;
  CameraLeaseState state = CameraLeaseState::Idle;
  std::string reason;
};

struct CameraLeaseStopResult {
  bool released = false;
  unsigned long long generation = 0;
  CameraLeaseState state = CameraLeaseState::Idle;
  std::string reason;
};

class CameraSource {
public:
  virtual ~CameraSource() = default;
  virtual bool open(int camera_index) = 0;
  virtual bool read(cv::Mat& frame) = 0;
  virtual void close() = 0;
};

using CameraSourceFactory =
  std::function<std::unique_ptr<CameraSource>()>;

class CameraLeaseManager final {
public:
  explicit CameraLeaseManager(
    int camera_index,
    CameraLeasePolicy policy = CameraLeasePolicy{},
    CameraSourceFactory source_factory = CameraSourceFactory{}
  );
  ~CameraLeaseManager();

  CameraLeaseManager(const CameraLeaseManager&) = delete;
  CameraLeaseManager& operator=(const CameraLeaseManager&) = delete;

  CameraLeaseStartResult start(int active_duration_ms = 0);
  CameraLeaseStopResult stop(
    const std::string& reason,
    int wait_timeout_ms = 500
  );
  CameraLeaseStatus status() const;
  bool snapshot(
    cv::Mat& out_frame,
    unsigned long long& out_frames_total
  ) const;

  const CameraLeasePolicy& policy() const;

private:
  void worker_main();
  void clear_frame_locked();

  int camera_index_ = 0;
  CameraLeasePolicy policy_;
  CameraSourceFactory source_factory_;

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::thread worker_;
  bool shutdown_ = false;
  bool requested_ = false;
  bool open_in_progress_ = false;
  CameraLeaseState state_ = CameraLeaseState::Idle;
  unsigned long long generation_ = 0;
  unsigned long long frames_total_ = 0;
  unsigned long long lease_frames_ = 0;
  bool camera_open_ = false;
  bool has_frame_ = false;
  cv::Mat latest_frame_;
  int open_latency_ms_ = -1;
  int first_frame_latency_ms_ = -1;
  int lease_active_duration_ms_ = 1000;
  std::string release_reason_ = "never_started";
};

std::string camera_lease_state_name(CameraLeaseState state);

}  // namespace face_unlock
