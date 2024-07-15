# Compiler-less toolchain for building with {Clang, libc++} + MemorySanitizer.
# The actual compiler is passed via `CXX` and `CC` environment variables.

cmake_minimum_required (VERSION 3.25)

include("$ENV{DIR_SCRIPTS}/toolchain64.cmake")

if(DEFINED ENV{CC})
  set(CMAKE_C_COMPILER $ENV{CC} CACHE STRING "Instrumentation C compiler" FORCE)
endif()

if(DEFINED ENV{CXX})
  set(CMAKE_CXX_COMPILER $ENV{CXX} CACHE STRING "Instrumentation C++ compiler" FORCE)
endif()

if(DEFINED ENV{CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES})
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES $ENV{CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES})
endif()

set(LIBCXX_BUILD_PATH "$ENV{LIBCXX_BUILD_PATH}")

# Force disabling the use of Readline. This is a holdover until readline builds with -fPIC.
set(BMQ_DISABLE_READLINE TRUE)

set(TOOLCHAIN_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}")
set(TOOLCHAIN_C_FLAGS   "${CMAKE_C_FLAGS_DEBUG}")

set(MSAN_SUPPRESSION_LIST_PATH "$ENV{DIR_SRC_BMQ}/etc/msansup.txt")

# Build shared flags.
string(CONCAT TOOLCHAIN_SHARED_FLAGS
       "${}"
       "${TOOLCHAIN_SHARED_FLAGS} "
       "-O0 "
       "-g "
       "-fno-omit-frame-pointer "
       "-fdiagnostics-show-option "
       "-fsanitize=memory "
       "-fsanitize-blacklist=${MSAN_SUPPRESSION_LIST_PATH} "
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

# Conditionally add flags helpful for debugging MemorySanitizer issues.
if (DEBUG_MEMORY_SANITIZER)
    string(CONCAT TOOLCHAIN_DEBUG_FLAGS
           "${TOOLCHAIN_SHARED_FLAGS} "
           "-fsanitize-memory-track-origins=2 "
           "-fno-optimize-sibling-calls "
          )
endif()

# Set the final configuration variables, as understood by CMake.
set_build_type(DEBUG)

# Disable GNU c++ extensions.
set(CMAKE_CXX_EXTENSIONS OFF)
