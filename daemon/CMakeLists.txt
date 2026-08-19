find_package(OpenCV REQUIRED COMPONENTS core videoio)

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
  ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(face-unlockd PRIVATE
  ${OpenCV_LIBS}
)

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

install(TARGETS face-unlockd face-unlock-crypto-selftest
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
