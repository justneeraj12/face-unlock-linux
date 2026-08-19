find_path(OPENCV_INCLUDE_DIR
  NAMES opencv2/core.hpp
  PATH_SUFFIXES opencv4
)

find_library(OPENCV_CORE_LIBRARY
  NAMES opencv_core
)

find_library(OPENCV_VIDEOIO_LIBRARY
  NAMES opencv_videoio
)

if(NOT OPENCV_INCLUDE_DIR OR NOT OPENCV_CORE_LIBRARY OR NOT OPENCV_VIDEOIO_LIBRARY)
  message(FATAL_ERROR "OpenCV core/videoio development files not found. Install libopencv-core-dev and libopencv-videoio-dev.")
endif()

find_path(SODIUM_INCLUDE_DIR sodium.h)
find_library(SODIUM_LIBRARY sodium)

if(NOT SODIUM_INCLUDE_DIR OR NOT SODIUM_LIBRARY)
  message(FATAL_ERROR "libsodium development files not found. Install libsodium-dev.")
endif()

add_library(face_unlock_crypto STATIC
  src/template_crypto.cpp
)

target_compile_options(face_unlock_crypto PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_include_directories(face_unlock_crypto PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SODIUM_INCLUDE_DIR}
)

target_link_libraries(face_unlock_crypto PUBLIC
  ${SODIUM_LIBRARY}
)

add_executable(face-unlockd
  src/main.cpp
)

target_compile_options(face-unlockd PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_compile_definitions(face-unlockd PRIVATE _GNU_SOURCE)

target_include_directories(face-unlockd PRIVATE
  ${OPENCV_INCLUDE_DIR}
)

target_link_libraries(face-unlockd PRIVATE
  ${OPENCV_CORE_LIBRARY}
  ${OPENCV_VIDEOIO_LIBRARY}
)

if(WITH_TORCH)
  find_package(Torch REQUIRED)

  target_compile_definitions(face-unlockd PRIVATE FACE_UNLOCK_WITH_TORCH=1)

  target_link_libraries(face-unlockd PRIVATE
    ${TORCH_LIBRARIES}
  )

  set_property(TARGET face-unlockd PROPERTY CXX_STANDARD 17)
endif()

add_executable(face-unlock-crypto-selftest
  src/crypto_selftest.cpp
)

target_compile_options(face-unlock-crypto-selftest PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_link_libraries(face-unlock-crypto-selftest PRIVATE
  face_unlock_crypto
)

if(BUILD_TESTING)
  add_test(
    NAME crypto_selftest
    COMMAND face-unlock-crypto-selftest
  )
endif()

install(TARGETS face-unlockd face-unlock-crypto-selftest
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
