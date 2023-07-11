# tutorials
# ---------

if (NOT BMQ_TARGET_TUTORIAL_NEEDED)
  return()
endif()

# :: BMQ_DECLARE_TUTORIAL :::::::::::::::::::::::::::::::::::::::::::::::::::::
# All-in-one function to setup make rule for a tutorial 'name'.  Each .cpp file
# in the current directory will be built as a task with the same name as its
# source file.  A rule with the tutorial name is added to build all tasks it
# contains.  And a 'tutorials' rule is added to build all tutorials.
#
function(BMQ_ADD_TUTORIAL name)
  # Pick a 'unique' tutorial name by prefixing it with 'tut_'
  project("tut_${name}" CXX)

  # Retrieve list of all .cpp files, and create a task for each of them
  file(GLOB sources "*.cpp")
  foreach (source ${sources})
    get_filename_component(taskName "${source}" NAME_WE)

    # Make sure to have a unique target name by concatenating the tutorial name
    # and the task name
    set(targetName "${name}_${taskName}")

    # Add the executable
    add_executable(${targetName} EXCLUDE_FROM_ALL "${source}")

    # Add './' to include paths
    target_include_directories(${targetName}
                               BEFORE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    # Add all compile definitions used for building 'bmq'
    target_compile_options(${targetName} PRIVATE
                           "$<TARGET_PROPERTY:bmq,COMPILE_OPTIONS>")

    # Link with 'bmq'
    target_link_libraries(${targetName} PRIVATE "bmq")

    ## Change the name of the generated output task from
    ## "<tutorialName>_<taskName>" to "<taskName>.tsk"
    set_target_properties(${targetName}
                          PROPERTIES OUTPUT_NAME "${taskName}.tsk")

    # Keep track of all targets to add the 'tutorial' rule at the end
    list(APPEND targets ${targetName})
  endforeach()

  # Add the 'tutorial' rule to build all tasks
  add_custom_target(${name} DEPENDS ${targets})

  # Add the current tutorial to the 'tutorials' rule
  add_dependencies("tutorials" ${name})
endfunction()


## ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
## ADD THE TUTORIALS
## ~~~~~~~~~~~~~~~~~

# Create a custom target 'tutorials' that builds all tutorials
add_custom_target("tutorials")

# Add each tutorial
add_subdirectory("advanced")
add_subdirectory("hello_world")
add_subdirectory("subscriptions")
