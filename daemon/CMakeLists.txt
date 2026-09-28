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

find_library(OPENCV_DNN_LIBRARY
  NAMES opencv_dnn
)

if(NOT OPENCV_INCLUDE_DIR OR NOT OPENCV_CORE_LIBRARY OR NOT OPENCV_VIDEOIO_LIBRARY)
  message(FATAL_ERROR "OpenCV core/videoio development files not found. Install libopencv-core-dev and libopencv-videoio-dev.")
endif()

if(NOT OPENCV_IMGPROC_LIBRARY OR NOT OPENCV_OBJDETECT_LIBRARY)
  message(WARNING "OpenCV imgproc/objdetect not found. Haar detector backend will not be available.")
endif()

if(NOT OPENCV_DNN_LIBRARY)
  message(WARNING "OpenCV dnn not found. YuNet detector backend will not be available.")
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

add_library(face_unlock_lockscreen_auth STATIC
  src/lockscreen_auth.cpp
)

target_compile_options(face_unlock_lockscreen_auth PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_include_directories(face_unlock_lockscreen_auth PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(face-unlock-lockscreen-auth-selftest
  src/lockscreen_auth_selftest.cpp
)

target_compile_options(face-unlock-lockscreen-auth-selftest PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_link_libraries(face-unlock-lockscreen-auth-selftest PRIVATE
  face_unlock_lockscreen_auth
)

add_library(face_unlock_camera_lease STATIC
  src/camera_lease.cpp
)

target_compile_options(face_unlock_camera_lease PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_include_directories(face_unlock_camera_lease PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${OPENCV_INCLUDE_DIR}
)

target_link_libraries(face_unlock_camera_lease PUBLIC
  ${OPENCV_CORE_LIBRARY}
  ${OPENCV_VIDEOIO_LIBRARY}
)

add_executable(face-unlock-camera-lease-selftest
  src/camera_lease_selftest.cpp
)

target_compile_options(face-unlock-camera-lease-selftest PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_link_libraries(face-unlock-camera-lease-selftest PRIVATE
  face_unlock_camera_lease
)

if(OPENCV_IMGPROC_LIBRARY)
  add_library(face_unlock_frame_quality STATIC
    src/frame_quality.cpp
  )
  target_compile_options(face_unlock_frame_quality PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
  target_include_directories(face_unlock_frame_quality PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENCV_INCLUDE_DIR}
  )
  target_link_libraries(face_unlock_frame_quality PUBLIC
    face_unlock_detector
    ${OPENCV_IMGPROC_LIBRARY}
  )

  add_executable(face-unlock-frame-quality-selftest
    src/frame_quality_selftest.cpp
  )
  target_compile_options(face-unlock-frame-quality-selftest PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
  target_link_libraries(face-unlock-frame-quality-selftest PRIVATE
    face_unlock_frame_quality
  )
endif()

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

if(OPENCV_IMGPROC_LIBRARY AND OPENCV_OBJDETECT_LIBRARY AND OPENCV_DNN_LIBRARY)
  target_compile_definitions(face_unlock_detector PUBLIC FACE_UNLOCK_HAVE_OPENCV_YUNET=1)
  target_link_libraries(face_unlock_detector PUBLIC
    ${OPENCV_DNN_LIBRARY}
  )
endif()

if(OPENCV_IMGPROC_LIBRARY AND OPENCV_OBJDETECT_LIBRARY AND OPENCV_DNN_LIBRARY)
  add_library(face_unlock_recognizer STATIC src/recognizer.cpp)
  target_compile_options(face_unlock_recognizer PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
  target_include_directories(face_unlock_recognizer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENCV_INCLUDE_DIR}
  )
  target_link_libraries(face_unlock_recognizer PUBLIC
    face_unlock_detector
    ${OPENCV_IMGPROC_LIBRARY}
    ${OPENCV_OBJDETECT_LIBRARY}
    ${OPENCV_DNN_LIBRARY}
  )

  add_executable(face-unlock-recognizer-selftest src/recognizer_selftest.cpp)
  target_compile_options(face-unlock-recognizer-selftest PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
  target_link_libraries(face-unlock-recognizer-selftest PRIVATE
    face_unlock_recognizer
  )

  add_library(face_unlock_profile STATIC src/face_profile.cpp)
  target_compile_options(face_unlock_profile PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
  target_include_directories(face_unlock_profile PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  target_link_libraries(face_unlock_profile PUBLIC
    face_unlock_recognizer
  )

  add_executable(face-unlock-profile-selftest
    src/face_profile_selftest.cpp
  )
  target_compile_options(face-unlock-profile-selftest PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
  target_link_libraries(face-unlock-profile-selftest PRIVATE
    face_unlock_profile
    face_unlock_crypto
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

target_compile_definitions(face-unlockd PRIVATE
  _GNU_SOURCE
  FACE_UNLOCK_VERSION="${PROJECT_VERSION}"
)

target_include_directories(face-unlockd PRIVATE
  ${OPENCV_INCLUDE_DIR}
)

target_link_libraries(face-unlockd PRIVATE
  ${OPENCV_CORE_LIBRARY}
  ${OPENCV_VIDEOIO_LIBRARY}
  face_unlock_crypto
  face_unlock_detector
  face_unlock_lockscreen_auth
  face_unlock_camera_lease
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
    NAME lockscreen_auth_policy
    COMMAND face-unlock-lockscreen-auth-selftest
  )

  add_test(
    NAME camera_lease
    COMMAND face-unlock-camera-lease-selftest
  )

  if(TARGET face-unlock-frame-quality-selftest)
    add_test(
      NAME frame_quality
      COMMAND face-unlock-frame-quality-selftest
    )
  endif()

  add_test(
    NAME camera_lease_protocol
    COMMAND bash
      ${CMAKE_SOURCE_DIR}/scripts/test-camera-lease-protocol.sh
      $<TARGET_FILE:face-unlockd>
      ${CMAKE_SOURCE_DIR}/scripts/test-socket-client.sh
  )
  set_tests_properties(camera_lease_protocol PROPERTIES TIMEOUT 30)


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

  if(TARGET face-unlock-recognizer-selftest)
    add_test(NAME cpu_recognizer COMMAND bash
      ${CMAKE_SOURCE_DIR}/scripts/test-cpu-recognizer.sh
      $<TARGET_FILE:face-unlock-recognizer-selftest>
      ${CMAKE_SOURCE_DIR}/models/face_recognition_sface_2021dec.onnx)
    set_tests_properties(cpu_recognizer PROPERTIES TIMEOUT 30)
  endif()

  if(TARGET face-unlock-profile-selftest)
    add_test(
      NAME face_profile_cpp
      COMMAND $<TARGET_FILE:face-unlock-profile-selftest>
    )
    set_tests_properties(face_profile_cpp PROPERTIES TIMEOUT 30)
  endif()

  add_test(
    NAME cpu_face_profile
    COMMAND bash
      ${CMAKE_SOURCE_DIR}/scripts/test-cpu-face-profile.sh
  )

  set_tests_properties(cpu_face_profile PROPERTIES
    TIMEOUT 30
  )
endif()

install(
  TARGETS
    face-unlockd
    face-unlock-crypto-selftest
    face-unlock-template-tool
    face-unlock-key-tool
    face-unlock-detector-selftest
    face-unlock-lockscreen-auth-selftest
    face-unlock-camera-lease-selftest
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(TARGET face-unlock-recognizer-selftest)
  install(
    TARGETS
      face-unlock-recognizer-selftest
      face-unlock-profile-selftest
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()
