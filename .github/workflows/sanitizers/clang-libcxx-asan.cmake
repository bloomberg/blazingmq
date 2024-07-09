# Compiler-less toolchain for building with {Clang, libc++} + AddressSanitizer.
# The actual compiler is passed via `CXX` and `CC` environment variables.
#-=-

cmake_minimum_required (VERSION 3.25)

# if(NOT DEFINED DISTRIBUTION_REFROOT)
#     if(DEFINED ENV{DISTRIBUTION_REFROOT})
#         set(DISTRIBUTION_REFROOT "$ENV{DISTRIBUTION_REFROOT}/" CACHE STRING "BB Dpkg root set from environment variable.")
#     else()
#         get_filename_component(REFROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../../ REALPATH)
#         set(DISTRIBUTION_REFROOT ${REFROOT}/ CACHE STRING "BB Dpkg root set from toolchain file location.")
#     endif()
# endif()

include("$ENV{DIR_SCRIPTS}/BBToolchain64.cmake")

if(DEFINED ENV{CC})
  set(CMAKE_C_COMPILER $ENV{CC} CACHE STRING "Instrumentation C compiler" FORCE)
endif()

if(DEFINED ENV{CXX})
  set(CMAKE_CXX_COMPILER $ENV{CXX} CACHE STRING "Instrumentation C++ compiler" FORCE)
endif()

set(LIBCXX_BUILD_PATH "$ENV{LIBCXX_BUILD_PATH}")

# Force disabling the use of Readline. This is a holdover until readline builds with -fPIC.
# https://bbgithub.dev.bloomberg.com/swt/readline/issues/8
set(BMQ_DISABLE_READLINE TRUE)

set(TOOLCHAIN_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}")
set(TOOLCHAIN_C_FLAGS   "${CMAKE_C_FLAGS_DEBUG}")

# Build shared flags.
string(CONCAT TOOLCHAIN_SHARED_FLAGS
       "${}"
       "${TOOLCHAIN_SHARED_FLAGS} "
       "-O0 "
       "-g "
       "-fno-omit-frame-pointer "
       "-fno-optimize-sibling-calls "
       "-fdiagnostics-show-option "
       "-isystem/usr/include "
       "-isystem/usr/lib/llvm-18/lib/clang/18/include "
      )

# Apply shared flags to each language.
string(CONCAT TOOLCHAIN_CXX_FLAGS
       "${TOOLCHAIN_CXX_FLAGS} "
       "${TOOLCHAIN_SHARED_FLAGS} "
      )
string(CONCAT TOOLCHAIN_C_FLAGS
       "${TOOLCHAIN_C_FLAGS} "
       "${TOOLCHAIN_SHARED_FLAGS} "
      )

# Use libc++ standard library for C++.
string(CONCAT TOOLCHAIN_CXX_FLAGS
       "${TOOLCHAIN_CXX_FLAGS} "
       "-stdlib=libc++ "
       "-I${LIBCXX_BUILD_PATH}/include/c++/v1 "
      )

# Suppress some warnings when building C++ code.
string(CONCAT TOOLCHAIN_CXX_FLAGS
       "${TOOLCHAIN_CXX_FLAGS} "
       "-Wno-c++98-compat "
       "-Wno-c++98-compat-extra-semi "
       "-Wno-c++98-compat-pedantic "
       "-Wno-deprecated "
       "-Wno-deprecated-declarations "
       "-Wno-disabled-macro-expansion "
       "-Wno-extra-semi-stmt "
       "-Wno-inconsistent-missing-destructor-override "
       "-Wno-inconsistent-missing-override "
       "-Wno-old-style-cast "
       "-Wno-undef "
       "-Wno-zero-as-null-pointer-constant "
       )

# Define linker flags (used for both shared-objects and executables).
string( CONCAT TOOLCHAIN_LINKER_FLAGS
        "${CMAKE_LINKER_FLAGS_DEBUG}"
        "-stdlib=libc++ "
        "-L${LIBCXX_BUILD_PATH}/lib "
        "-Wl,-rpath,${LIBCXX_BUILD_PATH}/lib "
        "-lc++abi "
        )

set( TOOLCHAIN_EXE_FLAGS "${TOOLCHAIN_LINKER_FLAGS}" )

macro(set_build_type type)
  set(CMAKE_CXX_FLAGS_${type}
    "${TOOLCHAIN_CXX_FLAGS} ${TOOLCHAIN_${type}_FLAGS}"  CACHE STRING "Default" FORCE)
  set(CMAKE_C_FLAGS_${type}
    "${TOOLCHAIN_C_FLAGS} ${TOOLCHAIN_${type}_FLAGS}"  CACHE STRING "Default" FORCE)
  set(CMAKE_EXE_LINKER_FLAGS_${type}
    "${TOOLCHAIN_EXE_FLAGS} ${TOOLCHAIN_${type}_FLAGS} -static-libsan"  CACHE STRING "Default" FORCE)
  set(CMAKE_SHARED_LINKER_FLAGS_${type}
    "${TOOLCHAIN_LINKER_FLAGS} ${TOOLCHAIN_${type}_FLAGS}"  CACHE STRING "Default" FORCE)
  set(CMAKE_MODULE_LINKER_FLAGS_${type}
    "${TOOLCHAIN_LINKER_FLAGS} ${TOOLCHAIN_${type}_FLAGS}"  CACHE STRING "Default" FORCE)
endmacro()

set( TOOLCHAIN_DEBUG_FLAGS
     "-fsanitize=address "
     )

# Set the final configuration variables, as understood by CMake.
set_build_type(DEBUG)

# Disable GNU c++ extensions.
set(CMAKE_CXX_EXTENSIONS OFF)
