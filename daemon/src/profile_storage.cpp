#include "profile_storage.h"

#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <sodium.h>

#include "template_crypto.h"

namespace face_unlock {
namespace {

namespace fs = std::filesystem;

void require_paths(const ProfileStoragePaths& paths) {
  if (paths.template_path.empty() || paths.key_path.empty() ||
      paths.manifest_path.empty() ||
      paths.template_path == paths.key_path ||
      paths.template_path == paths.manifest_path ||
      paths.key_path == paths.manifest_path) {
    throw std::runtime_error("invalid profile storage paths");
  }
}

void ensure_private_parent(const std::string& path) {
  const fs::path parent = fs::path(path).parent_path();
  if (parent.empty()) {
    throw std::runtime_error("profile storage parent is missing");
  }
  std::error_code error;
  fs::create_directories(parent, error);
  if (error) {
    throw std::runtime_error("profile storage directory creation failed");
  }
  fs::permissions(
    parent,
    fs::perms::owner_all,
    fs::perm_options::replace,
    error
  );
  if (error) {
    throw std::runtime_error("profile storage directory permission failed");
  }
}

std::vector<unsigned char> read_key(const std::string& path) {
  std::vector<unsigned char> key;
  std::string error;
  if (!read_file_bytes(path, key, error)) {
    throw std::runtime_error("profile key read failed");
  }
  if (key.size() != crypto_secretbox_KEYBYTES) {
    throw std::runtime_error("profile key size is invalid");
  }
  return key;
}

std::string manifest_json(const FaceProfile& profile) {
  std::size_t samples_total = 0;
  for (const PoseTemplate& pose : profile.poses) {
    samples_total += pose.sample_count;
  }
  std::ostringstream output;
  output << "{\n"
         << "  \"format\": \"face-unlock-enrollment-manifest\",\n"
         << "  \"format_version\": 1,\n"
         << "  \"model\": {\n"
         << "    \"embedding_model_id\": \"" << profile.model_id
         << "\",\n"
         << "    \"embedding_dim\": " << profile.embedding_dim << "\n"
         << "  },\n"
         << "  \"template\": {\n"
         << "    \"encryption\": \"libsodium_crypto_secretbox\",\n"
         << "    \"contains_raw_images\": false,\n"
         << "    \"contains_embeddings\": true,\n"
         << "    \"key_storage\": \"local_development_key_file\"\n"
         << "  },\n"
         << "  \"quality\": {\"samples_total\": " << samples_total << "},\n"
         << "  \"privacy\": {\n"
         << "    \"raw_images_saved\": false,\n"
         << "    \"face_crops_saved\": false,\n"
         << "    \"telemetry_enabled\": false\n"
         << "  },\n"
         << "  \"status\": {\n"
         << "    \"enrollment_complete\": true,\n"
         << "    \"real_biometric_template\": true,\n"
         << "    \"placeholder_only\": false\n"
         << "  }\n"
         << "}\n";
  return output.str();
}

}  // namespace

ProfileStorageResult commit_encrypted_face_profile(
  const FaceProfile& profile,
  const ProfileStoragePaths& paths
) {
  ProfileStorageResult result;
  try {
    require_paths(paths);
    validate_face_profile(profile);
    ensure_private_parent(paths.template_path);
    ensure_private_parent(paths.key_path);
    ensure_private_parent(paths.manifest_path);

    std::vector<unsigned char> key;
    std::string error;
    if (fs::exists(paths.key_path)) {
      key = read_key(paths.key_path);
    } else {
      key = generate_random_key();
      if (!write_file_0600(paths.key_path, key, error)) {
        throw std::runtime_error("profile key write failed");
      }
      result.key_created = true;
    }

    std::vector<unsigned char> plaintext = serialize_face_profile(profile);
    const EncryptedBlob encrypted = encrypt_template_bytes(plaintext, key);
    sodium_memzero(plaintext.data(), plaintext.size());

    if (!write_file_0600(paths.template_path, encrypted.bytes, error)) {
      sodium_memzero(key.data(), key.size());
      throw std::runtime_error("encrypted profile write failed");
    }

    const std::string manifest = manifest_json(profile);
    const std::vector<unsigned char> manifest_bytes(
      manifest.begin(), manifest.end()
    );
    if (!write_file_0600(paths.manifest_path, manifest_bytes, error)) {
      sodium_memzero(key.data(), key.size());
      throw std::runtime_error("profile manifest write failed");
    }

    sodium_memzero(key.data(), key.size());
    result.ok = true;
    result.reason = "committed";
    return result;
  } catch (const std::exception& error) {
    result.ok = false;
    result.reason = error.what();
    return result;
  }
}

FaceProfile load_encrypted_face_profile(
  const ProfileStoragePaths& paths
) {
  require_paths(paths);
  std::vector<unsigned char> key = read_key(paths.key_path);
  std::vector<unsigned char> encrypted_bytes;
  std::string error;
  if (!read_file_bytes(paths.template_path, encrypted_bytes, error)) {
    sodium_memzero(key.data(), key.size());
    throw std::runtime_error("encrypted profile read failed");
  }

  EncryptedBlob encrypted;
  encrypted.bytes = std::move(encrypted_bytes);
  std::vector<unsigned char> plaintext;
  try {
    plaintext = decrypt_template_bytes(encrypted, key);
    sodium_memzero(key.data(), key.size());
    FaceProfile profile = deserialize_face_profile(plaintext);
    sodium_memzero(plaintext.data(), plaintext.size());
    return profile;
  } catch (...) {
    sodium_memzero(key.data(), key.size());
    if (!plaintext.empty()) {
      sodium_memzero(plaintext.data(), plaintext.size());
    }
    throw;
  }
}

}  // namespace face_unlock
