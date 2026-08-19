#include "template_crypto.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <fcntl.h>
#include <sodium.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace face_unlock {
namespace {

constexpr unsigned char kMagic[] = {
  'F', 'U', 'L', 'T', 'P', 'L', '1', 0
};

constexpr std::size_t kMagicSize = sizeof(kMagic);
constexpr std::size_t kNonceSize = crypto_secretbox_NONCEBYTES;
constexpr std::size_t kKeySize = crypto_secretbox_KEYBYTES;
constexpr std::size_t kMacSize = crypto_secretbox_MACBYTES;

void require_crypto_ready() {
  if (!crypto_init()) {
    throw std::runtime_error("libsodium initialization failed");
  }
}

void require_key_size(const std::vector<unsigned char>& key) {
  if (key.size() != kKeySize) {
    throw std::runtime_error("invalid key size");
  }
}

bool write_all(int fd, const unsigned char* data, std::size_t size) {
  std::size_t offset = 0;

  while (offset < size) {
    const ssize_t written = ::write(fd, data + offset, size - offset);

    if (written <= 0) {
      return false;
    }

    offset += static_cast<std::size_t>(written);
  }

  return true;
}

}  // namespace

bool crypto_init() {
  static const int init_result = sodium_init();
  return init_result >= 0;
}

std::vector<unsigned char> generate_random_key() {
  require_crypto_ready();

  std::vector<unsigned char> key(kKeySize);
  randombytes_buf(key.data(), key.size());

  return key;
}

EncryptedBlob encrypt_template_bytes(
  const std::vector<unsigned char>& plaintext,
  const std::vector<unsigned char>& key
) {
  require_crypto_ready();
  require_key_size(key);

  std::vector<unsigned char> nonce(kNonceSize);
  randombytes_buf(nonce.data(), nonce.size());

  std::vector<unsigned char> ciphertext(plaintext.size() + kMacSize);

  if (crypto_secretbox_easy(
        ciphertext.data(),
        plaintext.data(),
        plaintext.size(),
        nonce.data(),
        key.data()
      ) != 0) {
    throw std::runtime_error("template encryption failed");
  }

  EncryptedBlob blob;

  blob.bytes.reserve(kMagicSize + nonce.size() + ciphertext.size());
  blob.bytes.insert(blob.bytes.end(), std::begin(kMagic), std::end(kMagic));
  blob.bytes.insert(blob.bytes.end(), nonce.begin(), nonce.end());
  blob.bytes.insert(blob.bytes.end(), ciphertext.begin(), ciphertext.end());

  return blob;
}

std::vector<unsigned char> decrypt_template_bytes(
  const EncryptedBlob& blob,
  const std::vector<unsigned char>& key
) {
  require_crypto_ready();
  require_key_size(key);

  if (blob.bytes.size() < kMagicSize + kNonceSize + kMacSize) {
    throw std::runtime_error("encrypted template blob too small");
  }

  if (std::memcmp(blob.bytes.data(), kMagic, kMagicSize) != 0) {
    throw std::runtime_error("encrypted template magic mismatch");
  }

  const unsigned char* nonce = blob.bytes.data() + kMagicSize;
  const unsigned char* ciphertext = nonce + kNonceSize;
  const std::size_t ciphertext_size = blob.bytes.size() - kMagicSize - kNonceSize;

  std::vector<unsigned char> plaintext(ciphertext_size - kMacSize);

  if (crypto_secretbox_open_easy(
        plaintext.data(),
        ciphertext,
        ciphertext_size,
        nonce,
        key.data()
      ) != 0) {
    throw std::runtime_error("template decryption failed");
  }

  return plaintext;
}

bool write_file_0600(
  const std::string& path,
  const std::vector<unsigned char>& bytes,
  std::string& error
) {
  const int fd = ::open(
    path.c_str(),
    O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
    S_IRUSR | S_IWUSR
  );

  if (fd < 0) {
    error = std::strerror(errno);
    return false;
  }

  const bool ok = write_all(fd, bytes.data(), bytes.size());

  if (!ok) {
    error = std::strerror(errno);
    ::close(fd);
    return false;
  }

  if (::fsync(fd) < 0) {
    error = std::strerror(errno);
    ::close(fd);
    return false;
  }

  if (::close(fd) < 0) {
    error = std::strerror(errno);
    return false;
  }

  return true;
}

bool read_file_bytes(
  const std::string& path,
  std::vector<unsigned char>& bytes,
  std::string& error
) {
  const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);

  if (fd < 0) {
    error = std::strerror(errno);
    return false;
  }

  bytes.clear();

  std::array<unsigned char, 4096> buffer {};

  while (true) {
    const ssize_t n = ::read(fd, buffer.data(), buffer.size());

    if (n < 0) {
      error = std::strerror(errno);
      ::close(fd);
      return false;
    }

    if (n == 0) {
      break;
    }

    bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + n);
  }

  if (::close(fd) < 0) {
    error = std::strerror(errno);
    return false;
  }

  return true;
}

std::string default_template_path() {
  const char* home = std::getenv("HOME");

  if (home == nullptr || std::string(home).empty()) {
    return "";
  }

  return std::string(home) + "/.local/share/face-unlock/template.enc";
}

}  // namespace face_unlock
