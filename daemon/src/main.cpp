#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {

std::string get_runtime_dir() {
  const char* xdg_runtime_dir = std::getenv("XDG_RUNTIME_DIR");

  if (xdg_runtime_dir != nullptr && std::string(xdg_runtime_dir).size() > 0) {
    return std::string(xdg_runtime_dir);
  }

  return "/run/user/" + std::to_string(getuid());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  const uid_t uid = getuid();
  const std::string runtime_dir = get_runtime_dir();
  const std::string socket_path = runtime_dir + "/face-unlock.sock";

  std::cout << "face-unlockd prototype" << '\n';
  std::cout << "version: 0.1.0" << '\n';
  std::cout << "uid: " << uid << '\n';
  std::cout << "runtime_dir: " << runtime_dir << '\n';
  std::cout << "planned_socket: " << socket_path << '\n';
  std::cout << "status: ok" << '\n';

  return 0;
}
