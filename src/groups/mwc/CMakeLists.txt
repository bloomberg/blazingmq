# mwc
# ---

if(NOT BMQ_TARGET_MWC_NEEDED)
  return()
endif()

add_library(mwc)

target_compile_definitions(mwc PRIVATE "MWC_INTERNAL_USAGE")
target_compile_definitions(mwc INTERFACE "MWC_INTERNAL_USAGE")

# Adding it to 'interface' so that mwc test drivers can compile; but sadly
# this makes it so that any target linking about this 'mwc' target will
# also implicitly inherit from that flag, which is ok since it's privately
# build, but not ideal.
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|(Apple)?Clang")
  # NOTE: The followings are using the 'deprecated' COMPILE_FLAGS property and
  # not COMPILE_OPTIONS because the later doesn't seem to work for a
  # 'SOURCE' target.
  set_property(SOURCE "mwcc/test/mwcc_array.t.cpp"
    APPEND
    PROPERTY COMPILE_FLAGS "-Wno-self-assign-overloaded")
  set_property(SOURCE "mwcc/mwcc_monitoredqueue.cpp"
    APPEND
    PROPERTY COMPILE_FLAGS "-Wno-unused-macros")
  set_property(SOURCE "mwcu/test/mwcu_weakmemfn.t.cpp"
    APPEND
    PROPERTY COMPILE_FLAGS "-Wno-ignored-qualifiers")
endif()

target_bmq_style_uor(mwc)
