# BMQDoxygen.cmake
#
# This module provides Doxygen documentation targets for BlazingMQ.
#
# It creates two documentation targets:
#   - apidocs:      Public API documentation for libbmq
#   - internaldocs: Internal documentation for libbmq and libmqb
#
# Both targets are added as dependencies to a top-level 'docs' target.

function(bmq_setup_doxygen)
  find_package(Doxygen)
  if(NOT DOXYGEN_FOUND)
    return()
  endif()

  add_custom_target(docs
    COMMENT "Build public and internal documentation"
  )

  # -----------------------------------------------------------------------------
  # API Documentation for libbmq
  # -----------------------------------------------------------------------------

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
  set(DOXYGEN_JAVADOC_AUTOBRIEF      YES)
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
  set(DOXYGEN_ALPHABETICAL_INDEX     NO)
  set(DOXYGEN_FULL_SIDEBAR           YES)
  set(DOXYGEN_GENERATE_TREEVIEW      YES)
  set(DOXYGEN_GENERATE_TAGFILE
    "${CMAKE_CURRENT_BINARY_DIR}/apidocs/tags.xml"
  )
  doxygen_add_docs(
    apidocs
    docs/mainpage.dox
    src/groups/bmq/bmqa
    src/groups/bmq/bmqt
    src/groups/bmq/bmqpi
    COMMENT "Generate public Doxygen documentation for libbmq"
  )
  add_dependencies(docs apidocs)

  # -----------------------------------------------------------------------------
  # Internal documentation for both libbmq and libmqb
  # -----------------------------------------------------------------------------

  unset(DOXYGEN_EXCLUDE_SYMBOLS)
  set(DOXYGEN_EXTRACT_PRIVATE        YES)
  set(DOXYGEN_SHOW_INCLUDE_FILES     YES)
  set(DOXYGEN_SHOW_NAMESPACES        YES)
  set(DOXYGEN_GENERATE_TODOLIST      YES)
  set(DOXYGEN_STRIP_FROM_PATH        src/groups)
  set(DOXYGEN_OUTPUT_DIRECTORY       internaldocs)
  set(DOXYGEN_GENERATE_TAGFILE
    "${CMAKE_CURRENT_BINARY_DIR}/internaldocs/tags.xml"
  )
  doxygen_add_docs(
    internaldocs
    src/groups/
    COMMENT "Generate internal Doxygen documentation for libbmq and libmqb"
  )
  add_dependencies(docs internaldocs)
endfunction()
