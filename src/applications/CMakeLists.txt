# applications
# ------------

# Enable safe asserts for applications in Debug/RelWithDebInfo builds
if (NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_definitions(BSLS_ASSERT_LEVEL_ASSERT_SAFE)
endif()

add_subdirectory( bmqbrkr )
add_subdirectory( bmqstoragetool )
add_subdirectory( bmqtool )
