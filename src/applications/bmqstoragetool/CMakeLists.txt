# bmqstoragetool
# --------------

if(NOT BMQ_TARGET_BMQSTORAGETOOL_NEEDED)
  return()
endif()

add_executable(bmqstoragetool)

target_compile_definitions(bmqstoragetool PRIVATE "MWC_INTERNAL_USAGE")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|(Apple)?Clang")
  # NOTE: The followings are using the 'deprecated' COMPILE_FLAGS property and
  # not COMPILE_OPTIONS because the later doesn't seem to work for a
  # 'SOURCE' target.
  set_property(SOURCE "m_bmqtool_application.cpp"
    APPEND
    PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral")
endif()

target_bmq_default_compiler_flags(bmqstoragetool)

set_target_properties(bmqstoragetool
  PROPERTIES OUTPUT_NAME "bmqstoragetool.tsk")
bbs_setup_target_uor(bmqstoragetool)

install(TARGETS bmqstoragetool RUNTIME DESTINATION bin)
