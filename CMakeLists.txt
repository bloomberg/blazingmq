cmake_minimum_required( VERSION 3.23 )

project(BMQ
  VERSION 99.99.99
  DESCRIPTION "A modern high-performance open source message queuing system"
  HOMEPAGE_URL "https://bloomberg.github.io/blazingmq/"
  LANGUAGES C CXX)

# -----------------------------------------------------------------------------
#                                    MODULES
# -----------------------------------------------------------------------------
# Where to find custom CMake Modules
#
list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/etc/cmake" )

include( BBVendor )
include( BMQPlugins )
include( TargetBMQStyleUor )
include( BmqPackageProvider )

setup_package_provider()

# -----------------------------------------------------------------------------
#                                INITIALIZATION
# -----------------------------------------------------------------------------

enable_testing()
# Configure tests to be part of the 'all' target
add_custom_target(all.t ALL)

set( CMAKE_POSITION_INDEPENDENT_CODE ON )

# Define CXXFlags/LDFlags per build mode, that are OS/compiler specific
if( "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?Clang" )
  # Enable as much Clang compiler warnings as we can
  add_compile_options(
          "-Weverything"
          "-Wno-covered-switch-default"
          "-Wno-padded"
          "-Wno-global-constructors"    # bmqp_protocol mask constants
          "-Wno-conversion"
          "-Wno-undef"                  # BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES
          "-Wno-float-equal"            # operator== in generated classes
          "-Wno-documentation-unknown-command"
                                        # Don't warn about @bbref{}
  )



  if( DEFINED CMAKE_CXX_STANDARD AND CMAKE_CXX_STANDARD GREATER_EQUAL 17 )
    # The below warnings are disabled, but should be revisited and properly
    # addressed
    add_compile_options(
            "-Wno-atomic-implicit-seq-cst"
            "-Wno-c++98-compat-pedantic"
            "-Wno-deprecated"
            "-Wno-disabled-macro-expansion"
            "-Wno-extra-semi-stmt"
            "-Wno-zero-as-null-pointer-constant"
            # to compile hello_world: uncomment or add 'override' specifier (C++11)
            # "-Wno-suggest-override"
  )
  endif()
endif()

if( "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )
  # Disable the warning about ABI change around exception specifications from
  # C++17 as there is nothing anyone can do about it; we just should not mix
  # C++17 and non C++17 built code.
  set_property( DIRECTORY
                APPEND
                PROPERTY COMPILE_OPTIONS "-Wno-noexcept-type" )
endif()

# Enable '-Werror' for all targets when using GCC or Clang.
# NOTE: o In order to be effective, this must be called before creating the
#         targets (i.e. before the 'bbproject_add_group/application').
#       o Do it here (after 'bbproject_setup' so that overlays are not
#         compiled with -Werror)
set_property( DIRECTORY
              APPEND
              PROPERTY COMPILE_WARNING_AS_ERROR ON )
if( "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|(Apple)?Clang" )
  # Disable warnings about unused functions or variables when not in Debug
  # build, so that we don't get warn (and build failure due to warn-as-error)
  # for code only used within BSLS_ASSERT_SAFE.
  if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS "-Wno-unused-variable")
    set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS "-Wno-unused-function")
    set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS "-Wno-unused-but-set-variable")
  endif()
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  # In Debug build, ensure that BSLS_ASSERT_SAFE macro is enabled
  add_definitions("-DBSLS_ASSERT_LEVEL_ASSERT_SAFE")
endif()

# This repo contains the canonical source of multiple, independently released,
# UORs (bmq, bmqbrkr, bmqbrkrcfg, bmqtool).  When releasing a UOR, that
# specific target is selected through injection of the 'INSTALL_TARGETS'
# variable.  Many static analysis tool leverage the generated
# 'compile_commands.json' in order to know which sources to work with.
# Therefore, we should only add the necessary targets based on the UOR being
# built.
#
# To that purpose, we have to 'manually' build the reverse dependencies, so
# that we will only add targets to the intended ones, based on what is
# currently being built.  We will set a "BMQ_TARGET_<xyz>_NEEDED" value to
# YES/NO.
# - if INSTALL_TARGETS is empty, this means we are not doing an integration
#   build, but just a normal developer build, therefore include all targets
# - otherwise, selectively enable targets based on what is being built

if (NOT DEFINED INSTALL_TARGETS)
  # If no specic install targets has been set, then enable them all
  set(BMQ_TARGET_BMQBRKR_NEEDED        YES)
  set(BMQ_TARGET_BMQBRKRCFG_NEEDED     YES)
  set(BMQ_TARGET_BMQTOOL_NEEDED        YES)
  set(BMQ_TARGET_BMQSTORAGETOOL_NEEDED YES)
  set(BMQ_TARGET_BMQ_NEEDED            YES)
  set(BMQ_TARGET_MQB_NEEDED            YES)
  set(BMQ_TARGET_E_BMQBRKR_NEEDED      YES)
  set(BMQ_TARGET_TUTORIAL_NEEDED       YES)
  set(BMQ_TARGET_PROMETHEUS_NEEDED     NO)
  set(BMQ_TARGET_IT_NEEDED             YES)
  set(BMQ_TARGET_FUZZTESTS_NEEDED      NO)
else()
  bbproject_check_install_target("bmqbrkr"         installBMQBRKR)
  bbproject_check_install_target("BMQBRKR_NIGHTLY" installNightly)

  if (installNightly)
    if (NOT installBMQBRKR)
      string(APPEND INSTALL_TARGETS ";BMQBRKR")
      set(installBMQBRKR YES)
    endif()
  endif()

  # Disable all by default, and then we'll enable selectively based on the
  # content of INSTALL_TARGETS
  set(BMQ_TARGET_BMQBRKR_NEEDED        NO)
  set(BMQ_TARGET_BMQBRKRCFG_NEEDED     NO)
  set(BMQ_TARGET_BMQTOOL_NEEDED        NO)
  set(BMQ_TARGET_BMQSTORAGETOOL_NEEDED NO)
  set(BMQ_TARGET_BMQ_NEEDED            NO)
  set(BMQ_TARGET_MQB_NEEDED            NO)
  set(BMQ_TARGET_TUTORIAL_NEEDED       NO)
  set(BMQ_TARGET_PROMETHEUS_NEEDED     NO)
  set(BMQ_TARGET_IT_NEEDED             NO)
  set(BMQ_TARGET_FUZZTESTS_NEEDED      NO)

  bbproject_check_install_target("bmq"              installBMQ)
  bbproject_check_install_target("mqb"              installMQB)
  bbproject_check_install_target("bmqbrkrcfg"       installBMQBRKRCFG)
  bbproject_check_install_target("bmqtool"          installBMQTOOL)
  bbproject_check_install_target("bmqstoragetool"   installBMQSTORAGETOOL)
  bbproject_check_install_target("prometheus"       installPROMETHEUS)
  bbproject_check_install_target("integrationtests" installIT)
  bbproject_check_install_target("fuzztests"        installFUZZTESTS)

  if (installBMQ)
    set(BMQ_TARGET_BMQ_NEEDED YES)
  endif()

  if (installMQB)
    set(BMQ_TARGET_BMQ_NEEDED YES)
    set(BMQ_TARGET_MQB_NEEDED YES)
  endif()

  if (installBMQBRKR)
    set(BMQ_TARGET_BMQ_NEEDED       YES)
    set(BMQ_TARGET_MQB_NEEDED       YES)
    set(BMQ_TARGET_BMQBRKR_NEEDED   YES)
    set(BMQ_TARGET_E_BMQBRKR_NEEDED YES)
  endif()

  if (installBMQBRKRCFG)
    set(BMQ_TARGET_BMQBRKRCFG_NEEDED YES)
  endif()

  if (installBMQTOOL)
    set(BMQ_TARGET_BMQ_NEEDED     YES)
    set(BMQ_TARGET_MQB_NEEDED     YES)
    set(BMQ_TARGET_BMQTOOL_NEEDED YES)
  endif()

  if (installBMQSTORAGETOOL)
    set(BMQ_TARGET_BMQ_NEEDED            YES)
    set(BMQ_TARGET_MQB_NEEDED            YES)
    set(BMQ_TARGET_BMQSTORAGETOOL_NEEDED YES)
  endif()

  if (installPROMETHEUS)
    set(BMQ_TARGET_BMQ_NEEDED        YES)
    set(BMQ_TARGET_MQB_NEEDED        YES)
    set(BMQ_TARGET_PROMETHEUS_NEEDED YES)
  endif()

  if (installIT)
    set(BMQ_TARGET_BMQ_NEEDED       YES)
    set(BMQ_TARGET_MQB_NEEDED       YES)
    set(BMQ_TARGET_BMQBRKR_NEEDED   YES)
    set(BMQ_TARGET_BMQTOOL_NEEDED   YES)
  endif()

  if (installFUZZTESTS)
    set(BMQ_TARGET_BMQ_NEEDED       YES)
    set(BMQ_TARGET_MQB_NEEDED       YES)
    set(BMQ_TARGET_FUZZTESTS_NEEDED YES)
  endif()
endif()

find_package(Git)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe HEAD
    WORKING_DIRECTORY "${local_dir}"
    OUTPUT_VARIABLE PROJECT_VERSION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

# -----------------------------------------------------------------------------
#                                DOCUMENTATION
# -----------------------------------------------------------------------------

find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(docs
    COMMENT "Build public and internal documentation"
  )

  # API Documentation for libbmq

  # Each CMake variable `DOXYGEN_something` becomes a Doxygen configuration
  # named `something`.
  set(DOXYGEN_ALIASES
    "bbref{1}=@ref BloombergLP::\\1 \\\"\\1\\\""
  )
  set(DOXYGEN_EXTRACT_ALL            YES)
  set(DOXYGEN_EXTRACT_STATIC         YES)
  set(DOXYGEN_EXCLUDE_PATTERNS       *.t.cpp *.cpp)
  set(DOXYGEN_EXCLUDE_SYMBOLS
    BloombergLP::bmqimp
    BloombergLP::bmqp
    BloombergLP::bslmt
    BloombergLP::bmqst
  )
  set(DOXYGEN_SHOW_INCLUDE_FILES     NO)
  set(DOXYGEN_SORT_MEMBER_DOCS       NO)
  set(DOXYGEN_MACRO_EXPANSION        YES)
  set(DOXYGEN_PREDEFINED
    "BSLS_KEYWORD_FINAL=final"
    "BSLS_KEYWORD_DELETED=delete"
    "BSLS_KEYWORD_OVERRIDE=override"
    "BSLMF_NESTED_TRAIT_DECLARATION(...)="
  )
  set(DOXYGEN_EXPAND_AS_DEFINED
    "BSLS_ASSERT_SAFE_IS_ACTIVE=1"
    "BSLS_COMPILER_FEATURES_SUPPORT_GENERALIZED_INITIALIZERS"
    "BSLS_LIBRARYFEATURES_HAS_CPP17_BASELINE_LIBRARY"
    "BSLS_LIBRARYFEATURES_HAS_CPP11_UNIQUE_PTR"
  )
  set(DOXYGEN_SHOW_NAMESPACES        NO)
  set(DOXYGEN_REPEAT_BRIEF           YES)
  set(DOXYGEN_INLINE_INHERITED_MEMB  YES)
  set(DOXYGEN_STRIP_FROM_PATH        src/groups/bmq)
  set(DOXYGEN_OUTPUT_DIRECTORY       apidocs)
  set(DOXYGEN_IMAGE_PATH             docs/assets/images)
  set(DOXYGEN_PROJECT_ICON           docs/favicon.ico)
  set(DOXYGEN_PROJECT_LOGO           docs/assets/images/blazingmq_logo.svg)
  set(DOXYGEN_PROJECT_NAME           libbmq)
  set(DOXYGEN_PROJECT_BRIEF
    "C++ SDK for BlazingMQ clients and plugins"
  )
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ./README.md)
  set(DOXYGEN_REFERENCED_BY_RELATION YES)
  set(DOXYGEN_REFERENCES             YES)
  set(DOXYGEN_ALPHABETICAL_INDEX     NO)
  set(DOXYGEN_FULL_SIDEBAR           YES)
  set(DOXYGEN_GENERATE_TREEVIEW      YES)
  doxygen_add_docs(
    apidocs
    src/groups/bmq/bmqa
    src/groups/bmq/bmqt
    src/groups/bmq/bmqpi
    README.md
    COMMENT "Generate public Doxygen documentation for libbmq"
  )
  add_dependencies(docs apidocs)

  # Internal documentation for both libbmq and libmqb.

  unset(DOXYGEN_EXCLUDE_SYMBOLS)
  set(DOXYGEN_EXTRACT_PRIVATE        YES)
  set(DOXYGEN_SHOW_INCLUDE_FILES     YES)
  set(DOXYGEN_SHOW_NAMESPACES        YES)
  set(DOXYGEN_GENERATE_TODOLIST      YES)
  set(DOXYGEN_STRIP_FROM_PATH        src/groups)
  set(DOXYGEN_OUTPUT_DIRECTORY       internaldocs)
  doxygen_add_docs(
    internaldocs
    src/groups/
    COMMENT "Generate internal Doxygen documentation for libbmq and libmqb"
  )
  add_dependencies(docs internaldocs)
endif()

add_library(bmqbrkr_plugins INTERFACE)
add_library(blazingmq::bmqbrkr_plugins ALIAS bmqbrkr_plugins)

# -----------------------------------------------------------------------------
#                                   PROJECTS
# -----------------------------------------------------------------------------
add_subdirectory( "src/groups" )
add_subdirectory( "src/applications" )
add_subdirectory( "src/tutorials" )
add_subdirectory( "src/plugins" )
add_subdirectory( "src/integration-tests" )
add_subdirectory( "src/fuzz-tests" )


# -----------------------------------------------------------------------------
#                                   PLUGINS
# -----------------------------------------------------------------------------

if(NOT BMQ_TARGET_BMQBRKR_NEEDED)
  return()
endif()

# Install all the headers for mqb + bmq 
install(TARGETS bmqbrkr_plugins
        EXPORT BmqbrkrPluginsTargets
        FILE_SET HEADERS
          COMPONENT bmqbrkr_plugins
)
install(EXPORT BmqbrkrPluginsTargets NAMESPACE blazingmq:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BmqbrkrPlugins" COMPONENT bmqbrkr_plugins)

find_package(BdeBuildSystem REQUIRED)
bbs_read_metadata(GROUP mqb SOURCE_DIR src/groups/mqb)
bbs_read_metadata(GROUP bmq SOURCE_DIR src/groups/bmq)
set(bmqbrkr_plugins_deps)
list(APPEND bmqbrkr_plugins_deps ${mqb_DEPENDS} ${bmq_DEPENDS})
list(REMOVE_DUPLICATES bmqbrkr_plugins_deps)
list(REMOVE_ITEM bmqbrkr_plugins_deps bmq) # don't need this dependency
include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/etc/cmake/BmqbrkrPluginsConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/BmqbrkrPluginsConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BmqbrkrPlugins
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/BmqbrkrPluginsConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BmqbrkrPlugins
        COMPONENT bmqbrkr_plugins)

