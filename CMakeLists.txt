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
