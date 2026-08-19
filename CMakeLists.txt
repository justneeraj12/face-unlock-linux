cmake_minimum_required(VERSION 3.22)

project(face-unlock-linux
  VERSION 0.1.0
  DESCRIPTION "Experimental face unlock utility for Ubuntu Linux"
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CTest)

option(WITH_TORCH "Build daemon with optional LibTorch/TorchScript support" OFF)

add_subdirectory(daemon)
add_subdirectory(pam)

install(FILES
  README.md
  SECURITY.md
  CONTRIBUTING.md
  ROADMAP.md
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

install(DIRECTORY
  docs/
  DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs
  FILES_MATCHING PATTERN "*.md"
)

install(DIRECTORY
  scripts/
  DESTINATION ${CMAKE_INSTALL_DATADIR}/face-unlock-linux/scripts
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  FILES_MATCHING PATTERN "*.sh"
)

install(DIRECTORY
  packaging/systemd/
  DESTINATION ${CMAKE_INSTALL_DATADIR}/face-unlock-linux/systemd
  FILES_MATCHING PATTERN "*.service"
)

set(CPACK_PACKAGE_NAME "face-unlock-linux")
set(CPACK_PACKAGE_VENDOR "face-unlock-linux contributors")
set(CPACK_PACKAGE_CONTACT "https://github.com/justneeraj12/face-unlock-linux")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Experimental face unlock daemon and PAM bridge for Ubuntu Linux")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/justneeraj12/face-unlock-linux")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "face-unlock-linux contributors")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_DEBIAN_PACKAGE_DEPENDS
  "libc6, libpam0g, libsodium23, libopencv-core406t64 | libopencv-core406, libopencv-videoio406t64 | libopencv-videoio406"
)

set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
"Experimental face unlock daemon and minimal PAM IPC bridge.

This package installs binaries and documentation only.

It does not automatically modify PAM configuration, sudo, login managers,
lock-screen configuration, GDM, SDDM, LightDM, or common-auth."
)

include(CPack)
