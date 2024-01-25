# This module provides functions to support generating test targets compatible
# with BlazingMQ CI.
#
# add_bmq_test( TARGET )

include_guard()

# :: _bmq_target_generate_td_manifest :::::::::::::::::::::::::::::::::::::::::::::::
# This function generates a test driver manifest for compatibility with the rat.rb
# testing tool.
#
# TEST DRIVER MANIFEST:
# A '<grpName>_td.manifest' file is generated at the root of the build
# directory, containing one line per test driver, in the following format:
# <grppkg_cmpName>: /full/path/to/component.t.tsk
# This manifest can be used by test driver executor tools, such as rat.rb.
#
function(_bmq_target_generate_td_manifest target)
  cmake_parse_arguments(""
    ""
    ""
    "TEST_DRIVERS"
    ${ARGN})

  # Generate the test driver manifest for that group library
  message(DEBUG "Manifesting ${_TEST_DRIVERS}")
  string(REGEX REPLACE ";" "\n" td_manifest "${_TEST_DRIVERS}")
  file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/${target}_td.manifest"
    CONTENT "${td_manifest}\n")
endfunction()

# :: bmq_add_test :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# This function searches for the test drivers of a UOR-style TARGET within the
# `tests` directory of each package. For each component, it generates a target
# named ${UOR_component}.t and ${UOR_component}.td for each component found,
# and an all.td target which depends on all the tests together.
#
# The *.td targets should not be depended upon directly mostly exist for
# historical compatibility.
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
    set(td_manifest)

    foreach(pkg ${${uor_name}_PACKAGES})
      if(${pkg}_TEST_TARGETS)
        if(NOT TARGET ${target}.t)
          add_custom_target(${target}.t)

          if(_COMPAT)
            add_custom_target(${target}.td)
          endif()
        endif()

        add_dependencies(${target}.t ${${pkg}_TEST_TARGETS})

        if(_COMPAT)
          add_dependencies(${target}.td ${target}.t)

          foreach(test_target ${${pkg}_TEST_TARGETS})
            string(REPLACE ".t" "" component ${test_target})
            set_target_properties(${test_target} PROPERTIES OUTPUT_NAME "${test_target}.tsk")
            list(APPEND td_manifest
              "${component}: $<TARGET_FILE:${test_target}>")
          endforeach()
        endif()

        if(import_test_deps)
          # Import UOR test dependencies only once and only if we have at least
          # one generated test target
          bbs_import_target_dependencies(${target} ${${uor_name}_TEST_PCDEPS})
          set(import_test_deps OFF)
        endif()
      endif()
    endforeach()

    # Generate the test driver manifest file for compatibility with rat.rb
    if(_COMPAT)
      _bmq_target_generate_td_manifest(${target} TEST_DRIVERS ${td_manifest})
    endif()
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

    # Generate the test driver manifest file for compatibility with rat.rb
    if(_COMPAT)
      _bmq_target_generate_td_manifest(${target} TEST_DRIVERS ${td_manifest})
    endif()

  endif()

  if(_COMPAT AND NOT TARGET all.td)
    add_custom_target(all.td)
    add_dependencies(all.td all.t)
  endif()
endfunction()

# :: bmq_add_application_test :::::::::::::::::::::::::::::::::::::::::::::::::
# This function searches for the test drivers of an 'application' TARGET. 
# It expects existence of intermediate library '${uor_name}_lib' which is 
# created by 'bbs_setup_target_uor()'.
# It generates a target named ${UOR_component}.t and ${UOR_component}.td for
# each component found, and an all.td target which depends on all the tests
# together.
#
# The *.td targets should not be depended upon directly mostly exist for
# historical compatibility.
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

  set(td_manifest)

  if(${lib_target}_TEST_TARGETS)

    if(NOT TARGET ${lib_target}.t)
      add_custom_target(${lib_target}.t)      
      if(_COMPAT)
        add_custom_target(${lib_target}.td)
      endif()
    endif()

    add_dependencies(${lib_target}.t ${${lib_target}_TEST_TARGETS})

    if(_COMPAT)
      foreach(test_target ${${lib_target}_TEST_TARGETS})
        string(REPLACE ".t" "" component ${test_target})
        set_target_properties(${test_target} PROPERTIES OUTPUT_NAME "${test_target}.tsk")
        list(APPEND td_manifest
          "${component}: $<TARGET_FILE:${test_target}>")
      endforeach()
    endif()

    bbs_import_target_dependencies(${lib_target} ${${uor_name}_TEST_PCDEPS})
  endif()

  # Generate the test driver manifest file for compatibility with rat.rb
  if(_COMPAT)
    _bmq_target_generate_td_manifest(${lib_target} TEST_DRIVERS ${td_manifest})
  endif()

  if(_COMPAT AND NOT TARGET all.td)
    add_custom_target(all.td)
    add_dependencies(all.td all.t)
  endif()
endfunction()
