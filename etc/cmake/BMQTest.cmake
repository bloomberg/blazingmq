# This module provides functions to support generating test targets compatible
# with BlazingMQ CI.
#
# add_bmq_test( TARGET )

include_guard()

# :: bmq_add_test :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# This function searches for the test drivers of a UOR-style TARGET within the
# `tests` directory of each package. For each component, it generates a target
# named ${UOR_component}.t for each component found.
function(bmq_add_test target)
  cmake_parse_arguments(PARSE_ARGV 1
    ""
    "SKIP_TESTS;NO_GEN_BDE_METADATA;NO_EMIT_PKG_CONFIG_FILE;COMPAT"
    "SOURCE_DIR"
    "CUSTOM_PACKAGES")

  find_package(BdeBuildSystem REQUIRED)

  # Get the name of the unit from the target
  get_target_property(uor_name ${target} NAME)

  # Use the current source directory if none is specified
  if(NOT _SOURCE_DIR)
    set(_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  # Check that BDE metadata exists and load it
  if(NOT DEFINED ${uor_name}_PACKAGES)
    if(EXISTS ${_SOURCE_DIR}/group)
      bbs_read_metadata(GROUP ${uor_name}
        SOURCE_DIR ${_SOURCE_DIR}
        CUSTOM_PACKAGES "${_CUSTOM_PACKAGES}")
    else()
      if(EXISTS ${_SOURCE_DIR}/package)
        bbs_read_metadata(PACKAGE ${uor_name}
          SOURCE_DIR ${_SOURCE_DIR})
      endif()
    endif()
  endif()

  # Each package in the groups
  if(${uor_name}_PACKAGES)
    foreach(pkg ${${uor_name}_PACKAGES})
      bbs_configure_target_tests(${pkg}
        SOURCES ${${pkg}_TEST_SOURCES}
        TEST_DEPS ${${pkg}_DEPENDS}
        ${${pkg}_TEST_DEPENDS}
        ${${uor_name}_PCDEPS}
        ${${uor_name}_TEST_PCDEPS}
        LABELS "all" ${target} ${pkg})
    endforeach()

    set(import_test_deps ON)

    foreach(pkg ${${uor_name}_PACKAGES})
      if(${pkg}_TEST_TARGETS)
        if(NOT TARGET ${target}.t)
          add_custom_target(${target}.t)
        endif()

        add_dependencies(${target}.t ${${pkg}_TEST_TARGETS})

        if(import_test_deps)
          # Import UOR test dependencies only once and only if we have at least
          # one generated test target
          bbs_import_target_dependencies(${target} ${${uor_name}_TEST_PCDEPS})
          set(import_test_deps OFF)
        endif()
      endif()
    endforeach()
  else()
    # Configure standalone library ( no packages ) and tests from BDE metadata
    bbs_configure_target_tests(${target}
      SOURCES ${${uor_name}_TEST_SOURCES}
      TEST_DEPS ${${uor_name}_PCDEPS}
      ${${uor_name}_TEST_PCDEPS}
      LABELS "all" ${target})

    if(${target}_TEST_TARGETS)
      bbs_import_target_dependencies(${target} ${${uor_name}_TEST_PCDEPS})
    endif()
  endif()
endfunction()

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
    LABELS     "all" ${target})

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
