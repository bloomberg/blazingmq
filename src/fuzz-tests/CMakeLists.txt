# fuzz-tests
# ------------

if(NOT BMQ_TARGET_FUZZTESTS_NEEDED OR NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?Clang")
  return()
endif()

# Create a custom target 'fuzztests' that builds all fuzz tests
add_custom_target("fuzztests")

find_package(BdeBuildSystem REQUIRED)
bbs_read_metadata(PACKAGE fuzz-tests)

function(BMQ_ADD_FUZZER fileName)
  get_filename_component(targetName "${fileName}" NAME_WE)
  add_executable(${targetName} ${fileName})

  target_bmq_default_compiler_flags(${targetName})
  target_include_directories(${targetName} PRIVATE ${fuzz-tests_INCLUDE_DIRS})
  target_link_libraries(${targetName} PRIVATE ${fuzz-tests_PCDEPS})
  target_link_options(${targetName} PUBLIC "-fsanitize=fuzzer")

  set_target_properties(${targetName}
    PROPERTIES OUTPUT_NAME ${targetName})

  # Add the current fuzzer to the 'fuzztests' rule
  add_dependencies("fuzztests" ${targetName})
endfunction()

file(GLOB sources "*fuzzer.cpp")
foreach (source ${sources})
  bmq_add_fuzzer(${source})
endforeach()

target_include_directories(eval_fuzzer
                           PRIVATE "$<TARGET_PROPERTY:bmq,BINARY_DIR>")
