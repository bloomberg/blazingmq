# BBToolchain64.cmake *GENERATED FILE - DO NOT EDIT* -*-cmake-*-

cmake_minimum_required (VERSION 3.22)

#@PURPOSE: Configure compiler, ABI flags and directory structure.
#
#@DESCRIPTION: This is a cmake toolchain file, designed to be specified by
# setting the CMAKE_TOOLCHAIN_FILE variable either as a command line parameter
# by using -D or by calling "set" before the top-level CMakeLists.txt call to
# project(). **This file should NOT be included directly**.  The purpose
# of the toolchain file is to prepare the build directory for cross-compiling.
# In our context, this means setting the compiler to be the production ready
# compiler version as well as the flags necessary for ABI compatibility so that
# the artifacts can be used on any machine in production.  Search paths for
# CMake find_### methods as well as the CMake PkgConfig integration are
# configured here.  In addition, this file will set basic variables necessary
# for navigating both the directory structure.  These variables should only
# be referenced by distribution modules such as Pekludge.cmake, as they are not
# guaranteed to exist in alternative toolchains.  Extending modules may take
# use of the following variables:
# * DISTRIBUTION_REFROOT - Root of dpkg artifacts.
# * ROBOLIB_REFROOT - Root of classic robolib artifacts.
# * IS_64BIT - Boolean value needed for directory navigation.
#
# Note that both refroot variables will first use the cmake command line
# parameter definition, then the environment variable if set and finally will
# use the toolchain file location to infer the refroots.

# Set base environment variables.
# if(NOT DEFINED DISTRIBUTION_REFROOT)
#     if(DEFINED ENV{DISTRIBUTION_REFROOT})
#         set(DISTRIBUTION_REFROOT "$ENV{DISTRIBUTION_REFROOT}/" CACHE STRING "BB Dpkg root set from environment variable.")
#     else()
#         get_filename_component(REFROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../ REALPATH)
#         set(DISTRIBUTION_REFROOT ${REFROOT}/ CACHE STRING "BB Dpkg root set from toolchain file location.")
#     endif()
# endif()
# if(NOT DEFINED ROBOLIB_REFROOT)
#     if(DEFINED ENV{ROBOLIB_REFROOT})
#         set(ROBOLIB_REFROOT "$ENV{ROBOLIB_REFROOT}/" CACHE STRING "BB Robo root set from environment variable.")
#     else()
#         get_filename_component(REFROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../ REALPATH)
#         set(ROBOLIB_REFROOT ${REFROOT}/ CACHE STRING "BB Robo root set from toolchain file location.")
#     endif()
# endif()
set(IS_64BIT yes CACHE BOOL "Bloomberg tool chain bitness.")

# Set distribution compilers and ABI flags.
# set(CMAKE_C_COMPILER ${DISTRIBUTION_REFROOT}/opt/bb/bin/gcc CACHE FILEPATH "C Compiler path")
# set(CMAKE_CXX_COMPILER ${DISTRIBUTION_REFROOT}/opt/bb/bin/g++ CACHE FILEPATH "C++ Compiler path")
# set(CMAKE_Fortran_COMPILER ${DISTRIBUTION_REFROOT}/opt/bb/bin/gfortran CACHE FILEPATH "Fortran Compiler path")

# Set the compiler flags. Use the CACHE instead of the *_INIT variants
# which are modified in the in the compiler initialization modules.
set(CMAKE_C_FLAGS "-march=westmere -m64 -fno-strict-aliasing" CACHE STRING "Bloomberg ABI C flags.")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O2 -fno-omit-frame-pointer" CACHE STRING "Bloomberg ABI C flags.")
set(CMAKE_CXX_STANDARD 20 CACHE STRING "Bloomberg C++ standard.")
set(CMAKE_CXX_STANDARD_REQUIRED 1 CACHE BOOL "Bloomberg standard version is required.")
set(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=0 -march=westmere -m64 -fno-strict-aliasing" CACHE STRING "Bloomberg ABI C++ flags.")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -fno-omit-frame-pointer" CACHE STRING "Bloomberg ABI C++ flags.")
set(CMAKE_Fortran_FLAGS "-march=westmere -mstackrealign -fasynchronous-unwind-tables -m64 -std=legacy -Wall -Wno-overwrite-recursive -fimplicit-none -fdec -frecursive -fno-automatic -fno-range-check -fno-align-commons -fd-lines-as-comments -static-libgfortran -fno-dec-promotion -fno-dec-old-init -fno-dec-non-integer-index -fno-dec-sequence -fno-dec-add-missing-indexes -fno-dec-non-logical-if -fno-dec-override-kind -fallow-invalid-boz -fallow-argument-mismatch" CACHE STRING "Bloomberg ABI Fortran flags.")
set(CMAKE_Fortran_FLAGS_RELWITHDEBINFO "-g -O2 -fno-omit-frame-pointer" CACHE STRING "Bloomberg ABI Fortran flags.")

# Compile variable overrides.
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_LIST_FILE}) # Sets this file as the override for the builtin compiler variables and rules
set(CMAKE_C_LINK_FLAGS "-fasynchronous-unwind-tables -Wl,--enable-new-dtags -Wl,--gc-sections -Wl,-uplink_timestamp___")
set(CMAKE_CXX_LINK_FLAGS "-fasynchronous-unwind-tables -Wl,--enable-new-dtags -Wl,--gc-sections -Wl,-uplink_timestamp___")
set(CMAKE_Fortran_LINK_FLAGS "")

# Ninja generator can preprocess source files on some platforms to build a better dependency tree than the Makefiles generator.
# We don't use include directives in our Fortran here at Bloomberg so this is not needed.
set(CMAKE_Fortran_PREPROCESS OFF)

set(CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES "-l:libgfortran.a;-l:libquadmath.a")

# Set the platform RPATH for production deployment.
set(CMAKE_PLATFORM_REQUIRED_RUNTIME_PATH /usr/lib64 /lib64 /bb/bin/so/64/NS02 /bb/bin/so/64 /bb/bin .)

# Set the local module path to include standard Bloomberg modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR} ${DISTRIBUTION_REFROOT}/opt/bb/share/cmake/Modules)

# Set the program path for pkg-config.  Versions 0.29.1 and older without our
# patch must be avoided since they do not scale to the size of our distribution.
# find_program(PKG_CONFIG_EXECUTABLE pkg-config PATHS
#   ${DISTRIBUTION_REFROOT}/opt/bb/lib64/bin
#   /opt/bb/lib64/bin
#   NO_SYSTEM_ENVIRONMENT_PATH)

# Toolchain files could be invoked multiple times in a single CMake run,
# therefore we need to check if we have appended the refroot pkgconfig path
# in a previous invokation before appending. We cannot use a cached variable
# to achieve this because the cache is not available between separate
# invokations of the toolchain file.
# set(ROBO_PKG_CONFIG_PATH "${DISTRIBUTION_REFROOT}/opt/bb/lib64/robo/pkgconfig:${DISTRIBUTION_REFROOT}/opt/bb/lib64/pkgconfig" CACHE STRING "The location of the robo pkgconfig files.")
# if (DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" MATCHES ".*${ROBO_PKG_CONFIG_PATH}$")
#     message(STATUS "WARNING: Using user supplied PKG_CONFIG_PATH=$ENV{PKG_CONFIG_PATH}")
# endif()
# if (NOT "$ENV{PKG_CONFIG_PATH}" MATCHES ".*${ROBO_PKG_CONFIG_PATH}$")
#     set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${ROBO_PKG_CONFIG_PATH}")
# endif()

# set(ENV{PKG_CONFIG_SYSROOT_DIR} ${DISTRIBUTION_REFROOT})

# Set the path for looking up includes, libs and files.
# list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${DISTRIBUTION_REFROOT}/opt/bb)
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS yes)

# Set the installation directory for libraries.  This is for use as the
# DESTINATION for calls to the cmake 'install' command.  Note that this is not
# an actual cmake variable, but has become a de facto standard for use in
# systems that employ a lib/lib64 deployment architecture.  To allow for use
# without a toolchain, use "./${CMAKE_INSTALL_LIBDIR}" to avoid configuration
# errors when this variable is not set.
set(CMAKE_INSTALL_LIBDIR lib64)

# if ("amd64" STREQUAL "aix6-powerpc" AND IS_64BIT)
#     # On AIX, the archiver defaults to 32 bit only.  This is a known issue
#     # in CMake and unlikely to be fixed.  The following is the recommended
#     # workaround.  Note that the AIX archiver can also use an environment
#     # variable, but in cmake, there is no way to set the env at build time.
#     set(CMAKE_CXX_ARCHIVE_CREATE
#         "<CMAKE_AR> -X64 cr <TARGET> <LINK_FLAGS> <OBJECTS>")
#     set(CMAKE_CXX_ARCHIVE_APPEND
#         "<CMAKE_AR> -X64 r <TARGET> <LINK_FLAGS> <OBJECTS>")
#     set(CMAKE_CXX_ARCHIVE_FINISH
#         "<CMAKE_RANLIB> -X64 <TARGET> <LINK_FLAGS>")
#     set(CMAKE_C_ARCHIVE_CREATE
#         "<CMAKE_AR> -X64 cr <TARGET> <LINK_FLAGS> <OBJECTS>")
#     set(CMAKE_C_ARCHIVE_APPEND
#         "<CMAKE_AR> -X64 r <TARGET> <LINK_FLAGS> <OBJECTS>")
#     set(CMAKE_C_ARCHIVE_FINISH
#         "<CMAKE_RANLIB> -X64 <TARGET> <LINK_FLAGS>")
#     set(CMAKE_Fortran_ARCHIVE_CREATE
#         "<CMAKE_AR> -X64 cr <TARGET> <LINK_FLAGS> <OBJECTS>")
#     set(CMAKE_Fortran_ARCHIVE_APPEND
#         "<CMAKE_AR> -X64 r <TARGET> <LINK_FLAGS> <OBJECTS>")
#     set(CMAKE_Fortran_ARCHIVE_FINISH
#         "<CMAKE_RANLIB> -X64 <TARGET> <LINK_FLAGS>")
# endif()

# -----------------------------------------------------------------------------
# NOTICE:
#      Copyright (C) Bloomberg L.P., 2016
#      All Rights Reserved.
#      Property of Bloomberg L.P. (BLP)
#      This software is made available solely pursuant to the
#      terms of a BLP license agreement which governs its use.
# ----------------------------- END-OF-FILE -----------------------------------
