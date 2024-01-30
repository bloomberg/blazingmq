# bmqstoragetool
# --------------

if(NOT BMQ_TARGET_BMQSTORAGETOOL_NEEDED)
  return()
endif()

add_executable(bmqstoragetool)

target_compile_definitions(bmqstoragetool PRIVATE "MWC_INTERNAL_USAGE")

target_bmq_default_compiler_flags(bmqstoragetool)

set_target_properties(bmqstoragetool
  PROPERTIES OUTPUT_NAME "bmqstoragetool.tsk")
bbs_setup_target_uor(bmqstoragetool SKIP_TESTS)

include(BMQTest)
bmq_add_application_test(bmqstoragetool COMPAT)

install(TARGETS bmqstoragetool RUNTIME DESTINATION bin)
