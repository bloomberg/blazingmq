# bmqstoragewriter
# ----------------

if("${CMAKE_SYSTEM_NAME}" MATCHES "SunOS" OR NOT BMQ_TARGET_BMQSTORAGEWRITER_NEEDED)
  return()
endif()

add_executable(bmqstoragewriter)

target_bmq_default_compiler_flags(bmqstoragewriter)

set_target_properties(bmqstoragewriter
  PROPERTIES OUTPUT_NAME "bmqstoragewriter.tsk")
bbs_setup_target_uor(bmqstoragewriter SKIP_TESTS)

include(BMQTest)
bmq_add_application_test(bmqstoragewriter COMPAT)

install(TARGETS bmqstoragewriter RUNTIME DESTINATION bin COMPONENT bmqstoragewriter)
