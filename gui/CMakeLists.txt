find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable(face-unlock-enroll
  src/main.cpp
)

target_compile_options(face-unlock-enroll PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_link_libraries(face-unlock-enroll PRIVATE
  Qt6::Widgets
)

install(TARGETS face-unlock-enroll
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
