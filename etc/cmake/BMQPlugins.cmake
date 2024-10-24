include_guard()

macro(_bmq_add_include_paths target)
    cmake_parse_arguments(
        ""
        ""
        "TARGET"
        "DEPS"
        ${ARGN}
    )

    foreach(linkopt ${_DEPS})
        target_include_directories(${target} PRIVATE $<TARGET_PROPERTY:${linkopt},INTERFACE_INCLUDE_DIRECTORIES>)
    endforeach()
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

    # Add './' to #include-paths.
    target_include_directories(${name} BEFORE PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    # Configure link-time options.
    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    _bmq_add_include_paths(${name} DEPS mqb bmq)
    target_link_libraries(${name} PRIVATE ${${name}_DEPENDS})

    # include( BMQTest )
    # add_bmq_test( ${name} )

    # Output the shared object into the same directory as 'bmqbrkr.tsk'.
    set_target_properties(
        ${name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/plugins")

    include(TargetBMQStyleUor)
    target_bmq_default_compiler_flags(${name})

    bbs_import_target_dependencies(${name} ${${name}_PCDEPS})

    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    # DPKG/install rules
    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    install(TARGETS ${name} COMPONENT ${name} LIBRARY DESTINATION "data/bmq/plugins")
endfunction()
