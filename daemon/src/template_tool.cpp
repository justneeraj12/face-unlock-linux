#include "template_crypto.h"

#include <filesystem>
#include <ctime>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace {

void print_usage(const char* program) {
  std::cout << "Usage:\n";
  std::cout << "  " << program << " status\n";
  std::cout << "  " << program << " create-placeholder --i-understand-placeholder [--overwrite]\n";
  std::cout << "  " << program << " delete --yes\n";
  std::cout << "\n";
  std::cout << "This tool manages the encrypted placeholder template scaffold.\n";
  std::cout << "It does not create a real biometric template yet.\n";
}

bool file_exists(const std::string& path) {
  struct stat st {};
  return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string mode_string(const std::string& path) {
  struct stat st {};

  if (::stat(path.c_str(), &st) != 0) {
    return "unknown";
  }

  char buffer[16] {};
  std::snprintf(buffer, sizeof(buffer), "%04o", static_cast<unsigned>(st.st_mode & 0777));
  return buffer;
}

std::string default_enrollment_manifest_path() {
  const char* home = std::getenv("HOME");

  if (home == nullptr || std::string(home).empty()) {
    return "";
  }

  return std::string(home) + "/.local/share/face-unlock/enrollment.json";
}

std::string current_utc_timestamp() {
  std::time_t now = std::time(nullptr);
  std::tm tm_utc {};

  gmtime_r(&now, &tm_utc);

  char buffer[32] {};
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);

  return buffer;
}

std::string current_username() {
  const char* user = std::getenv("USER");

  if (user != nullptr && std::string(user).size() > 0) {
    return user;
  }

  return "unknown";
}

std::string placeholder_manifest_json(const std::string& template_path) {
  const std::string now = current_utc_timestamp();

  return std::string()
    + "{\n"
    + "  \"format\": \"face-unlock-enrollment-manifest\",\n"
    + "  \"format_version\": 1,\n"
    + "  \"created_at\": \"" + now + "\",\n"
    + "  \"updated_at\": \"" + now + "\",\n"
    + "  \"user\": {\n"
    + "    \"uid\": " + std::to_string(getuid()) + ",\n"
    + "    \"username\": \"" + current_username() + "\"\n"
    + "  },\n"
    + "  \"model\": {\n"
    + "    \"embedding_model_id\": \"placeholder-none\",\n"
    + "    \"detector_model_id\": \"none\",\n"
    + "    \"embedding_dim\": 0,\n"
    + "    \"input_size\": [112, 112],\n"
    + "    \"preprocessing\": {\n"
    + "      \"color_order\": \"not_applicable\",\n"
    + "      \"normalization\": \"not_applicable\",\n"
    + "      \"alignment\": \"not_implemented\"\n"
    + "    }\n"
    + "  },\n"
    + "  \"template\": {\n"
    + "    \"encrypted_template_path\": \"" + template_path + "\",\n"
    + "    \"encryption\": \"libsodium_crypto_secretbox\",\n"
    + "    \"contains_raw_images\": false,\n"
    + "    \"contains_embeddings\": false,\n"
    + "    \"key_storage\": \"not_implemented\"\n"
    + "  },\n"
    + "  \"quality\": {\n"
    + "    \"samples_total\": 0,\n"
    + "    \"pose_slots\": {\n"
    + "      \"center\": false,\n"
    + "      \"left\": false,\n"
    + "      \"right\": false,\n"
    + "      \"up\": false,\n"
    + "      \"down\": false\n"
    + "    },\n"
    + "    \"min_luma\": null,\n"
    + "    \"max_luma\": null,\n"
    + "    \"mean_luma\": null,\n"
    + "    \"sharpness_score\": null\n"
    + "  },\n"
    + "  \"privacy\": {\n"
    + "    \"raw_images_saved\": false,\n"
    + "    \"face_crops_saved\": false,\n"
    + "    \"telemetry_enabled\": false,\n"
    + "    \"consent_version\": \"placeholder\"\n"
    + "  },\n"
    + "  \"status\": {\n"
    + "    \"enrollment_complete\": false,\n"
    + "    \"real_biometric_template\": false,\n"
    + "    \"placeholder_only\": true\n"
    + "  }\n"
    + "}\n";
}

bool write_text_file_0600(
  const std::string& path,
  const std::string& text,
  std::string& error
) {
  const std::vector<unsigned char> bytes(text.begin(), text.end());
  return face_unlock::write_file_0600(path, bytes, error);
}

int command_status() {
  const std::string path = face_unlock::default_template_path();

  if (path.empty()) {
    std::cout << "template_status: error\n";
    std::cout << "reason: home_unavailable\n";
    return 1;
  }

  const std::string manifest_path = default_enrollment_manifest_path();

  std::cout << "template_path: " << path << "\n";
  std::cout << "manifest_path: " << manifest_path << "\n";

  if (!file_exists(path)) {
    std::cout << "template_status: missing\n";
    std::cout << "manifest_status: " << (file_exists(manifest_path) ? "present" : "missing") << "\n";
    std::cout << "status: ok\n";
    return 0;
  }

  std::cout << "template_status: present\n";
  std::cout << "template_mode: " << mode_string(path) << "\n";
  std::cout << "manifest_status: " << (file_exists(manifest_path) ? "present" : "missing") << "\n";
  if (file_exists(manifest_path)) {
    std::cout << "manifest_mode: " << mode_string(manifest_path) << "\n";
  }

  std::vector<unsigned char> bytes;
  std::string error;

  if (!face_unlock::read_file_bytes(path, bytes, error)) {
    std::cout << "read_status: failed\n";
    std::cout << "read_error: " << error << "\n";
    return 1;
  }

  std::cout << "template_size: " << bytes.size() << "\n";
  std::cout << "status: ok\n";
  return 0;
}

int command_create_placeholder(bool consent, bool overwrite) {
  if (!consent) {
    std::cerr << "ERROR: create-placeholder requires --i-understand-placeholder\n";
    std::cerr << "This creates an encrypted placeholder, not a real biometric template.\n";
    return 1;
  }

  if (!face_unlock::crypto_init()) {
    std::cerr << "crypto_status: init_failed\n";
    return 1;
  }

  const std::string path = face_unlock::default_template_path();

  if (path.empty()) {
    std::cerr << "template_status: error\n";
    std::cerr << "reason: home_unavailable\n";
    return 1;
  }

  if (file_exists(path) && !overwrite) {
    std::cerr << "ERROR: template already exists:\n";
    std::cerr << "  " << path << "\n";
    std::cerr << "Use --overwrite to replace it.\n";
    return 1;
  }

  const fs::path template_path(path);
  const fs::path parent = template_path.parent_path();

  std::error_code ec;
  fs::create_directories(parent, ec);

  if (ec) {
    std::cerr << "mkdir_status: failed\n";
    std::cerr << "mkdir_error: " << ec.message() << "\n";
    return 1;
  }

  fs::permissions(
    parent,
    fs::perms::owner_all,
    fs::perm_options::replace,
    ec
  );

  const std::string placeholder =
    "{"
    "\"format\":\"face-unlock-placeholder-template\","
    "\"version\":1,"
    "\"warning\":\"not a real biometric template\""
    "}";

  const std::vector<unsigned char> plaintext(
    placeholder.begin(),
    placeholder.end()
  );

  const std::vector<unsigned char> key =
    face_unlock::generate_random_key();

  const face_unlock::EncryptedBlob encrypted =
    face_unlock::encrypt_template_bytes(plaintext, key);

  std::string error;

  if (!face_unlock::write_file_0600(path, encrypted.bytes, error)) {
    std::cerr << "write_status: failed\n";
    std::cerr << "write_error: " << error << "\n";
    return 1;
  }

  const std::string manifest_path = default_enrollment_manifest_path();
  const std::string manifest = placeholder_manifest_json(path);

  if (!write_text_file_0600(manifest_path, manifest, error)) {
    std::cerr << "manifest_write_status: failed\n";
    std::cerr << "manifest_write_error: " << error << "\n";
    return 1;
  }

  std::cout << "template_create_status: ok\n";
  std::cout << "template_path: " << path << "\n";
  std::cout << "template_mode: " << mode_string(path) << "\n";
  std::cout << "template_size: " << encrypted.bytes.size() << "\n";
  std::cout << "manifest_create_status: ok\n";
  std::cout << "manifest_path: " << manifest_path << "\n";
  std::cout << "manifest_mode: " << mode_string(manifest_path) << "\n";
  std::cout << "key_status: discarded\n";
  std::cout << "warning: placeholder is encrypted but not decryptable later because the random test key is discarded\n";
  std::cout << "status: ok\n";

  return 0;
}

int command_delete(bool yes) {
  if (!yes) {
    std::cerr << "ERROR: delete requires --yes\n";
    return 1;
  }

  const std::string path = face_unlock::default_template_path();

  if (path.empty()) {
    std::cerr << "template_status: error\n";
    std::cerr << "reason: home_unavailable\n";
    return 1;
  }

  if (!file_exists(path)) {
    std::cout << "template_delete_status: already_missing\n";
    std::cout << "template_path: " << path << "\n";
    std::cout << "status: ok\n";
    return 0;
  }

  const std::string manifest_path = default_enrollment_manifest_path();

  std::error_code ec;
  fs::remove(path, ec);

  if (ec) {
    std::cerr << "template_delete_status: failed\n";
    std::cerr << "delete_error: " << ec.message() << "\n";
    return 1;
  }

  std::error_code manifest_ec;
  const bool manifest_removed = fs::remove(manifest_path, manifest_ec);

  if (manifest_ec) {
    std::cerr << "manifest_delete_status: failed\n";
    std::cerr << "manifest_delete_error: " << manifest_ec.message() << "\n";
    return 1;
  }

  std::cout << "template_delete_status: ok\n";
  std::cout << "template_path: " << path << "\n";
  std::cout << "manifest_delete_status: " << (manifest_removed ? "ok" : "already_missing") << "\n";
  std::cout << "manifest_path: " << manifest_path << "\n";
  std::cout << "status: ok\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  const std::string command = argv[1];

  if (command == "status") {
    return command_status();
  }

  if (command == "create-placeholder") {
    bool consent = false;
    bool overwrite = false;

    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];

      if (arg == "--i-understand-placeholder") {
        consent = true;
      } else if (arg == "--overwrite") {
        overwrite = true;
      } else {
        std::cerr << "unknown_argument: " << arg << "\n";
        print_usage(argv[0]);
        return 1;
      }
    }

    return command_create_placeholder(consent, overwrite);
  }

  if (command == "delete") {
    bool yes = false;

    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];

      if (arg == "--yes") {
        yes = true;
      } else {
        std::cerr << "unknown_argument: " << arg << "\n";
        print_usage(argv[0]);
        return 1;
      }
    }

    return command_delete(yes);
  }

  std::cerr << "unknown_command: " << command << "\n";
  print_usage(argv[0]);
  return 1;
}
