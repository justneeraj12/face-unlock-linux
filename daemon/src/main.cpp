#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <optional>
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

namespace {

std::atomic<bool> g_running{true};

void handle_signal(int signal_number) {
  (void)signal_number;
  g_running = false;
}

struct Options {
  int camera_index = 0;
  bool loop = false;
  bool serve = false;
  bool daemon = false;
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

void print_usage(const char* program_name) {
  std::cout << "Usage: " << program_name
            << " [--camera INDEX] [--loop] [--serve] [--daemon]\n";
  std::cout << "Options:\n";
  std::cout << "  --camera, -c INDEX   Camera index to open. Default: 0\n";
  std::cout << "  --loop               Keep reading frames until Ctrl+C\n";
  std::cout << "  --serve              Run local UNIX socket server until Ctrl+C\n";
  std::cout << "  --daemon             Run camera worker and socket server together\n";
  std::cout << "  --help, -h           Show this help text\n";
}

Options parse_options(int argc, char** argv) {
  Options options;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];

    if ((arg == "--camera" || arg == "-c") && i + 1 < argc) {
      options.camera_index = std::atoi(argv[i + 1]);
      ++i;
    } else if (arg == "--loop") {
      options.loop = true;
    } else if (arg == "--serve") {
      options.serve = true;
    } else if (arg == "--daemon") {
      options.daemon = true;
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
    static_cast<int>(options.daemon);

  if (mode_count > 1) {
    std::cerr << "error: choose only one of --loop, --serve, or --daemon\n";
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

bool peer_is_allowed(const ucred& credentials) {
  const uid_t daemon_uid = getuid();

  // Current prototype policy:
  // - allow same user only
  //
  // Later PAM testing may need a carefully reviewed policy for root-owned PAM clients.
  return credentials.uid == daemon_uid;
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

std::string build_response_for_request(const std::string& request, FrameStore* frame_store) {
  const std::string op = extract_operation(request);
  const CameraStatus camera_status = get_camera_status(frame_store);
  const std::string camera_fields = camera_status_json_fields(camera_status);

  if (op == "ping") {
    return "{\"status\":\"ok\",\"op\":\"ping\",\"reason\":\"daemon_alive\""
      + camera_fields + "}\n";
  }

  if (op == "camera_status") {
    return "{\"status\":\"ok\",\"op\":\"camera_status\""
      + camera_fields + "}\n";
  }

  if (op == "auth") {
    return "{\"status\":\"fail\",\"op\":\"auth\",\"reason\":\"auth_not_implemented\""
      + camera_fields + "}\n";
  }

  return "{\"status\":\"fail\",\"op\":\"unknown\",\"reason\":\"unknown_operation\""
    + camera_fields + "}\n";
}

void handle_client(int client_fd, FrameStore* frame_store) {
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

  if (!peer_is_allowed(credentials)) {
    std::cout << "peer_status: rejected" << '\n';
    write_json_response(
      client_fd,
      "{\"status\":\"fail\",\"reason\":\"peer_not_allowed\"}\n"
    );
    ::close(client_fd);
    return;
  }

  std::cout << "peer_status: allowed" << '\n';

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

  write_json_response(client_fd, build_response_for_request(request, frame_store));

  ::close(client_fd);
}

bool run_socket_server(FrameStore* frame_store) {
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

    handle_client(client_fd, frame_store);
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

bool run_daemon_mode(int camera_index) {
  std::cout << "daemon_status: starting" << '\n';

  FrameStore frame_store;

  std::thread camera_thread([&frame_store, camera_index]() {
    camera_worker(frame_store, camera_index);
  });

  const bool server_ok = run_socket_server(&frame_store);

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

  const uid_t uid = getuid();
  const std::string runtime_dir = get_runtime_dir();
  const std::string socket_path = get_socket_path();

  std::cout << "face-unlockd prototype" << '\n';
  std::cout << "version: 0.1.0" << '\n';
  std::cout << "uid: " << uid << '\n';
  std::cout << "runtime_dir: " << runtime_dir << '\n';
  std::cout << "planned_socket: " << socket_path << '\n';

  if (options.serve) {
    std::cout << "mode: serve" << '\n';

    const bool ok = run_socket_server(nullptr);

    if (!ok) {
      std::cout << "status: socket_error" << '\n';
      return 3;
    }

    std::cout << "status: ok" << '\n';
    return 0;
  }

  if (options.daemon) {
    std::cout << "mode: daemon" << '\n';

    const bool ok = run_daemon_mode(options.camera_index);

    if (!ok) {
      std::cout << "status: daemon_error" << '\n';
      return 4;
    }

    std::cout << "status: ok" << '\n';
    return 0;
  }

  std::cout << "mode: " << (options.loop ? "loop" : "one_shot") << '\n';

  cv::VideoCapture camera;

  if (!open_camera(camera, options.camera_index)) {
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
