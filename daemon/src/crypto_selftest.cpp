#include "template_crypto.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

int main() {
  try {
    if (!face_unlock::crypto_init()) {
      std::cerr << "crypto_status: init_failed\n";
      return 1;
    }

    std::cout << "crypto_status: initialized\n";

    const std::string plaintext_string =
      "face-unlock-template-placeholder-v1";

    const std::vector<unsigned char> plaintext(
      plaintext_string.begin(),
      plaintext_string.end()
    );

    const std::vector<unsigned char> key =
      face_unlock::generate_random_key();

    const face_unlock::EncryptedBlob encrypted =
      face_unlock::encrypt_template_bytes(plaintext, key);

    std::cout << "encrypt_status: ok\n";
    std::cout << "encrypted_size: " << encrypted.bytes.size() << "\n";

    const fs::path test_path =
      fs::temp_directory_path() /
      ("face-unlock-template-selftest-" + std::to_string(getpid()) + ".enc");

    std::string error;

    if (!face_unlock::write_file_0600(test_path.string(), encrypted.bytes, error)) {
      std::cerr << "write_status: failed\n";
      std::cerr << "write_error: " << error << "\n";
      return 1;
    }

    std::cout << "write_status: ok\n";
    std::cout << "test_path: " << test_path.string() << "\n";

    std::vector<unsigned char> loaded;

    if (!face_unlock::read_file_bytes(test_path.string(), loaded, error)) {
      std::cerr << "read_status: failed\n";
      std::cerr << "read_error: " << error << "\n";
      fs::remove(test_path);
      return 1;
    }

    std::cout << "read_status: ok\n";
    std::cout << "loaded_size: " << loaded.size() << "\n";

    face_unlock::EncryptedBlob loaded_blob;
    loaded_blob.bytes = loaded;

    const std::vector<unsigned char> decrypted =
      face_unlock::decrypt_template_bytes(loaded_blob, key);

    if (decrypted != plaintext) {
      std::cerr << "decrypt_status: mismatch\n";
      fs::remove(test_path);
      return 1;
    }

    std::cout << "decrypt_status: ok\n";

    fs::remove(test_path);

    std::cout << "cleanup_status: ok\n";
    std::cout << "status: ok\n";

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "status: error\n";
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
}
