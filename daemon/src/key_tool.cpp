#include "template_crypto.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <sodium.h>

namespace fs = std::filesystem;

namespace {

std::string default_key_path() {
  const char* home = std::getenv("HOME");

  if (home == nullptr || std::string(home).empty()) {
    return "";
  }

  return std::string(home) + "/.local/share/face-unlock/template.key";
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

void print_usage(const char* program) {
  std::cout << "Usage:\n";
  std::cout << "  " << program << " status\n";
  std::cout << "  " << program << " create-dev-key --i-understand-dev-key-risk [--overwrite]\n";
  std::cout << "  " << program << " delete --yes\n";
  std::cout << "\n";
  std::cout << "This is a development key management scaffold.\n";
  std::cout << "It is not final production key management.\n";
}

int command_status() {
  const std::string path = default_key_path();

  if (path.empty()) {
    std::cout << "key_status: error\n";
    std::cout << "reason: home_unavailable\n";
    return 1;
  }

  std::cout << "key_path: " << path << "\n";

  if (!file_exists(path)) {
    std::cout << "key_status: missing\n";
    std::cout << "status: ok\n";
    return 0;
  }

  std::vector<unsigned char> bytes;
  std::string error;

  if (!face_unlock::read_file_bytes(path, bytes, error)) {
    std::cout << "key_status: unreadable\n";
    std::cout << "read_error: " << error << "\n";
    return 1;
  }

  std::cout << "key_status: present\n";
  std::cout << "key_mode: " << mode_string(path) << "\n";
  std::cout << "key_size: " << bytes.size() << "\n";

  if (bytes.size() == crypto_secretbox_KEYBYTES) {
    std::cout << "key_size_status: ok\n";
  } else {
    std::cout << "key_size_status: unexpected\n";
  }

  std::cout << "status: ok\n";
  return 0;
}

int command_create_dev_key(bool consent, bool overwrite) {
  if (!consent) {
    std::cerr << "ERROR: create-dev-key requires --i-understand-dev-key-risk\n";
    std::cerr << "This creates a raw local development key file.\n";
    return 1;
  }

  if (!face_unlock::crypto_init()) {
    std::cerr << "crypto_status: init_failed\n";
    return 1;
  }

  const std::string path = default_key_path();

  if (path.empty()) {
    std::cerr << "key_status: error\n";
    std::cerr << "reason: home_unavailable\n";
    return 1;
  }

  if (file_exists(path) && !overwrite) {
    std::cerr << "ERROR: key already exists:\n";
    std::cerr << "  " << path << "\n";
    std::cerr << "Use --overwrite to replace it.\n";
    return 1;
  }

  const fs::path key_path(path);
  const fs::path parent = key_path.parent_path();

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

  const std::vector<unsigned char> key = face_unlock::generate_random_key();

  std::string error;

  if (!face_unlock::write_file_0600(path, key, error)) {
    std::cerr << "key_write_status: failed\n";
    std::cerr << "key_write_error: " << error << "\n";
    return 1;
  }

  std::cout << "key_create_status: ok\n";
  std::cout << "key_path: " << path << "\n";
  std::cout << "key_mode: " << mode_string(path) << "\n";
  std::cout << "key_size: " << key.size() << "\n";
  std::cout << "warning: raw local development key; not production key management\n";
  std::cout << "status: ok\n";

  return 0;
}

int command_delete(bool yes) {
  if (!yes) {
    std::cerr << "ERROR: delete requires --yes\n";
    return 1;
  }

  const std::string path = default_key_path();

  if (path.empty()) {
    std::cerr << "key_status: error\n";
    std::cerr << "reason: home_unavailable\n";
    return 1;
  }

  if (!file_exists(path)) {
    std::cout << "key_delete_status: already_missing\n";
    std::cout << "key_path: " << path << "\n";
    std::cout << "status: ok\n";
    return 0;
  }

  std::error_code ec;
  fs::remove(path, ec);

  if (ec) {
    std::cerr << "key_delete_status: failed\n";
    std::cerr << "delete_error: " << ec.message() << "\n";
    return 1;
  }

  std::cout << "key_delete_status: ok\n";
  std::cout << "key_path: " << path << "\n";
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

  if (command == "create-dev-key") {
    bool consent = false;
    bool overwrite = false;

    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];

      if (arg == "--i-understand-dev-key-risk") {
        consent = true;
      } else if (arg == "--overwrite") {
        overwrite = true;
      } else {
        std::cerr << "unknown_argument: " << arg << "\n";
        print_usage(argv[0]);
        return 1;
      }
    }

    return command_create_dev_key(consent, overwrite);
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
