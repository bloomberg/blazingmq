include_guard()

macro(_bmq_dedup_link_flags target)
    cmake_parse_arguments(
        ""
        ""
        "TARGET"
        "DEPS"
        ${ARGN}
    )

    message(DEBUG "Resolved target ${target} deps: ${${target}_DEPENDS}")
    message(DEBUG "Culling depends from ${target}: ${_DEPS}")

    bbs_import_target_dependencies(${target} ${${target}_PCDEPS})

    foreach(linkopt ${${target}_DEPENDS})
        list(FIND _DEPS "${linkopt}" idx)

        if(${idx} GREATER -1)
            if("${linkopt}" MATCHES "^-l" OR "${linkopt}" MATCHES "^[a-z]*$")
                # Remove any link-flags like "mqb", "bmq", etc.; or any flags
                # that are "-l<library>"; if they are already linked by
                # 'bmqbrkr.tsk'.
                list(REMOVE_ITEM ${target}_DEPENDS "${linkopt}")
                target_include_directories(${target} PRIVATE $<TARGET_PROPERTY:${linkopt},INTERFACE_INCLUDE_DIRECTORIES>)
            endif()
        endif()
    endforeach()

    message(DEBUG "Culled depends from ${target} deps: ${${target}_DEPENDS}")
    list(REMOVE_DUPLICATES ${target}_DEPENDS)
endmacro()

# :: bmq_add_plugin :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
# All-in-one function to setup a plugin 'name'.
function(bmq_add_plugin name)
    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    # Build the list of plugin source-files, based on the '.mem' file.
    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    find_package(BdeBuildSystem REQUIRED)

    bbs_read_metadata(PACKAGE ${name})

    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    # Declare the plugin library, and configure compile-time options.
    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    # Declare the plugin library.
    add_library(${name} MODULE ${${name}_SOURCE_FILES})

    # Give plugins access to MWC.
    target_compile_definitions(${name} PRIVATE "MWC_INTERNAL_USAGE")

    # Add './' to #include-paths.
    target_include_directories(${name} BEFORE PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    # Configure link-time options.
    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    _bmq_dedup_link_flags(${name} DEPS bal bsl mqb bmq mwc)
    target_link_libraries(${name} PRIVATE ${${name}_DEPENDS})

    # include( BMQTest )
    # add_bmq_test( ${name} )

    # Output the shared object into the same directory as 'bmqbrkr.tsk'.
    set_target_properties(
        ${name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/plugins")

    include(TargetBMQStyleUor)
    target_bmq_default_compiler_flags(${name})
endfunction()
