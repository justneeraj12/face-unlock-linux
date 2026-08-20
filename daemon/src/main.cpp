#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>

#include <cerrno>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include "template_crypto.h"

#include <sodium.h>

#ifdef FACE_UNLOCK_WITH_TORCH
#include <torch/script.h>
#include <torch/torch.h>
#endif

namespace {

std::string read_text_file(const std::string& path);

std::atomic<bool> g_running{true};

void handle_signal(int signal_number) {
  (void)signal_number;
  g_running = false;
}

struct Options {
  int camera_index = 0;
  bool camera_index_set = false;
  bool loop = false;
  bool serve = false;
  bool daemon = false;
  bool model_test = false;
  std::string model_path = "models/embedding_stub.pt";
};

struct AppConfig {
  bool loaded = false;
  std::string path;
  int camera_index = 0;
  int max_auth_attempts = 3;
};

struct FrameStore {
  std::mutex mutex;
  cv::Mat latest_frame;
  unsigned long long frames_total = 0;
  bool has_frame = false;

  void update(const cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(mutex);
    frame.copyTo(latest_frame);
    ++frames_total;
    has_frame = true;
  }

  bool snapshot(cv::Mat& out_frame, unsigned long long& out_frames_total) {
    std::lock_guard<std::mutex> lock(mutex);

    out_frames_total = frames_total;

    if (!has_frame || latest_frame.empty()) {
      return false;
    }

    latest_frame.copyTo(out_frame);
    return true;
  }
};

struct AuthState {
  explicit AuthState(int max_attempts_in)
      : max_attempts(max_attempts_in) {}

  std::mutex mutex;
  int max_attempts = 3;
  int failed_attempts = 0;

  int remaining_locked() const {
    const int remaining = max_attempts - failed_attempts;
    return remaining > 0 ? remaining : 0;
  }

  void reset() {
    std::lock_guard<std::mutex> lock(mutex);
    failed_attempts = 0;
  }

  int record_failure_and_remaining() {
    std::lock_guard<std::mutex> lock(mutex);

    if (failed_attempts < max_attempts) {
      ++failed_attempts;
    }

    return remaining_locked();
  }

  bool too_many_attempts() {
    std::lock_guard<std::mutex> lock(mutex);
    return failed_attempts >= max_attempts;
  }

  int failed_count() {
    std::lock_guard<std::mutex> lock(mutex);
    return failed_attempts;
  }

  int remaining() {
    std::lock_guard<std::mutex> lock(mutex);
    return remaining_locked();
  }
};

std::string get_runtime_dir() {
  const char* xdg_runtime_dir = std::getenv("XDG_RUNTIME_DIR");

  if (xdg_runtime_dir != nullptr && std::string(xdg_runtime_dir).size() > 0) {
    return std::string(xdg_runtime_dir);
  }

  return "/run/user/" + std::to_string(getuid());
}

std::string get_socket_path() {
  return get_runtime_dir() + "/face-unlock.sock";
}

std::string get_default_config_path() {
  const char* home = std::getenv("HOME");

  if (home == nullptr || std::string(home).empty()) {
    return "";
  }

  return std::string(home) + "/.config/face-unlock/config.json";
}

std::string get_default_template_path() {
  const char* home = std::getenv("HOME");

  if (home == nullptr || std::string(home).empty()) {
    return "";
  }

  return std::string(home) + "/.local/share/face-unlock/template.enc";
}

bool template_file_exists() {
  const std::string path = get_default_template_path();

  if (path.empty()) {
    return false;
  }

  return std::filesystem::is_regular_file(path);
}

std::string template_json_fields() {
  return std::string(",\"template\":\"") +
    (template_file_exists() ? "present" : "missing") +
    "\"";
}

std::string get_default_enrollment_manifest_path() {
  const char* home = std::getenv("HOME");

  if (home == nullptr || std::string(home).empty()) {
    return "";
  }

  return std::string(home) + "/.local/share/face-unlock/enrollment.json";
}

std::string compact_metadata_text(const std::string& input) {
  std::string compact;
  compact.reserve(input.size());

  for (char ch : input) {
    if (ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t') {
      compact.push_back(ch);
    }
  }

  return compact;
}

std::string enrollment_status_value() {
  const std::string path = get_default_enrollment_manifest_path();

  if (path.empty()) {
    return "missing";
  }

  if (!std::filesystem::is_regular_file(path)) {
    return "missing";
  }

  const std::string content = read_text_file(path);

  if (content.empty()) {
    return "unreadable";
  }

  const std::string compact = compact_metadata_text(content);

  if (compact.find("\"placeholder_only\":true") != std::string::npos) {
    return "placeholder";
  }

  if (compact.find("\"real_biometric_template\":true") != std::string::npos &&
      compact.find("\"enrollment_complete\":true") != std::string::npos) {
    return "real";
  }

  return "present_unknown";
}

std::string enrollment_json_fields() {
  return std::string(",\"enrollment\":\"") + enrollment_status_value() + "\"";
}

std::string get_default_key_path() {
  const char* home = std::getenv("HOME");

  if (home == nullptr || std::string(home).empty()) {
    return "";
  }

  return std::string(home) + "/.local/share/face-unlock/template.key";
}

bool key_file_exists() {
  const std::string path = get_default_key_path();

  if (path.empty()) {
    return false;
  }

  return std::filesystem::is_regular_file(path);
}

std::string manifest_key_storage_status() {
  const std::string path = get_default_enrollment_manifest_path();

  if (path.empty() || !std::filesystem::is_regular_file(path)) {
    return "manifest_missing";
  }

  const std::string content = read_text_file(path);

  if (content.empty()) {
    return "manifest_unreadable";
  }

  const std::string compact = compact_metadata_text(content);

  if (compact.find("\"key_storage\":\"local_development_key_file\"") != std::string::npos) {
    return "local_development_key_file";
  }

  if (compact.find("\"key_storage\":\"discarded_random_key\"") != std::string::npos) {
    return "discarded_random_key";
  }

  return "unknown";
}

std::string decryptability_status_value() {
  if (!template_file_exists()) {
    return "template_missing";
  }

  const std::string key_storage = manifest_key_storage_status();

  if (key_storage == "local_development_key_file") {
    return key_file_exists() ? "possible_with_dev_key" : "key_missing";
  }

  if (key_storage == "discarded_random_key") {
    return "not_possible_discarded_key";
  }

  return "unknown";
}

std::string key_json_fields() {
  return std::string(",\"key\":\"") +
    (key_file_exists() ? "present" : "missing") +
    "\",\"decryptability\":\"" +
    decryptability_status_value() +
    "\",\"key_storage\":\"" +
    manifest_key_storage_status() +
    "\"";
}

bool read_dev_key_bytes(std::vector<unsigned char>& key) {
  const std::string path = get_default_key_path();

  if (path.empty()) {
    return false;
  }

  std::string error;

  if (!face_unlock::read_file_bytes(path, key, error)) {
    return false;
  }

  return key.size() == crypto_secretbox_KEYBYTES;
}

std::string template_decrypt_status_value() {
  const std::string decryptability = decryptability_status_value();

  if (decryptability == "template_missing") {
    return "template_missing";
  }

  if (decryptability == "not_possible_discarded_key") {
    return "not_possible_discarded_key";
  }

  if (decryptability == "key_missing") {
    return "key_missing";
  }

  if (decryptability != "possible_with_dev_key") {
    return "not_checked";
  }

  std::vector<unsigned char> key;

  if (!read_dev_key_bytes(key)) {
    return "key_unavailable";
  }

  std::vector<unsigned char> encrypted;
  std::string error;

  if (!face_unlock::read_file_bytes(get_default_template_path(), encrypted, error)) {
    return "template_read_failed";
  }

  try {
    face_unlock::EncryptedBlob blob;
    blob.bytes = encrypted;

    const std::vector<unsigned char> plaintext =
      face_unlock::decrypt_template_bytes(blob, key);

    (void)plaintext;
    return "ok";
  } catch (const std::exception&) {
    return "failed";
  }
}

std::string template_decrypt_json_fields() {
  return std::string(",\"template_decrypt\":\"") +
    template_decrypt_status_value() +
    "\"";
}


std::string read_text_file(const std::string& path) {
  std::ifstream input(path);

  if (!input) {
    return "";
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

std::optional<int> extract_int_config_value(
  const std::string& content,
  const std::string& key
) {
  const std::string quoted_key = "\"" + key + "\"";
  const std::size_t key_pos = content.find(quoted_key);

  if (key_pos == std::string::npos) {
    return std::nullopt;
  }

  const std::size_t colon_pos = content.find(':', key_pos);

  if (colon_pos == std::string::npos) {
    return std::nullopt;
  }

  std::size_t value_pos = colon_pos + 1;

  while (value_pos < content.size() &&
         (content[value_pos] == ' ' ||
          content[value_pos] == '\n' ||
          content[value_pos] == '\r' ||
          content[value_pos] == '\t')) {
    ++value_pos;
  }

  if (value_pos >= content.size()) {
    return std::nullopt;
  }

  char* end_ptr = nullptr;
  const long value = std::strtol(content.c_str() + value_pos, &end_ptr, 10);

  if (end_ptr == content.c_str() + value_pos) {
    return std::nullopt;
  }

  return static_cast<int>(value);
}

AppConfig load_app_config() {
  AppConfig config;
  config.path = get_default_config_path();

  if (config.path.empty()) {
    return config;
  }

  const std::string content = read_text_file(config.path);

  if (content.empty()) {
    return config;
  }

  config.loaded = true;

  const std::optional<int> camera_index =
    extract_int_config_value(content, "camera_index");

  if (camera_index.has_value() && camera_index.value() >= 0) {
    config.camera_index = camera_index.value();
  }

  const std::optional<int> max_auth_attempts =
    extract_int_config_value(content, "max_auth_attempts");

  if (max_auth_attempts.has_value() &&
      max_auth_attempts.value() >= 1 &&
      max_auth_attempts.value() <= 10) {
    config.max_auth_attempts = max_auth_attempts.value();
  }

  return config;
}


void print_usage(const char* program_name) {
  std::cout << "Usage: " << program_name
            << " [--camera INDEX] [--loop] [--serve] [--daemon]\n";
  std::cout << "Options:\n";
  std::cout << "  --camera, -c INDEX   Camera index to open. Default: 0\n";
  std::cout << "  --loop               Keep reading frames until Ctrl+C\n";
  std::cout << "  --serve              Run local UNIX socket server until Ctrl+C\n";
  std::cout << "  --daemon             Run camera worker and socket server together\n";
  std::cout << "  --model-test         Load TorchScript model and run dummy forward pass\n";
  std::cout << "  --model PATH         TorchScript model path. Default: models/embedding_stub.pt\n";
  std::cout << "  --help, -h           Show this help text\n";
}

Options parse_options(int argc, char** argv) {
  Options options;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];

    if ((arg == "--camera" || arg == "-c") && i + 1 < argc) {
      options.camera_index = std::atoi(argv[i + 1]);
      options.camera_index_set = true;
      ++i;
    } else if (arg == "--loop") {
      options.loop = true;
    } else if (arg == "--serve") {
      options.serve = true;
    } else if (arg == "--daemon") {
      options.daemon = true;
    } else if (arg == "--model-test") {
      options.model_test = true;
    } else if (arg == "--model" && i + 1 < argc) {
      options.model_path = argv[i + 1];
      ++i;
    } else if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      std::exit(0);
    } else {
      std::cerr << "unknown_argument: " << arg << '\n';
      print_usage(argv[0]);
      std::exit(1);
    }
  }

  const int mode_count =
    static_cast<int>(options.loop) +
    static_cast<int>(options.serve) +
    static_cast<int>(options.daemon) +
    static_cast<int>(options.model_test);

  if (mode_count > 1) {
    std::cerr << "error: choose only one of --loop, --serve, --daemon, or --model-test\n";
    std::exit(1);
  }

  return options;
}

bool open_camera(cv::VideoCapture& camera, int camera_index) {
  std::cout << "camera_index: " << camera_index << '\n';

  if (!camera.open(camera_index, cv::CAP_V4L2)) {
    std::cout << "camera_status: open_failed" << '\n';
    return false;
  }

  camera.set(cv::CAP_PROP_FRAME_WIDTH, 640);
  camera.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

  std::cout << "camera_status: opened" << '\n';
  return true;
}

bool read_frame_with_warmup(cv::VideoCapture& camera, cv::Mat& frame) {
  for (int attempt = 1; attempt <= 30; ++attempt) {
    if (camera.read(frame) && !frame.empty()) {
      return true;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return false;
}

bool run_one_shot(cv::VideoCapture& camera) {
  cv::Mat frame;

  if (!read_frame_with_warmup(camera, frame)) {
    std::cout << "frame_status: read_failed" << '\n';
    return false;
  }

  std::cout << "frame_status: ok" << '\n';
  std::cout << "frame_width: " << frame.cols << '\n';
  std::cout << "frame_height: " << frame.rows << '\n';
  std::cout << "frame_channels: " << frame.channels() << '\n';

  return true;
}

bool run_loop(cv::VideoCapture& camera) {
  std::cout << "loop_status: started" << '\n';
  std::cout << "stop_hint: press Ctrl+C to stop" << '\n';

  using clock = std::chrono::steady_clock;

  auto last_report = clock::now();
  unsigned long long frames_total = 0;
  unsigned long long frames_since_report = 0;

  while (g_running) {
    cv::Mat frame;

    if (!camera.read(frame) || frame.empty()) {
      std::cout << "loop_warning: empty_frame" << '\n';
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      continue;
    }

    ++frames_total;
    ++frames_since_report;

    const auto now = clock::now();
    const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - last_report)
        .count();

    if (elapsed_ms >= 1000) {
      const double fps = 1000.0 * static_cast<double>(frames_since_report) /
                         static_cast<double>(elapsed_ms);

      std::cout << "loop_report:"
                << " frames_total=" << frames_total
                << " fps=" << fps
                << " width=" << frame.cols
                << " height=" << frame.rows
                << " channels=" << frame.channels()
                << '\n';

      frames_since_report = 0;
      last_report = now;
    }
  }

  std::cout << "loop_status: stopping" << '\n';
  std::cout << "frames_total: " << frames_total << '\n';
  return frames_total > 0;
}

int create_server_socket(const std::string& socket_path) {
  const int server_fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

  if (server_fd < 0) {
    std::cerr << "socket_error: " << std::strerror(errno) << '\n';
    return -1;
  }

  ::unlink(socket_path.c_str());

  sockaddr_un address {};
  address.sun_family = AF_UNIX;

  if (socket_path.size() >= sizeof(address.sun_path)) {
    std::cerr << "socket_path_error: path_too_long\n";
    ::close(server_fd);
    return -1;
  }

  std::strncpy(address.sun_path, socket_path.c_str(), sizeof(address.sun_path) - 1);

  const mode_t old_umask = ::umask(0077);

  if (::bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
    std::cerr << "bind_error: " << std::strerror(errno) << '\n';
    ::umask(old_umask);
    ::close(server_fd);
    return -1;
  }

  ::umask(old_umask);

  if (::chmod(socket_path.c_str(), 0600) < 0) {
    std::cerr << "chmod_warning: " << std::strerror(errno) << '\n';
  }

  if (::listen(server_fd, 8) < 0) {
    std::cerr << "listen_error: " << std::strerror(errno) << '\n';
    ::close(server_fd);
    ::unlink(socket_path.c_str());
    return -1;
  }

  return server_fd;
}

bool get_peer_credentials(int client_fd, ucred& credentials) {
  std::memset(&credentials, 0, sizeof(credentials));

  socklen_t length = sizeof(credentials);

  if (::getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &credentials, &length) < 0) {
    std::cerr << "peer_credentials_error: " << std::strerror(errno) << '\n';
    return false;
  }

  return true;
}

void write_json_response(int client_fd, const std::string& response) {
  const ssize_t bytes_written = ::write(client_fd, response.data(), response.size());

  if (bytes_written < 0) {
    std::cerr << "write_warning: " << std::strerror(errno) << '\n';
  }
}

bool root_auth_peer_enabled() {
  const char* value = std::getenv("FACE_UNLOCK_ALLOW_ROOT_AUTH");

  return value != nullptr && std::string(value) == "1";
}

std::string peer_policy_decision(
  const ucred& credentials,
  const std::string& operation
) {
  const uid_t daemon_uid = getuid();

  if (credentials.uid == daemon_uid) {
    return "allow_same_uid";
  }

  // sudo/PAM may connect as root.
  //
  // Root auth peers are allowed only when explicitly enabled.
  // This keeps sudo integration opt-in.
  if (credentials.uid == 0 && operation == "auth" && root_auth_peer_enabled()) {
    return "allow_root_auth";
  }

  if (credentials.uid == 0 && operation == "auth") {
    return "reject_root_auth_disabled";
  }

  return "reject";
}

bool peer_is_allowed_for_operation(
  const ucred& credentials,
  const std::string& operation
) {
  return peer_policy_decision(credentials, operation) != "reject";
}

struct CameraStatus {
  std::string state = "not_attached";
  unsigned long long frames_total = 0;
  int frame_width = 0;
  int frame_height = 0;
  int frame_channels = 0;
};

CameraStatus get_camera_status(FrameStore* frame_store) {
  CameraStatus status;

  if (frame_store == nullptr) {
    status.state = "not_attached";
    return status;
  }

  cv::Mat snapshot;
  const bool has_frame = frame_store->snapshot(snapshot, status.frames_total);

  if (!has_frame) {
    status.state = "not_ready";
    return status;
  }

  status.state = "ready";
  status.frame_width = snapshot.cols;
  status.frame_height = snapshot.rows;
  status.frame_channels = snapshot.channels();

  return status;
}

std::string camera_status_json_fields(const CameraStatus& camera_status) {
  std::string fields =
    ",\"camera\":\"" + camera_status.state + "\""
    + ",\"frames_total\":" + std::to_string(camera_status.frames_total);

  if (camera_status.state == "ready") {
    fields +=
      ",\"frame_width\":" + std::to_string(camera_status.frame_width)
      + ",\"frame_height\":" + std::to_string(camera_status.frame_height)
      + ",\"frame_channels\":" + std::to_string(camera_status.frame_channels);
  }

  return fields;
}

std::string compact_jsonish(const std::string& request) {
  std::string compact;
  compact.reserve(request.size());

  for (char ch : request) {
    if (ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t') {
      compact.push_back(ch);
    }
  }

  return compact;
}

std::string extract_operation(const std::string& request) {
  const std::string compact = compact_jsonish(request);

  if (compact.find("\"op\":\"template_status\"") != std::string::npos) {
    return "template_status";
  }

  if (compact.find("\"op\":\"camera_status\"") != std::string::npos) {
    return "camera_status";
  }

  if (compact.find("\"op\":\"auth\"") != std::string::npos) {
    return "auth";
  }

  if (compact.find("\"op\":\"ping\"") != std::string::npos) {
    return "ping";
  }

  return "unknown";
}

bool dev_auth_enabled() {
  const char* value = std::getenv("FACE_UNLOCK_DEV_ALLOW");

  return value != nullptr && std::string(value) == "1";
}

std::string build_response_for_request(const std::string& request, FrameStore* frame_store, AuthState* auth_state) {
  const std::string op = extract_operation(request);
  const CameraStatus camera_status = get_camera_status(frame_store);
  const std::string camera_fields = camera_status_json_fields(camera_status);
  const std::string template_fields = template_json_fields();
  const std::string enrollment_fields = enrollment_json_fields();
  const std::string key_fields = key_json_fields();

  if (op == "ping") {
    return "{\"status\":\"ok\",\"op\":\"ping\",\"reason\":\"daemon_alive\""
      + camera_fields + template_fields + enrollment_fields + key_fields + "}\n";
  }

  if (op == "template_status") {
    return "{\"status\":\"ok\",\"op\":\"template_status\""
      + camera_fields
      + template_fields
      + enrollment_fields
      + key_fields
      + template_decrypt_json_fields()
      + "}\n";
  }

  if (op == "camera_status") {
    return "{\"status\":\"ok\",\"op\":\"camera_status\""
      + camera_fields + template_fields + enrollment_fields + key_fields + "}\n";
  }

  if (op == "auth") {
    if (auth_state == nullptr) {
      return "{\"status\":\"fail\",\"op\":\"auth\",\"reason\":\"auth_state_unavailable\""
        + camera_fields + template_fields + enrollment_fields + key_fields + "}\n";
    }

    if (dev_auth_enabled() && camera_status.state == "ready") {
      auth_state->reset();

      return "{\"status\":\"ok\",\"op\":\"auth\",\"reason\":\"dev_allow_camera_ready\""
        + camera_fields
        + template_fields + enrollment_fields + key_fields
        + ",\"auth_attempts_failed\":0"
        + ",\"auth_attempts_remaining\":" + std::to_string(auth_state->remaining())
        + "}\n";
    }

    if (auth_state->too_many_attempts()) {
      return "{\"status\":\"fail\",\"op\":\"auth\",\"reason\":\"too_many_attempts\""
        + camera_fields
        + template_fields + enrollment_fields + key_fields
        + ",\"auth_attempts_failed\":" + std::to_string(auth_state->failed_count())
        + ",\"auth_attempts_remaining\":0"
        + "}\n";
    }

    const int attempts_remaining = auth_state->record_failure_and_remaining();

    if (dev_auth_enabled() && camera_status.state != "ready") {
      return "{\"status\":\"fail\",\"op\":\"auth\",\"reason\":\"camera_not_ready\""
        + camera_fields
        + template_fields + enrollment_fields + key_fields
        + ",\"auth_attempts_failed\":" + std::to_string(auth_state->failed_count())
        + ",\"auth_attempts_remaining\":" + std::to_string(attempts_remaining)
        + "}\n";
    }

    if (!template_file_exists()) {
      return "{\"status\":\"fail\",\"op\":\"auth\",\"reason\":\"template_missing\""
        + camera_fields
        + template_fields + enrollment_fields + key_fields
        + ",\"auth_attempts_failed\":" + std::to_string(auth_state->failed_count())
        + ",\"auth_attempts_remaining\":" + std::to_string(attempts_remaining)
        + "}\n";
    }

    const std::string decryptability = decryptability_status_value();

    if (decryptability == "not_possible_discarded_key") {
      return "{\"status\":\"fail\",\"op\":\"auth\",\"reason\":\"template_not_decryptable\""
        + camera_fields
        + template_fields + enrollment_fields + key_fields
        + ",\"auth_attempts_failed\":" + std::to_string(auth_state->failed_count())
        + ",\"auth_attempts_remaining\":" + std::to_string(attempts_remaining)
        + "}\n";
    }

    if (decryptability == "key_missing") {
      return "{\"status\":\"fail\",\"op\":\"auth\",\"reason\":\"key_missing\""
        + camera_fields
        + template_fields + enrollment_fields + key_fields
        + ",\"auth_attempts_failed\":" + std::to_string(auth_state->failed_count())
        + ",\"auth_attempts_remaining\":" + std::to_string(attempts_remaining)
        + "}\n";
    }

    const std::string decrypt_status = template_decrypt_status_value();

    if (decrypt_status != "ok") {
      return "{\"status\":\"fail\",\"op\":\"auth\",\"reason\":\"template_decrypt_failed\""
        + camera_fields
        + template_fields + enrollment_fields + key_fields
        + template_decrypt_json_fields()
        + ",\"auth_attempts_failed\":" + std::to_string(auth_state->failed_count())
        + ",\"auth_attempts_remaining\":" + std::to_string(attempts_remaining)
        + "}\n";
    }

    return "{\"status\":\"fail\",\"op\":\"auth\",\"reason\":\"matcher_not_implemented\""
      + camera_fields
      + template_fields + enrollment_fields + key_fields
      + template_decrypt_json_fields()
      + ",\"auth_attempts_failed\":" + std::to_string(auth_state->failed_count())
      + ",\"auth_attempts_remaining\":" + std::to_string(attempts_remaining)
      + "}\n";
  }

  return "{\"status\":\"fail\",\"op\":\"unknown\",\"reason\":\"unknown_operation\""
    + camera_fields + template_fields + enrollment_fields + key_fields + "}\n";
}

void handle_client(int client_fd, FrameStore* frame_store, AuthState* auth_state) {
  ucred credentials {};

  if (!get_peer_credentials(client_fd, credentials)) {
    write_json_response(
      client_fd,
      "{\"status\":\"fail\",\"reason\":\"peer_credentials_unavailable\"}\n"
    );
    ::close(client_fd);
    return;
  }

  std::cout << "peer_credentials:"
            << " pid=" << credentials.pid
            << " uid=" << credentials.uid
            << " gid=" << credentials.gid
            << '\n';

  char buffer[1024] {};
  std::string request;
  const ssize_t bytes_read = ::read(client_fd, buffer, sizeof(buffer) - 1);

  if (bytes_read < 0) {
    std::cerr << "read_warning: " << std::strerror(errno) << '\n';
  } else {
    buffer[bytes_read] = '\0';
    request = buffer;
    std::cout << "client_request: " << request << '\n';
  }

  const std::string operation = extract_operation(request);
  const std::string policy = peer_policy_decision(credentials, operation);

  std::cout << "client_operation: " << operation << '\n';
  std::cout << "peer_policy: " << policy << '\n';

  if (!peer_is_allowed_for_operation(credentials, operation)) {
    std::cout << "peer_status: rejected" << '\n';
    write_json_response(
      client_fd,
      "{\"status\":\"fail\",\"reason\":\"peer_not_allowed\"}\n"
    );
    ::close(client_fd);
    return;
  }

  std::cout << "peer_status: allowed" << '\n';

  write_json_response(client_fd, build_response_for_request(request, frame_store, auth_state));

  ::close(client_fd);
}

bool run_socket_server(FrameStore* frame_store, AuthState* auth_state) {
  const std::string socket_path = get_socket_path();

  std::cout << "socket_path: " << socket_path << '\n';

  const int server_fd = create_server_socket(socket_path);

  if (server_fd < 0) {
    std::cout << "socket_status: error\n";
    return false;
  }

  std::cout << "socket_status: listening\n";
  std::cout << "socket_mode: 0600\n";
  std::cout << "server_status: started\n";
  std::cout << "stop_hint: press Ctrl+C to stop\n";

  while (g_running) {
    pollfd pfd {};
    pfd.fd = server_fd;
    pfd.events = POLLIN;

    const int poll_result = ::poll(&pfd, 1, 250);

    if (poll_result < 0) {
      if (errno == EINTR) {
        continue;
      }

      std::cerr << "poll_warning: " << std::strerror(errno) << '\n';
      continue;
    }

    if (poll_result == 0) {
      continue;
    }

    if ((pfd.revents & POLLIN) == 0) {
      continue;
    }

    const int client_fd = ::accept4(server_fd, nullptr, nullptr, SOCK_CLOEXEC);

    if (client_fd < 0) {
      if (errno == EINTR) {
        continue;
      }

      std::cerr << "accept_warning: " << std::strerror(errno) << '\n';
      continue;
    }

    handle_client(client_fd, frame_store, auth_state);
  }

  std::cout << "server_status: stopping\n";

  ::close(server_fd);
  ::unlink(socket_path.c_str());

  return true;
}

void camera_worker(FrameStore& frame_store, int camera_index) {
  cv::VideoCapture camera;

  if (!open_camera(camera, camera_index)) {
    g_running = false;
    return;
  }

  std::cout << "camera_worker_status: started" << '\n';

  using clock = std::chrono::steady_clock;
  auto last_report = clock::now();
  unsigned long long frames_since_report = 0;

  while (g_running) {
    cv::Mat frame;

    if (!camera.read(frame) || frame.empty()) {
      std::cout << "camera_worker_warning: empty_frame" << '\n';
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      continue;
    }

    frame_store.update(frame);
    ++frames_since_report;

    const auto now = clock::now();
    const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - last_report)
        .count();

    if (elapsed_ms >= 1000) {
      unsigned long long frames_total = 0;
      cv::Mat snapshot;
      frame_store.snapshot(snapshot, frames_total);

      const double fps = 1000.0 * static_cast<double>(frames_since_report) /
                         static_cast<double>(elapsed_ms);

      std::cout << "camera_worker_report:"
                << " frames_total=" << frames_total
                << " fps=" << fps
                << " width=" << frame.cols
                << " height=" << frame.rows
                << " channels=" << frame.channels()
                << '\n';

      frames_since_report = 0;
      last_report = now;
    }
  }

  camera.release();
  std::cout << "camera_worker_status: stopped" << '\n';
}

bool run_model_test(const std::string& model_path) {
#ifndef FACE_UNLOCK_WITH_TORCH
  (void)model_path;
  std::cout << "torch_status: disabled\n";
  std::cout << "status: torch_not_enabled\n";
  return false;
#else
  try {
    std::cout << "torch_status: enabled\n";
    std::cout << "model_path: " << model_path << "\n";

    torch::jit::script::Module module = torch::jit::load(model_path);
    module.eval();

    torch::NoGradGuard no_grad;

    torch::Tensor input = torch::zeros({1, 3, 112, 112});
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input);

    torch::jit::IValue output_value = module.forward(inputs);

    if (!output_value.isTensor()) {
      std::cout << "model_output_status: not_tensor\n";
      return false;
    }

    torch::Tensor output = output_value.toTensor();

    std::cout << "model_load_status: ok\n";
    std::cout << "model_forward_status: ok\n";
    std::cout << "model_output_sizes: " << output.sizes() << "\n";
    std::cout << "status: ok\n";

    return true;
  } catch (const std::exception& e) {
    std::cout << "model_load_status: failed\n";
    std::cout << "error: " << e.what() << "\n";
    std::cout << "status: model_error\n";
    return false;
  }
#endif
}

bool run_daemon_mode(int camera_index, int max_auth_attempts) {
  std::cout << "daemon_status: starting" << '\n';

  FrameStore frame_store;
  AuthState auth_state(max_auth_attempts);

  std::thread camera_thread([&frame_store, camera_index]() {
    camera_worker(frame_store, camera_index);
  });

  const bool server_ok = run_socket_server(&frame_store, &auth_state);

  g_running = false;

  if (camera_thread.joinable()) {
    camera_thread.join();
  }

  std::cout << "daemon_status: stopped" << '\n';

  return server_ok;
}

}  // namespace

int main(int argc, char** argv) {
  std::signal(SIGINT, handle_signal);
  std::signal(SIGTERM, handle_signal);

  const Options options = parse_options(argc, argv);
  const AppConfig config = load_app_config();
  const int camera_index =
    options.camera_index_set ? options.camera_index : config.camera_index;

  const uid_t uid = getuid();
  const std::string runtime_dir = get_runtime_dir();
  const std::string socket_path = get_socket_path();

  std::cout << "face-unlockd prototype" << '\n';
  std::cout << "version: 0.1.0" << '\n';
  std::cout << "uid: " << uid << '\n';
  std::cout << "runtime_dir: " << runtime_dir << '\n';
  std::cout << "planned_socket: " << socket_path << '\n';
  std::cout << "config_path: " << (config.path.empty() ? "none" : config.path) << '\n';
  std::cout << "config_loaded: " << (config.loaded ? "true" : "false") << '\n';
  std::cout << "effective_camera_index: " << camera_index << '\n';
  std::cout << "max_auth_attempts: " << config.max_auth_attempts << '\n';
  std::cout << "dev_auth_enabled: " << (dev_auth_enabled() ? "true" : "false") << '\n';
  std::cout << "root_auth_peer_enabled: " << (root_auth_peer_enabled() ? "true" : "false") << '\n';

  if (options.model_test) {
    std::cout << "mode: model_test" << '\n';

    const bool ok = run_model_test(options.model_path);

    return ok ? 0 : 5;
  }

  if (options.serve) {
    std::cout << "mode: serve" << '\n';

    AuthState auth_state(config.max_auth_attempts);
    const bool ok = run_socket_server(nullptr, &auth_state);

    if (!ok) {
      std::cout << "status: socket_error" << '\n';
      return 3;
    }

    std::cout << "status: ok" << '\n';
    return 0;
  }

  if (options.daemon) {
    std::cout << "mode: daemon" << '\n';

    const bool ok = run_daemon_mode(camera_index, config.max_auth_attempts);

    if (!ok) {
      std::cout << "status: daemon_error" << '\n';
      return 4;
    }

    std::cout << "status: ok" << '\n';
    return 0;
  }

  std::cout << "mode: " << (options.loop ? "loop" : "one_shot") << '\n';

  cv::VideoCapture camera;

  if (!open_camera(camera, camera_index)) {
    std::cout << "status: camera_error" << '\n';
    return 2;
  }

  const bool ok = options.loop ? run_loop(camera) : run_one_shot(camera);

  camera.release();

  if (!ok) {
    std::cout << "status: camera_error" << '\n';
    return 2;
  }

  std::cout << "status: ok" << '\n';
  return 0;
}
