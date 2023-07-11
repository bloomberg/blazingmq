# This module provides a CMake function for configuring BlazingMQ targets based
# on the BDE UOR project layout. Some aspects of the BlazingMQ project layout
# are extensions to the BDE style, particularly in the way test targets are
# discovered and named, and this module's purpose is to support those
# extensions.
include_guard()

# :: target_bmq_default_compiler_flags :::::::::::::::::::::::::::::::::::::::::
# This function sets compiler and linker flag defaults for BlazingMQ targets.
#
# compile options
# GNU
# questionable option, only used in pekludge builds
# -D_SYS_SYSMACROS_H
# -pipe
function(target_bmq_default_compiler_flags TARGET)
  include(BBVendor)

  # ------------------------------------
  # GCC CXXFlags / LDFlags
  # NOTE: o no-strict-aliasing: -O2 and -Wall don't play nicely together
  # (See internal ticket D57524807)
  target_configure_compile_options(${TARGET}
    COMPILER_ID GNU
    CONFIGS Debug Release RelWithDebInfo
    OPTIONS -Wall
  )

  target_configure_compile_options(${TARGET}
    COMPILER_ID GNU
    CONFIGS Debug
    OPTIONS -fno-default-inline -fno-omit-frame-pointer
  )

  # ------------------------------------
  # CLANG CXXFlags / LDFlags
  target_configure_compile_options(${TARGET}
    COMPILER_ID Clang
    CONFIGS Debug Release RelWithDebInfo
    OPTIONS -Wall
  )
endfunction()

# :: target_bmq_style_uor :::::::::::::::::::::::::::::::::::::::::::::::::::::
# This function configures BlazingMQ UOR targets with all the default compiler
# flags for the standard build configs, adds test targets, and resolves
# dependencies.
function(target_bmq_style_uor TARGET)
  cmake_parse_arguments(""
    ""
    ""
    "PRIVATE_PACKAGES"
    ${ARGN})

  find_package(BdeBuildSystem REQUIRED)

  target_bmq_default_compiler_flags(${TARGET})

  add_library(${TARGET}-flags INTERFACE IMPORTED)
  target_compile_definitions(${TARGET}-flags INTERFACE "MWC_INTERNAL_USAGE")

  bbs_setup_target_uor(${TARGET}
    NO_EMIT_PKG_CONFIG_FILE
    SKIP_TESTS
    PRIVATE_PACKAGES "${_PRIVATE_PACKAGES}")

  get_target_property(uor_name ${TARGET} NAME)

  # Check that BDE metadata exists and load it
  set(_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

  if(EXISTS ${_SOURCE_DIR}/group)
    bbs_read_metadata(GROUP ${TARGET}
      SOURCE_DIR ${_SOURCE_DIR})
  else()
    if(EXISTS ${_SOURCE_DIR}/package)
      bbs_read_metadata(PACKAGE ${TARGET}
        SOURCE_DIR ${_SOURCE_DIR})
    endif()
  endif()

  # Set flags for each object library
  if(${uor_name}_PACKAGES)
    foreach(pkg ${${uor_name}_PACKAGES})
      target_bmq_default_compiler_flags(${pkg}-iface)
      target_link_libraries(${pkg}-iface PUBLIC ${TARGET}-flags)
    endforeach()
  endif()

  include(BMQTest)
  bmq_add_test(${TARGET} COMPAT)
endfunction()

# :: bmq_install_target_headers ::::::::::::::::::::::::::::::::::::::::::::::::
# This function generates installation command for target headers.
# NOTE: Please call `bbs_read_metadata` before calling this function to make
# sure this makes correctly.
function(bmq_install_target_headers target)
    bbs_install_target_headers(${ARGV})
endfunction()

# :: bmq_emit_pkg_config :::::::::::::::::::::::::::::::::::::::::::::::::::::::
# This function emits package config for the target.
function(bmq_emit_pkg_config target)
  cmake_parse_arguments(PARSE_ARGV 1
    ""
    ""
    "COMPONENT"
    "")
  bbs_assert_no_unparsed_args("")

  get_target_property(uor_name ${target} NAME)

  # default the component to the target name normalized as a dpkg name
  if(NOT _COMPONENT)
    string(REPLACE "_" "-" _COMPONENT ${uor_name})
  endif()

  bbs_emit_pkgconfig_file(TARGET ${target}
    PREFIX "${CMAKE_INSTALL_PREFIX}"
    VERSION "${BB_BUILDID_PKG_VERSION}" # todo: add real version
    COMPONENT ${_COMPONENT})
endfunction()
