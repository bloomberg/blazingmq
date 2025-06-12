# bmqtool
# -------

if(NOT BMQ_TARGET_BMQTOOL_NEEDED)
  return()
endif()

add_executable(bmqtool)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|(Apple)?Clang")
  # NOTE: The followings are using the 'deprecated' COMPILE_FLAGS property and
  # not COMPILE_OPTIONS because the later doesn't seem to work for a
  # 'SOURCE' target.
  set_property(SOURCE "m_bmqtool_application.cpp"
    APPEND
    PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral")
endif()

# Turn off warnings in generated code.
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?Clang")
    set_property(SOURCE "m_bmqtool_messages.cpp"
                 APPEND
                 PROPERTY COMPILE_OPTIONS "-Wno-everything"
    )
endif()

target_bmq_default_compiler_flags(bmqtool)

set_target_properties(bmqtool
  PROPERTIES OUTPUT_NAME "bmqtool.tsk")
bbs_setup_target_uor(bmqtool SKIP_TESTS)
bbs_add_target_bde_flags(bmqtool PUBLIC)
bbs_add_target_thread_flags(bmqtool PUBLIC)

include(BMQTest)
bmq_add_application_test(bmqtool COMPAT)

install(TARGETS bmqtool RUNTIME DESTINATION bin)
