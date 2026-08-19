add_executable(face-unlockd
  src/main.cpp
)

target_compile_options(face-unlockd PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

install(TARGETS face-unlockd
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
