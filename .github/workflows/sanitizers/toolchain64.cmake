# Configure compiler and ABI flags.
# This is a cmake toolchain file, designed to be specified by
# setting the CMAKE_TOOLCHAIN_FILE variable either as a command line parameter
# by using -D or by calling "set" before the top-level CMakeLists.txt call to
# project(). **This file should NOT be included directly**.  The purpose
# of the toolchain file is to prepare the build directory for cross-compiling.

cmake_minimum_required (VERSION 3.22)

set(IS_64BIT yes CACHE BOOL "Tool chain bitness.")

# Set the compiler flags. Use the CACHE instead of the *_INIT variants
# which are modified in the in the compiler initialization modules.
set(CMAKE_C_FLAGS "-march=westmere -m64 -fno-strict-aliasing" CACHE STRING "ABI C flags.")
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard.")
set(CMAKE_CXX_STANDARD_REQUIRED 1 CACHE BOOL "Standard version is required.")
set(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=0 -march=westmere -m64 -fno-strict-aliasing" CACHE STRING "ABI C++ flags.")

# Compile variable overrides.
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_LIST_FILE}) # Sets this file as the override for the builtin compiler variables and rules
set(CMAKE_C_LINK_FLAGS "-fasynchronous-unwind-tables -Wl,--enable-new-dtags -Wl,--gc-sections -Wl,-uplink_timestamp___")
set(CMAKE_CXX_LINK_FLAGS "-fasynchronous-unwind-tables -Wl,--enable-new-dtags -Wl,--gc-sections -Wl,-uplink_timestamp___")

# Set the path for looking up includes, libs and files.
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS yes)

# Set the installation directory for libraries. This is for use as the
# DESTINATION for calls to the cmake 'install' command.  Note that this is not
# an actual cmake variable, but has become a de facto standard for use in
# systems that employ a lib/lib64 deployment architecture.  To allow for use
# without a toolchain, use "./${CMAKE_INSTALL_LIBDIR}" to avoid configuration
# errors when this variable is not set.
set(CMAKE_INSTALL_LIBDIR lib64)
