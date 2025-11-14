# This module provides functions to support generating test targets compatible
# with BlazingMQ CI.
#
# bmq_add_application_test( TARGET )

include_guard()

# :: bmq_add_application_test :::::::::::::::::::::::::::::::::::::::::::::::::
# This function searches for the test drivers of an 'application' TARGET.  It
# expects existence of intermediate library '${uor_name}_lib' which is created
# by 'bbs_setup_target_uor()'.  It generates a target named ${UOR_component}.t.
function(bmq_add_application_test target)
  cmake_parse_arguments(PARSE_ARGV 1
    ""
    "COMPAT"
    "SOURCE_DIR"
    "")

  find_package(BdeBuildSystem REQUIRED)

  # Get the name of the unit from the target
  get_target_property(uor_name ${target} NAME)

  # Use the current source directory if none is specified
  if(NOT _SOURCE_DIR)
    set(_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  # Check that BDE metadata exists and load it
  if(NOT DEFINED ${uor_name}_PACKAGES)
    if(EXISTS ${_SOURCE_DIR}/package)
      bbs_read_metadata(PACKAGE ${uor_name}
        SOURCE_DIR ${_SOURCE_DIR})
    endif()
  endif()

  # Refer to intermediate library created by 'bbs_setup_target_uor()'
  set(lib_target "${uor_name}_lib")

  bbs_configure_target_tests(${lib_target}
    SOURCES    ${${uor_name}_TEST_SOURCES}
    TEST_DEPS  ${${uor_name}_PCDEPS}
               ${${uor_name}_TEST_PCDEPS}
    LABELS     "unit;all" ${target})

  if (TARGET ${lib_target}.t)
    if (NOT TARGET ${target}.t)
      add_custom_target(${target}.t)
    endif()
    add_dependencies(${target}.t ${lib_target}.t)
  endif()

  if (${lib_target}_TEST_TARGETS)
    bbs_import_target_dependencies(${lib_target} ${${uor_name}_TEST_PCDEPS})
  endif()

  if(${lib_target}_TEST_TARGETS)
    bbs_import_target_dependencies(${lib_target} ${${uor_name}_TEST_PCDEPS})
  endif()
endfunction()
