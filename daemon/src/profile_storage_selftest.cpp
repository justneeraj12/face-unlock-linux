#include "profile_storage.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {
namespace fs = std::filesystem;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

face_unlock::FaceProfile profile() {
  face_unlock::FaceProfile value;
  value.model_id = "sface-2021dec";
  value.embedding_dim = 2;
  for (int index = 0; index < 5; ++index) {
    face_unlock::PoseTemplate pose;
    pose.pose = static_cast<face_unlock::PoseSlot>(index);
    pose.sample_count = 3;
    pose.centroid = index == 0 ?
      std::vector<float>{1.0F, 0.0F} :
      std::vector<float>{0.0F, 1.0F};
    value.poses.push_back(pose);
  }
  return value;
}

mode_t mode(const std::string& path) {
  struct stat metadata {};
  if (::stat(path.c_str(), &metadata) != 0) return 0;
  return metadata.st_mode & 0777;
}

std::string read_text(const std::string& path) {
  std::ifstream input(path);
  return std::string(
    std::istreambuf_iterator<char>(input),
    std::istreambuf_iterator<char>()
  );
}

}  // namespace

int main() {
  char directory_template[] = "/tmp/face-unlock-profile-storage.XXXXXX";
  char* directory = ::mkdtemp(directory_template);
  if (directory == nullptr) {
    std::cerr << "status: failed\nerror: mkdtemp failed\n";
    return 1;
  }
  const fs::path root(directory);
  try {
    const face_unlock::ProfileStoragePaths paths{
      (root / "template.enc").string(),
      (root / "template.key").string(),
      (root / "enrollment.json").string(),
    };
    const face_unlock::ProfileStorageMetadata validation{
      true,
      5,
      0.91,
      0.45,
    };
    const auto unvalidated = face_unlock::commit_encrypted_face_profile(
      profile(), paths, face_unlock::ProfileStorageMetadata{}
    );
    require(!unvalidated.ok && !fs::exists(paths.template_path) &&
            !fs::exists(paths.key_path) && !fs::exists(paths.manifest_path),
            "unvalidated profile was persisted");
    const auto first = face_unlock::commit_encrypted_face_profile(
      profile(), paths, validation
    );
    require(first.ok && first.key_created, "initial commit failed");
    require(mode(paths.template_path) == 0600 &&
            mode(paths.key_path) == 0600 &&
            mode(paths.manifest_path) == 0600,
            "stored file permissions are not 0600");
    const std::string manifest = read_text(paths.manifest_path);
    require(manifest.find("\"enrollment_complete\": true") !=
              std::string::npos &&
            manifest.find("\"real_biometric_template\": true") !=
              std::string::npos &&
            manifest.find("\"placeholder_only\": false") !=
              std::string::npos &&
            manifest.find("\"heldout_validation_passed\": true") !=
              std::string::npos &&
            manifest.find("\"heldout_samples_total\": 5") !=
              std::string::npos &&
            manifest.find("\"created_at\": \"") !=
              std::string::npos &&
            manifest.find("\"user\": {") != std::string::npos &&
            manifest.find("\"input_size\": [112, 112]") !=
              std::string::npos &&
            manifest.find("\"encrypted_template_path\": \"") !=
              std::string::npos &&
            manifest.find("\"center\": true") != std::string::npos &&
            manifest.find("\\\"") == std::string::npos,
            "enrollment manifest is malformed");

    const auto loaded = face_unlock::load_encrypted_face_profile(paths);
    require(loaded.model_id == "sface-2021dec" &&
            loaded.poses.size() == 5,
            "stored profile did not load");

    const auto second = face_unlock::commit_encrypted_face_profile(
      profile(), paths, validation
    );
    require(second.ok && !second.key_created,
            "existing profile key was not reused");

    std::fstream tamper(paths.template_path, std::ios::in | std::ios::out |
      std::ios::binary);
    char byte = 0;
    tamper.read(&byte, 1);
    byte ^= 0x01;
    tamper.seekp(0);
    tamper.write(&byte, 1);
    tamper.close();
    bool tamper_rejected = false;
    try {
      (void)face_unlock::load_encrypted_face_profile(paths);
    } catch (const std::exception&) {
      tamper_rejected = true;
    }
    require(tamper_rejected, "tampered encrypted profile loaded");

    std::cout << "profile_validation_gate_status: ok\n";
    std::cout << "profile_atomic_commit_status: ok\n";
    std::cout << "profile_file_mode_status: 0600\n";
    std::cout << "profile_manifest_status: ok\n";
    std::cout << "profile_key_reuse_status: ok\n";
    std::cout << "profile_tamper_rejection_status: ok\n";
    std::cout << "status: ok\n";
    fs::remove_all(root);
    return 0;
  } catch (const std::exception& error) {
    fs::remove_all(root);
    std::cerr << "status: failed\nerror: " << error.what() << '\n';
    return 1;
  }
}
