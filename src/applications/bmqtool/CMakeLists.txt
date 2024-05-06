# bmqtool
# -------

if(NOT BMQ_TARGET_BMQTOOL_NEEDED)
  return()
endif()

add_executable(bmqtool)

target_compile_definitions(bmqtool PRIVATE "MWC_INTERNAL_USAGE")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|(Apple)?Clang")
  # NOTE: The followings are using the 'deprecated' COMPILE_FLAGS property and
  # not COMPILE_OPTIONS because the later doesn't seem to work for a
  # 'SOURCE' target.
  set_property(SOURCE "m_bmqtool_application.cpp"
    APPEND
    PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral")
endif()

target_bmq_default_compiler_flags(bmqtool)

set_target_properties(bmqtool
  PROPERTIES OUTPUT_NAME "bmqtool.tsk")
bbs_setup_target_uor(bmqtool)
bbs_add_target_bde_flags(bmqtool PUBLIC)
bbs_add_target_thread_flags(bmqtool PUBLIC)

install(TARGETS bmqtool RUNTIME DESTINATION bin)
