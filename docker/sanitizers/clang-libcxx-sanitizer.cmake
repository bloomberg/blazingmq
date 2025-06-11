# Compiler-less toolchain for building with {Clang, libc++} + Sanitizer.
# The actual compiler is passed via `CXX` and `CC` environment variables.
# Sanitizer name (asan/msan/tsan/ubsan) is passed via `SANITIZER_NAME` environment variable.

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

if(DEFINED ENV{LIBCXX_BUILD_PATH})
  set(LIBCXX_BUILD_PATH "$ENV{LIBCXX_BUILD_PATH}")
endif()

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

if($ENV{FUZZER} STREQUAL "off")
    # Use instrumented libc++ (LLVM) standard library for C++ built from source
    string(CONCAT TOOLCHAIN_CXX_FLAGS
           "${TOOLCHAIN_CXX_FLAGS} "
           "-stdlib=libc++ "
           "-I${LIBCXX_BUILD_PATH}/include/c++/v1 "
          )
    string(CONCAT TOOLCHAIN_LINKER_FLAGS
           "${CMAKE_LINKER_FLAGS_DEBUG}"
           "-stdlib=libc++ "
           "-L${LIBCXX_BUILD_PATH}/lib "
           "-Wl,-rpath,${LIBCXX_BUILD_PATH}/lib "
           "-lc++abi "
           )
elseif($ENV{FUZZER} STREQUAL "on")
    # Use non-instrumented libstdc++ (GNU) standard library for C++. The reason
    # why we use this library is that preinstalled clang and libFuzzer depend
    # on GNU library. If we try to link it to a fuzzer with other dependencies,
    # built with LLVM libc++, it leads to a conflict. An alternative is to
    # build clang and libFuzzer ourselves with LLVM libc++. Though it takes too
    # much time. Also it contradicts BFuzz workflow that we are trying to
    # imitate here.
    string(CONCAT TOOLCHAIN_LINKER_FLAGS
           "${CMAKE_LINKER_FLAGS_DEBUG}"
           "-lstdc++ "
           )
endif()

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
       "-Wno-unsafe-buffer-usage "
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

# Define sanitizer specific flags
set(SANITIZER_NAME $ENV{SANITIZER_NAME})
if(SANITIZER_NAME STREQUAL "asan")
  set(TOOLCHAIN_DEBUG_FLAGS "-fsanitize=address ")
elseif(SANITIZER_NAME STREQUAL "msan")
  set(MSAN_SUPPRESSION_LIST_PATH "$ENV{DIR_SRC_BMQ}/etc/msansup.txt")
  set(TOOLCHAIN_DEBUG_FLAGS "-fsanitize=memory -fsanitize-blacklist=${MSAN_SUPPRESSION_LIST_PATH} ")
  # Conditionally add flags helpful for debugging MemorySanitizer issues.
  if (DEBUG_MEMORY_SANITIZER)
      string(CONCAT TOOLCHAIN_DEBUG_FLAGS
            "${TOOLCHAIN_DEBUG_FLAGS} "
            "-fsanitize-memory-track-origins=2 "
            )
  endif()
elseif(SANITIZER_NAME STREQUAL "tsan")
  set(TOOLCHAIN_DEBUG_FLAGS "-fsanitize=thread ")
elseif(SANITIZER_NAME STREQUAL "ubsan")
  set(TOOLCHAIN_DEBUG_FLAGS "-fsanitize=undefined ")
else()
  message(FATAL_ERROR "Unexpected sanitizer name: ${SANITIZER_NAME}")
endif()

if($ENV{FUZZER} STREQUAL "on")
  string(APPEND TOOLCHAIN_DEBUG_FLAGS "-fsanitize=fuzzer-no-link ")
endif()

# Set the final configuration variables, as understood by CMake.
set_build_type(DEBUG)

# Disable GNU c++ extensions.
set(CMAKE_CXX_EXTENSIONS OFF)
