#pragma once

#include <cstddef>
#include <string>

#include "face_profile.h"

namespace face_unlock {

struct ProfileStoragePaths {
  std::string template_path;
  std::string key_path;
  std::string manifest_path;
};

struct ProfileStorageResult {
  bool ok = false;
  bool key_created = false;
  std::string reason;
};

struct ProfileStorageMetadata {
  bool heldout_validation_passed = false;
  std::size_t heldout_samples = 0;
  double lowest_heldout_similarity = -1.0;
  double required_heldout_similarity = 0.45;
};

ProfileStorageResult commit_encrypted_face_profile(
  const FaceProfile& profile,
  const ProfileStoragePaths& paths,
  const ProfileStorageMetadata& metadata
);

FaceProfile load_encrypted_face_profile(
  const ProfileStoragePaths& paths
);

}  // namespace face_unlock
