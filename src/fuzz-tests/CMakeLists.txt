# fuzz-tests
# ------------

if(NOT BMQ_TARGET_FUZZTESTS_NEEDED OR NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?Clang")
  return()
endif()

# Create a custom target 'fuzztests' that builds all fuzz tests
add_custom_target("fuzztests")

find_package(BdeBuildSystem REQUIRED)
bbs_read_metadata(PACKAGE fuzz-tests)

file(GLOB sources "*fuzzer.cpp")
foreach (source ${sources})
  string(REGEX MATCH "\/([A-Za-z_]+)\.cpp$" _ ${source})
  set(targetName ${CMAKE_MATCH_1})
  add_executable(${targetName} ${source})

  target_bmq_default_compiler_flags(${targetName})
  target_include_directories(${targetName} PRIVATE ${fuzz-tests_INCLUDE_DIRS})
  target_link_libraries(${targetName} PRIVATE ${fuzz-tests_PCDEPS})
  target_link_options(${targetName} PRIVATE "-fsanitize=fuzzer")

  set_target_properties(${targetName}
    PROPERTIES OUTPUT_NAME ${targetName})

  # Add the current tutorial to the 'fuzztests' rule
  add_dependencies("fuzztests" ${targetName})
endforeach()

get_target_property(BMQ_BIN_DIR bmq BINARY_DIR)
target_include_directories(eval_fuzzer
                           PRIVATE "$<TARGET_PROPERTY:bmq,BINARY_DIR>")
