find_package(OpenCV REQUIRED COMPONENTS core videoio)

add_executable(face-unlockd
  src/main.cpp
)

target_compile_options(face-unlockd PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_include_directories(face-unlockd PRIVATE
  ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(face-unlockd PRIVATE
  ${OpenCV_LIBS}
)

install(TARGETS face-unlockd
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
