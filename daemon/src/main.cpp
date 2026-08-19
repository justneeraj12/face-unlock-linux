#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

namespace {

std::string get_runtime_dir() {
  const char* xdg_runtime_dir = std::getenv("XDG_RUNTIME_DIR");

  if (xdg_runtime_dir != nullptr && std::string(xdg_runtime_dir).size() > 0) {
    return std::string(xdg_runtime_dir);
  }

  return "/run/user/" + std::to_string(getuid());
}

int parse_camera_index(int argc, char** argv) {
  int camera_index = 0;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];

    if ((arg == "--camera" || arg == "-c") && i + 1 < argc) {
      camera_index = std::atoi(argv[i + 1]);
      ++i;
    }
  }

  return camera_index;
}

bool read_one_camera_frame(int camera_index) {
  std::cout << "camera_index: " << camera_index << '\n';

  cv::VideoCapture camera;

  if (!camera.open(camera_index, cv::CAP_V4L2)) {
    std::cout << "camera_status: open_failed" << '\n';
    return false;
  }

  std::cout << "camera_status: opened" << '\n';

  camera.set(cv::CAP_PROP_FRAME_WIDTH, 640);
  camera.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

  cv::Mat frame;

  // Some webcams need a few frames before delivering a valid image.
  for (int attempt = 1; attempt <= 30; ++attempt) {
    if (camera.read(frame) && !frame.empty()) {
      std::cout << "frame_status: ok" << '\n';
      std::cout << "frame_width: " << frame.cols << '\n';
      std::cout << "frame_height: " << frame.rows << '\n';
      std::cout << "frame_channels: " << frame.channels() << '\n';
      return true;
    }
  }

  std::cout << "frame_status: read_failed" << '\n';
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  const uid_t uid = getuid();
  const std::string runtime_dir = get_runtime_dir();
  const std::string socket_path = runtime_dir + "/face-unlock.sock";
  const int camera_index = parse_camera_index(argc, argv);

  std::cout << "face-unlockd prototype" << '\n';
  std::cout << "version: 0.1.0" << '\n';
  std::cout << "uid: " << uid << '\n';
  std::cout << "runtime_dir: " << runtime_dir << '\n';
  std::cout << "planned_socket: " << socket_path << '\n';

  const bool camera_ok = read_one_camera_frame(camera_index);

  if (!camera_ok) {
    std::cout << "status: camera_error" << '\n';
    return 2;
  }

  std::cout << "status: ok" << '\n';
  return 0;
}
