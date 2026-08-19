#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace face_unlock {

struct EncryptedBlob {
  std::vector<unsigned char> bytes;
};

bool crypto_init();

std::vector<unsigned char> generate_random_key();

EncryptedBlob encrypt_template_bytes(
  const std::vector<unsigned char>& plaintext,
  const std::vector<unsigned char>& key
);

std::vector<unsigned char> decrypt_template_bytes(
  const EncryptedBlob& blob,
  const std::vector<unsigned char>& key
);

bool write_file_0600(
  const std::string& path,
  const std::vector<unsigned char>& bytes,
  std::string& error
);

bool read_file_bytes(
  const std::string& path,
  std::vector<unsigned char>& bytes,
  std::string& error
);

std::string default_template_path();

}  // namespace face_unlock
