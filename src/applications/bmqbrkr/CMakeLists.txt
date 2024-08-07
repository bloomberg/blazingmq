# Build Explanation
# -----------------

# bmqbrkr uses an architecture of plugins in shared libraries, which are
# dynamically loaded by bmqbrkr.tsk at startup based on configuration. Those
# plugins typically may have vendor or Bloomberg dependencies, and this is why
# they are taken out from the core 'bmqbrkr.tsk', in order to keep its
# dependencies to the bare minimum, while still providing a single package for
# the BlazingMQ infrastructure.
#
# Shared libraries at Bloomberg are not so much recommended, and BDE only
# provides static versions of their libraries, with some of them (bsl
# Allocator, ball LogManager, ...) using static global variables.  Therefore,
# it is not recommended (or even not possible) to have the task and the plugins
# link statically with those libraries; this would cause, at best, crashes at
# exit.  One solution would be to link bmqbrkr.tsk AND the plugins all against
# shared version of the libraries, but BDE/ROBO doesn't provide shared
# libraries.
#
# The strategy adopted here is to build the bmqbrkr.tsk statically linking into
# it all of the dependencies declared in the .dep (and don't strip out any
# unused symbol from them); and then building the plugins without linking to
# any of them (but linking to its own very specific dependencies); i.e. leaving
# all of BDE symbols undefined. That way, when the bqmbrkr.tsk opens the
# plugin, the linker, at runtime, will resolve all those symbols to the ones
# provided by the task.
#
# On Linux and SunOS, this is achieved by using some linker flags
# (-whole-archive, -z allextract, ...), which indicates to the linker that it
# should keep all symbols from the libraries mentioned after, even if those are
# not used in the task.
#
# By keeping all symbols, the task will be quite bigger than it should be, but
# that way we guarantee that any symbols used by the plugins will be found.
# Because of that, depencies of bmqbrkr.tsk must be kept under close watch and
# to the bare minimum.

if(BMQ_TARGET_BMQBRKR_NEEDED)
  # Read dependencies from .dep and resolve them with pkgconfig
  bbs_read_metadata(PACKAGE bmqbrkr)

  add_executable(bmqbrkr ${bmqbrkr_MAIN_SOURCE} ${bmqbrkr_SOURCE_FILES})
  target_include_directories(bmqbrkr
    BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  if(installNightly)
    set_target_properties(bmqbrkr
      PROPERTIES OUTPUT_NAME "bmqbrkr.nightly.tsk")
  else()
    set_target_properties(bmqbrkr
    PROPERTIES OUTPUT_NAME "bmqbrkr.tsk")
  endif()

  set_target_properties(bmqbrkr
    PROPERTIES ENABLE_EXPORTS ON)

  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # TODO: target_link_libraries( bmqbrkr PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,${bmqbrkr_DEPENDS}>")
    target_link_libraries(bmqbrkr PRIVATE

      # Exports symbols from 'bmqbrkr.tsk' such that they
      # can be loaded and called by code from plugins.
      # Without this flag, functions from plugins could
      # not, for instance, find and utilize BDE functions
      # (resulting in a runtime linkage failure).
      #
      # Note that Solaris publishes all symbols to the
      # dynamic symbol table by default.
      "-Wl,--whole-archive"
      ${bmqbrkr_DEPENDS}
      "-Wl,--no-whole-archive")
  elseif(CMAKE_SYSTEM_NAME MATCHES "SunOS")
    target_link_libraries(bmqbrkr PRIVATE
      "-Wl,--whole-archive"
      ${bmqbrkr_DEPENDS}
      "-Wl,--no-whole-archive")
  elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(bmqbrkr PRIVATE "-all_load" ${bmqbrkr_DEPENDS})
  endif()

  target_bmq_default_compiler_flags(bmqbrkr)

  # Fix malloc locks for multithreaded app.  See internal ticket D34187030.
  target_link_libraries(bmqbrkr PRIVATE $<$<PLATFORM_ID:SunOS>:$<$<CXX_COMPILER_ID:SunPro>:umem>>)
  bbs_add_target_bde_flags(bmqbrkr PUBLIC)
  bbs_add_target_thread_flags(bmqbrkr PUBLIC)
endif() # BMQ_TARGET_BMQBRKR_NEEDED

# DPKG and install rules
# ======================

# BMQBRKR install rules
bbproject_check_install_target("BMQBRKR" installBmqbrkr)

if(installBmqbrkr)
  install(TARGETS bmqbrkr RUNTIME DESTINATION "bin" COMPONENT bmqbrkr)
endif()

if(BMQ_TARGET_BMQBRKR_NEEDED)
  # So that we can easily locally start the broker
  if(NOT ${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    add_custom_command(
      TARGET bmqbrkr POST_BUILD
      COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/etc ${CMAKE_CURRENT_BINARY_DIR}/etc
      COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/run ${CMAKE_CURRENT_BINARY_DIR}/run)
endif()
endif()
