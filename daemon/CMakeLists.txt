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

find_library(OPENCV_IMGPROC_LIBRARY
  NAMES opencv_imgproc
)

find_library(OPENCV_OBJDETECT_LIBRARY
  NAMES opencv_objdetect
)

if(NOT OPENCV_INCLUDE_DIR OR NOT OPENCV_CORE_LIBRARY OR NOT OPENCV_VIDEOIO_LIBRARY)
  message(FATAL_ERROR "OpenCV core/videoio development files not found. Install libopencv-core-dev and libopencv-videoio-dev.")
endif()

if(NOT OPENCV_IMGPROC_LIBRARY OR NOT OPENCV_OBJDETECT_LIBRARY)
  message(WARNING "OpenCV imgproc/objdetect not found. Haar detector backend will not be available.")
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

add_library(face_unlock_detector STATIC
  src/detector.cpp
)

target_compile_options(face_unlock_detector PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_include_directories(face_unlock_detector PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${OPENCV_INCLUDE_DIR}
)

target_link_libraries(face_unlock_detector PUBLIC
  ${OPENCV_CORE_LIBRARY}
)

if(OPENCV_IMGPROC_LIBRARY AND OPENCV_OBJDETECT_LIBRARY)
  target_compile_definitions(face_unlock_detector PUBLIC FACE_UNLOCK_HAVE_OPENCV_OBJDETECT=1)
  target_link_libraries(face_unlock_detector PUBLIC
    ${OPENCV_IMGPROC_LIBRARY}
    ${OPENCV_OBJDETECT_LIBRARY}
  )
endif()

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
  face_unlock_crypto
  face_unlock_detector
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

add_executable(face-unlock-template-tool
  src/template_tool.cpp
)

target_compile_options(face-unlock-template-tool PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_link_libraries(face-unlock-template-tool PRIVATE
  face_unlock_crypto
)

add_executable(face-unlock-key-tool
  src/key_tool.cpp
)

target_compile_options(face-unlock-key-tool PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_link_libraries(face-unlock-key-tool PRIVATE
  face_unlock_crypto
)


add_executable(face-unlock-detector-selftest
  src/detector_selftest.cpp
)

target_compile_options(face-unlock-detector-selftest PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_link_libraries(face-unlock-detector-selftest PRIVATE
  face_unlock_detector
)

if(BUILD_TESTING)
  add_test(
    NAME crypto_selftest
    COMMAND face-unlock-crypto-selftest
  )

  add_test(
    NAME detector_selftest
    COMMAND face-unlock-detector-selftest
  )


  add_test(
    NAME key_template_flow
    COMMAND bash
      ${CMAKE_SOURCE_DIR}/scripts/test-key-template-flow.sh
      $<TARGET_FILE:face-unlock-key-tool>
      $<TARGET_FILE:face-unlock-template-tool>
  )

  set_tests_properties(key_template_flow PROPERTIES
    TIMEOUT 30
  )

  add_test(
    NAME daemon_metadata
    COMMAND bash
      ${CMAKE_SOURCE_DIR}/scripts/test-daemon-metadata.sh
      $<TARGET_FILE:face-unlockd>
      $<TARGET_FILE:face-unlock-key-tool>
      $<TARGET_FILE:face-unlock-template-tool>
      ${CMAKE_SOURCE_DIR}/scripts/test-socket-client.sh
  )

  set_tests_properties(daemon_metadata PROPERTIES
    TIMEOUT 30
  )

  add_test(
    NAME auth_reasons
    COMMAND bash
      ${CMAKE_SOURCE_DIR}/scripts/test-auth-reasons.sh
      $<TARGET_FILE:face-unlockd>
      $<TARGET_FILE:face-unlock-key-tool>
      $<TARGET_FILE:face-unlock-template-tool>
      ${CMAKE_SOURCE_DIR}/scripts/test-socket-client.sh
  )

  set_tests_properties(auth_reasons PROPERTIES
    TIMEOUT 30
  )

  add_test(
    NAME detector_backends
    COMMAND bash
      ${CMAKE_SOURCE_DIR}/scripts/test-detector-backends.sh
      $<TARGET_FILE:face-unlockd>
      $<TARGET_FILE:face-unlock-detector-selftest>
      ${CMAKE_SOURCE_DIR}/scripts/test-socket-client.sh
  )

  set_tests_properties(detector_backends PROPERTIES
    TIMEOUT 30
  )
endif()

install(TARGETS face-unlockd face-unlock-crypto-selftest face-unlock-template-tool face-unlock-key-tool face-unlock-detector-selftest
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
