#include "template_crypto.h"

#include <array>
#include <cerrno>
#include <cstdio>
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
constexpr std::size_t kMaximumStoredFileSize = 1024 * 1024;

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

    if (written < 0 && errno == EINTR) {
      continue;
    }
    if (written <= 0) {
      return false;
    }

    offset += static_cast<std::size_t>(written);
  }

  return true;
}

std::string parent_directory(const std::string& path) {
  const std::size_t separator = path.find_last_of('/');
  if (separator == std::string::npos) {
    return ".";
  }
  if (separator == 0) {
    return "/";
  }
  return path.substr(0, separator);
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
  error.clear();

  if (path.empty() || path.back() == '/') {
    error = "invalid destination path";
    return false;
  }
  if (bytes.size() > kMaximumStoredFileSize) {
    error = "file exceeds maximum supported size";
    return false;
  }

  struct stat existing {};
  if (::lstat(path.c_str(), &existing) == 0) {
    if (!S_ISREG(existing.st_mode)) {
      error = "refusing to replace non-regular file";
      return false;
    }
  } else if (errno != ENOENT) {
    error = std::strerror(errno);
    return false;
  }

  std::string temporary_path = path + ".tmp.XXXXXX";
  std::vector<char> temporary_name(
    temporary_path.begin(),
    temporary_path.end()
  );
  temporary_name.push_back('\0');

  const int fd = ::mkstemp(temporary_name.data());
  if (fd < 0) {
    error = std::strerror(errno);
    return false;
  }
  temporary_path = temporary_name.data();

  const auto fail_before_rename = [&](const std::string& message) {
    error = message;
    ::close(fd);
    ::unlink(temporary_path.c_str());
    return false;
  };

  if (::fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
    return fail_before_rename(std::strerror(errno));
  }
  if (::fchmod(fd, S_IRUSR | S_IWUSR) < 0) {
    return fail_before_rename(std::strerror(errno));
  }
  if (!write_all(fd, bytes.data(), bytes.size())) {
    return fail_before_rename(std::strerror(errno));
  }
  if (::fsync(fd) < 0) {
    return fail_before_rename(std::strerror(errno));
  }
  if (::close(fd) < 0) {
    error = std::strerror(errno);
    ::unlink(temporary_path.c_str());
    return false;
  }

  if (::rename(temporary_path.c_str(), path.c_str()) < 0) {
    error = std::strerror(errno);
    ::unlink(temporary_path.c_str());
    return false;
  }

  const std::string directory = parent_directory(path);
  const int directory_fd = ::open(
    directory.c_str(),
    O_RDONLY | O_DIRECTORY | O_CLOEXEC
  );
  if (directory_fd < 0) {
    error = std::strerror(errno);
    return false;
  }
  if (::fsync(directory_fd) < 0) {
    error = std::strerror(errno);
    ::close(directory_fd);
    return false;
  }
  if (::close(directory_fd) < 0) {
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
  error.clear();

  const int fd = ::open(
    path.c_str(),
    O_RDONLY | O_CLOEXEC | O_NOFOLLOW
  );
  if (fd < 0) {
    error = std::strerror(errno);
    return false;
  }

  struct stat metadata {};
  if (::fstat(fd, &metadata) < 0) {
    error = std::strerror(errno);
    ::close(fd);
    return false;
  }
  if (!S_ISREG(metadata.st_mode)) {
    error = "refusing to read non-regular file";
    ::close(fd);
    return false;
  }
  if (metadata.st_size < 0 ||
      static_cast<std::uintmax_t>(metadata.st_size) > kMaximumStoredFileSize) {
    error = "file exceeds maximum supported size";
    ::close(fd);
    return false;
  }

  bytes.clear();
  bytes.reserve(static_cast<std::size_t>(metadata.st_size));
  std::array<unsigned char, 4096> buffer {};

  while (true) {
    const ssize_t count = ::read(fd, buffer.data(), buffer.size());

    if (count < 0 && errno == EINTR) {
      continue;
    }
    if (count < 0) {
      error = std::strerror(errno);
      ::close(fd);
      return false;
    }
    if (count == 0) {
      break;
    }
    if (static_cast<std::size_t>(count) >
        kMaximumStoredFileSize - bytes.size()) {
      error = "file exceeds maximum supported size";
      ::close(fd);
      bytes.clear();
      return false;
    }

    bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + count);
  }

  if (::close(fd) < 0) {
    error = std::strerror(errno);
    bytes.clear();
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
