find_path(PAM_INCLUDE_DIR security/pam_modules.h)
find_library(PAM_LIBRARY pam)

if(NOT PAM_INCLUDE_DIR OR NOT PAM_LIBRARY)
  message(FATAL_ERROR "PAM development files not found. Install libpam0g-dev.")
endif()

add_library(pam_face_unlock MODULE
  pam_face_unlock.c
)

target_compile_options(pam_face_unlock PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_include_directories(pam_face_unlock PRIVATE
  ${PAM_INCLUDE_DIR}
)

target_link_libraries(pam_face_unlock PRIVATE
  ${PAM_LIBRARY}
)

set_target_properties(pam_face_unlock PROPERTIES
  PREFIX ""
  OUTPUT_NAME "pam_face_unlock"
)

install(TARGETS pam_face_unlock
  LIBRARY DESTINATION lib/security
)
