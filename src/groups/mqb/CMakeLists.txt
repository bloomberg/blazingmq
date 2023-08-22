# mqb
# ---

if (NOT BMQ_TARGET_MQB_NEEDED)
  return()
endif()

# Add the libmqb group library, since its a 'utility' library for bmqbrkr, it
# doesn't need to have any install rules.
add_library( mqb )
set_property( TARGET mqb PROPERTY POSITION_INDEPENDENT_CODE ON )

target_compile_definitions( mqb PRIVATE "MWC_INTERNAL_USAGE" )

# Pass build type to a source file so that it can print it in startup banner
set_property(SOURCE "mqba/mqba_application.cpp"
             APPEND
             PROPERTY COMPILE_DEFINITIONS "BMQ_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

set(MQB_PRIVATE_PACKAGES mqba mqbblp mqbc mqbcmd mqbconfm mqbi mqbmock mqbnet mqbs mqbsi mqbsl mqbu)
target_bmq_style_uor( mqb PRIVATE_PACKAGES ${MQB_PRIVATE_PACKAGES})

# Additional system required libraries:
#  o dlopen()/dlsym() require to link with '-ldl' on Linux
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_link_libraries( mqb PUBLIC dl )
endif()
