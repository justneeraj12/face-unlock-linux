#pragma once

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

ProfileStorageResult commit_encrypted_face_profile(
  const FaceProfile& profile,
  const ProfileStoragePaths& paths
);

FaceProfile load_encrypted_face_profile(
  const ProfileStoragePaths& paths
);

}  // namespace face_unlock
