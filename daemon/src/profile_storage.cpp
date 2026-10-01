#include "profile_storage.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <locale>
#include <pwd.h>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
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

std::string json_escape(const std::string& value) {
  std::ostringstream escaped;
  for (const unsigned char character : value) {
    switch (character) {
      case '\"': escaped << "\\\""; break;
      case '\\': escaped << "\\\\"; break;
      case '\b': escaped << "\\b"; break;
      case '\f': escaped << "\\f"; break;
      case '\n': escaped << "\\n"; break;
      case '\r': escaped << "\\r"; break;
      case '\t': escaped << "\\t"; break;
      default:
        if (character < 0x20) {
          char buffer[7] {};
          std::snprintf(
            buffer,
            sizeof(buffer),
            "\\u%04x",
            static_cast<unsigned>(character)
          );
          escaped << buffer;
        } else {
          escaped << static_cast<char>(character);
        }
    }
  }
  return escaped.str();
}

std::string current_utc_timestamp() {
  const std::time_t now = std::time(nullptr);
  std::tm utc {};
  if (::gmtime_r(&now, &utc) == nullptr) {
    throw std::runtime_error("profile timestamp generation failed");
  }
  char buffer[32] {};
  if (std::strftime(
        buffer,
        sizeof(buffer),
        "%Y-%m-%dT%H:%M:%SZ",
        &utc
      ) == 0) {
    throw std::runtime_error("profile timestamp formatting failed");
  }
  return buffer;
}

std::string current_username() {
  struct passwd entry {};
  struct passwd* result = nullptr;
  std::array<char, 16384> buffer {};
  if (::getpwuid_r(
        ::getuid(),
        &entry,
        buffer.data(),
        buffer.size(),
        &result
      ) == 0 && result != nullptr && entry.pw_name != nullptr &&
      entry.pw_name[0] != '\0') {
    return entry.pw_name;
  }
  return std::to_string(::getuid());
}

std::string manifest_json(
  const FaceProfile& profile,
  const ProfileStoragePaths& paths,
  const ProfileStorageMetadata& metadata
) {
  std::size_t samples_total = 0;
  for (const PoseTemplate& pose : profile.poses) {
    samples_total += pose.sample_count;
  }
  const std::string now = current_utc_timestamp();
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << "{\n"
         << "  \"format\": \"face-unlock-enrollment-manifest\",\n"
         << "  \"format_version\": 1,\n"
         << "  \"created_at\": \"" << now << "\",\n"
         << "  \"updated_at\": \"" << now << "\",\n"
         << "  \"user\": {\n"
         << "    \"uid\": " << ::getuid() << ",\n"
         << "    \"username\": \""
         << json_escape(current_username()) << "\"\n"
         << "  },\n"
         << "  \"model\": {\n"
         << "    \"embedding_model_id\": \""
         << json_escape(profile.model_id) << "\",\n"
         << "    \"detector_model_id\": \"opencv-yunet-runtime-selected\",\n"
         << "    \"embedding_dim\": " << profile.embedding_dim << ",\n"
         << "    \"input_size\": [112, 112],\n"
         << "    \"preprocessing\": {\n"
         << "      \"color_order\": \"BGR\",\n"
         << "      \"normalization\": \"opencv_sface_internal\",\n"
         << "      \"alignment\": \"opencv_sface_align_crop\"\n"
         << "    }\n"
         << "  },\n"
         << "  \"template\": {\n"
         << "    \"encrypted_template_path\": \""
         << json_escape(paths.template_path) << "\",\n"
         << "    \"encryption\": \"libsodium_crypto_secretbox\",\n"
         << "    \"contains_raw_images\": false,\n"
         << "    \"contains_embeddings\": true,\n"
         << "    \"key_storage\": \"local_development_key_file\"\n"
         << "  },\n"
         << "  \"quality\": {\n"
         << "    \"training_samples_total\": " << samples_total << ",\n"
         << "    \"heldout_samples_total\": "
         << metadata.heldout_samples << ",\n"
         << "    \"heldout_validation_passed\": true,\n"
         << "    \"lowest_heldout_similarity\": "
         << metadata.lowest_heldout_similarity << ",\n"
         << "    \"required_heldout_similarity\": "
         << metadata.required_heldout_similarity << ",\n"
         << "    \"pose_slots\": {\n"
         << "      \"center\": true,\n"
         << "      \"left\": true,\n"
         << "      \"right\": true,\n"
         << "      \"up\": true,\n"
         << "      \"down\": true\n"
         << "    }\n"
         << "  },\n"
         << "  \"privacy\": {\n"
         << "    \"raw_images_saved\": false,\n"
         << "    \"face_crops_saved\": false,\n"
         << "    \"telemetry_enabled\": false,\n"
         << "    \"consent_version\": \"native-enrollment-v1\"\n"
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
  const ProfileStoragePaths& paths,
  const ProfileStorageMetadata& metadata
) {
  ProfileStorageResult result;
  try {
    require_paths(paths);
    validate_face_profile(profile);
    if (!metadata.heldout_validation_passed ||
        metadata.heldout_samples < 5 ||
        !std::isfinite(metadata.lowest_heldout_similarity) ||
        !std::isfinite(metadata.required_heldout_similarity) ||
        metadata.lowest_heldout_similarity <
          metadata.required_heldout_similarity ||
        metadata.lowest_heldout_similarity < -1.0 ||
        metadata.lowest_heldout_similarity > 1.0 ||
        metadata.required_heldout_similarity < -1.0 ||
        metadata.required_heldout_similarity > 1.0) {
      throw std::runtime_error("held-out profile validation required");
    }
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

    const std::string manifest = manifest_json(profile, paths, metadata);
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
