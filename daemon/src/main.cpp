#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

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
};

std::string get_runtime_dir() {
  const char* xdg_runtime_dir = std::getenv("XDG_RUNTIME_DIR");

  if (xdg_runtime_dir != nullptr && std::string(xdg_runtime_dir).size() > 0) {
    return std::string(xdg_runtime_dir);
  }

  return "/run/user/" + std::to_string(getuid());
}

void print_usage(const char* program_name) {
  std::cout << "Usage: " << program_name << " [--camera INDEX] [--loop]\n";
  std::cout << "\n";
  std::cout << "Options:\n";
  std::cout << "  --camera, -c INDEX   Camera index to open. Default: 0\n";
  std::cout << "  --loop               Keep reading frames until Ctrl+C\n";
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
    } else if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      std::exit(0);
    } else {
      std::cerr << "unknown_argument: " << arg << '\n';
      print_usage(argv[0]);
      std::exit(1);
    }
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

}  // namespace

int main(int argc, char** argv) {
  std::signal(SIGINT, handle_signal);
  std::signal(SIGTERM, handle_signal);

  const Options options = parse_options(argc, argv);

  const uid_t uid = getuid();
  const std::string runtime_dir = get_runtime_dir();
  const std::string socket_path = runtime_dir + "/face-unlock.sock";

  std::cout << "face-unlockd prototype" << '\n';
  std::cout << "version: 0.1.0" << '\n';
  std::cout << "uid: " << uid << '\n';
  std::cout << "runtime_dir: " << runtime_dir << '\n';
  std::cout << "planned_socket: " << socket_path << '\n';
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
