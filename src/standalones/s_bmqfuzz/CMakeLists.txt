# s_bmqfuzz
# ---------

if(NOT BMQ_TARGET_FUZZTESTS_NEEDED OR NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?Clang")
  return()
endif()

# Create a custom target 'fuzztests' that builds all fuzz tests
add_custom_target("fuzztests")

find_package(BdeBuildSystem REQUIRED)
bbs_read_metadata(PACKAGE s_bmqfuzz)

function(BMQ_ADD_FUZZER fileName)
  get_filename_component(targetName "${fileName}" NAME_WE)
  message("targetName: ${targetName}")
  add_executable(${targetName} ${fileName})

  target_bmq_default_compiler_flags(${targetName})
  target_include_directories(${targetName} PRIVATE ${s_bmqfuzz_INCLUDE_DIRS})
  target_link_libraries(${targetName} PRIVATE ${s_bmqfuzz_PCDEPS})

  set_target_properties(${targetName}
    PROPERTIES OUTPUT_NAME ${targetName})

  # Add the current fuzzer to the 'fuzztests' rule
  add_dependencies("fuzztests" ${targetName})
endfunction()

foreach (source ${s_bmqfuzz_SOURCE_FILES})
  message("source: ${source}")
  bmq_add_fuzzer(${source})
endforeach()

target_include_directories(s_bmqfuzz_eval
                           PRIVATE "$<TARGET_PROPERTY:bmq,BINARY_DIR>")
