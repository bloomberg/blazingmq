# bmq
# ---

if(NOT BMQ_TARGET_BMQ_NEEDED)
  return()
endif()

# Add the libbmq group library only installing the public headers
add_library(bmq)
set_property(TARGET bmq PROPERTY POSITION_INDEPENDENT_CODE ON)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|(Apple)?Clang")
  target_compile_options(bmq PRIVATE "-msse4.2")
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|(Apple)?Clang")
  # NOTE: The followings are using the 'deprecated' COMPILE_FLAGS property and
  # not COMPILE_OPTIONS because the later doesn't seem to work for a
  # 'SOURCE' target.

  # TBD: temporarily until certain deprecated bmqt::AckResult enum values can
  # be removed ( >libbmq-1.3.5)
  set_property(SOURCE "bmqp/bmqp_protocolutil.cpp"
    APPEND
    PROPERTY COMPILE_FLAGS "-Wno-switch-enum")
  set_property(SOURCE "bmqp/test/bmqp_messageproperties.t.cpp"
    APPEND
    PROPERTY COMPILE_FLAGS "-Wno-self-assign-overloaded")

  set_property(SOURCE "bmqc/bmqc_array.t.cpp"
    APPEND
    PROPERTY COMPILE_FLAGS "-Wno-self-assign-overloaded")
  set_property(SOURCE "bmqc/bmqc_monitoredqueue.cpp"
    APPEND
    PROPERTY COMPILE_FLAGS "-Wno-unused-macros")
  set_property(SOURCE "bmqu/bmqu_weakmemfn.t.cpp"
    APPEND
    PROPERTY COMPILE_FLAGS "-Wno-ignored-qualifiers")
endif()

set(BMQ_PRIVATE_PACKAGES
  bmqimp
  bmqp
  bmqeval
  bmqc
  bmqex
  bmqio
  bmqma
  bmqst
  bmqstm
  bmqsys
  bmqt
  bmqtsk
  bmqtst
  bmqu
)
target_bmq_style_uor( bmq PRIVATE_PACKAGES ${BMQ_PRIVATE_PACKAGES} )

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  # Search homebrewed keg-only versions of Bison and Flex on macOS.
  execute_process(
    COMMAND brew --prefix bison
    RESULT_VARIABLE HOMEBREW_BISON
    OUTPUT_VARIABLE HOMEBREW_BISON_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if (HOMEBREW_BISON EQUAL 0 AND EXISTS "${HOMEBREW_BISON_PREFIX}")
    message(STATUS "Using bison keg installed by Homebrew at ${HOMEBREW_BISON_PREFIX}")
    set(BISON_EXECUTABLE "${HOMEBREW_BISON_PREFIX}/bin/bison")
  endif()

  execute_process(
    COMMAND brew --prefix flex
    RESULT_VARIABLE HOMEBREW_FLEX
    OUTPUT_VARIABLE HOMEBREW_FLEX_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if (HOMEBREW_FLEX EQUAL 0 AND EXISTS "${HOMEBREW_FLEX_PREFIX}")
    message(STATUS "Using flex keg installed by Homebrew at ${HOMEBREW_FLEX_PREFIX}")
    set(FLEX_EXECUTABLE "${HOMEBREW_FLEX_PREFIX}/bin/flex")
  endif()
endif()

# Look for Flex and Bison IN THIS ORDER! Otherwise FLEX_INCLUDE_DIRS is set to NOT-FOUND
find_package(FLEX REQUIRED)
find_package(BISON 3 REQUIRED)

BISON_TARGET(SimpleEvaluatorParser
             bmqeval/bmqeval_simpleevaluatorparser.y
             ${CMAKE_CURRENT_BINARY_DIR}/bmqeval_simpleevaluatorparser.cpp)
FLEX_TARGET(SimpleEvaluatorScanner
            bmqeval/bmqeval_simpleevaluatorscanner.l
            ${CMAKE_CURRENT_BINARY_DIR}/bmqeval_simpleevaluatorscanner.cpp)
ADD_FLEX_BISON_DEPENDENCY(SimpleEvaluatorScanner SimpleEvaluatorParser)

target_sources(bmqeval-iface
               PRIVATE
               ${BISON_SimpleEvaluatorParser_OUTPUTS}
               ${FLEX_SimpleEvaluatorScanner_OUTPUTS})

target_include_directories(bmqeval-iface SYSTEM PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${FLEX_INCLUDE_DIRS})

target_include_directories(bmqeval_simpleevaluator.t
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(bmqeval_simpleevaluator.t PUBLIC bmq benchmark)

