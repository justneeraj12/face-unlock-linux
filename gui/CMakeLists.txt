find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

add_executable(face-unlock-enroll
  src/daemon_client.cpp
  src/main.cpp
)

target_compile_options(face-unlock-enroll PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_link_libraries(face-unlock-enroll PRIVATE
  Qt6::Widgets
  Qt6::Network
)

if(BUILD_TESTING)
  add_test(
    NAME gui_enrollment_json
    COMMAND $<TARGET_FILE:face-unlock-enroll>
      --self-test-enrollment-json
  )
endif()

install(TARGETS face-unlock-enroll
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
