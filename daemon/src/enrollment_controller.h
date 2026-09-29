#pragma once

#include <cstddef>
#include <string>

#include "camera_lease.h"
#include "detector.h"
#include "enrollment_session.h"
#include "frame_quality.h"
#include "profile_storage.h"
#include "recognizer.h"

namespace face_unlock {

struct EnrollmentOperationResult {
  bool ok = false;
  bool sample_accepted = false;
  bool key_created = false;
  std::string reason = "not_started";
  EnrollmentStatus enrollment;
  PoseSlot pose = PoseSlot::Unknown;
  std::size_t faces_detected = 0;
  double detector_ms = 0.0;
  double embedding_ms = 0.0;
  FrameQualityResult quality;
};

class EnrollmentController final {
public:
  EnrollmentController(
    CameraLeaseManager* camera,
    FaceDetector* detector,
    FaceEmbedder* embedder,
    ProfileStoragePaths storage_paths,
    EnrollmentPolicy policy = EnrollmentPolicy{},
    int camera_lease_ms = 45000
  );

  EnrollmentOperationResult start();
  EnrollmentOperationResult capture();
  EnrollmentOperationResult status();
  EnrollmentOperationResult cancel();
  EnrollmentOperationResult commit();

private:
  EnrollmentOperationResult result(
    bool ok,
    const std::string& reason
  ) const;

  CameraLeaseManager* camera_ = nullptr;
  FaceDetector* detector_ = nullptr;
  FaceEmbedder* embedder_ = nullptr;
  ProfileStoragePaths storage_paths_;
  EnrollmentSession session_;
  unsigned long long last_processed_frames_total_ = 0;
  int camera_lease_ms_ = 45000;
};

}  // namespace face_unlock
