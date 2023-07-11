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
# not used in the task. On AIX, we must first prelink the libraries .a into an
# object .o (asking it to keep all symbols with -bexpfull), and then link that
# .o with the task.
#
# By keeping all symbols, the task will be quite bigger than it should be, but
# that way we guarantee that any symbols used by the plugins will be found.
# Because of that, depencies of bmqbrkr.tsk must be kept under close watch and
# to the bare minimum.

macro(_bmq_prelink_aix_modules)
  # Declare the task with compiler and linker flags
  # On AIX, we are going to generate a prelink .o file with all static
  # libraries, that needs to be added to the sources used to build the task
  # NOTE: When doing a fully fresh build, the .o doesn't yet exist and CMake
  #       will complain when adding the .o as a source to the broker. This
  #       file is generated as a PRE_LINK rule, so at this point, just make
  #       sure it exist.
  file( WRITE "${CMAKE_CURRENT_BINARY_DIR}/aix_prelink_libs.o" "" )
  target_sources( bmqbrkr PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/aix_prelink_libs.o" )

  # The AIX equivalent flags are "-bexpfull -bnogc" ${deps} "-bgc" where each
  # 'deps' is -bkeepfile:<full_path_to_lib.a>, but this doesn't quite seem to
  # work as expected, so instead we do what is call prelinking, asking the
  # loader to generate a .o with all the .a
  if (IS_64BIT)
    set(_bit_flag "-b64")
  else()
    set(_bit_flag "-b32")
  endif()

  # Generate the link parameters
  # NOTE: o bmqbrkr_DEPENDS contains all the declared .dep:
  #         ex 'mqb;bmq;bce;bde;bsl'
  #       o _ext_libs contains all the external (from the refRoot) libs
  #       o _prj_libs contains explicit path to all the libs from this project
  foreach( dep ${bmqbrkr_DEPENDS} )
    if( TARGET ${dep} )
      # It's a lib defined in one project, get the path to the output lib
      # And add the proper includes dirs
      list( APPEND _prj_libs "$<TARGET_FILE:${dep}>" )
      get_property( _include_dir TARGET ${dep} PROPERTY INCLUDE_DIRECTORIES )
      target_include_directories( bmqbrkr BEFORE INTERFACE ${_include_dir} )
    else()
      list( APPEND _ext_libs "$<TARGET_FILE:${dep}>" )
    endif()
  endforeach()

  set( cmd
    /bin/ld ${_bit_flag} -r -bbinder:/usr/ccs/bin/bind64
    "-L${libPath}" -bexpfull -bnogc ${_prj_libs} )
  foreach( lib ${_ext_libs} )
    list( APPEND cmd "${lib}" )
  endforeach()
  list( APPEND cmd "-o" "${CMAKE_CURRENT_BINARY_DIR}/aix_prelink_libs.o" )
  add_custom_command( TARGET bmqbrkr
    PRE_LINK
    COMMAND ${cmd}
    COMMENT "Prelinking ..." )
endmacro()

if(BMQ_TARGET_BMQBRKR_NEEDED)
  # Read dependencies from .dep and resolve them with pkgconfig
  bbs_read_metadata(PACKAGE bmqbrkr)

  add_executable(bmqbrkr ${bmqbrkr_MAIN_SOURCE} ${bmqbrkr_SOURCE_FILES})
  target_compile_definitions(bmqbrkr PRIVATE "MWC_INTERNAL_USAGE")
  target_include_directories(bmqbrkr
    BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  if(installNightly)
    set_target_properties(bmqbrkr
      PROPERTIES OUTPUT_NAME "bmqbrkr.nightly.tsk")
  else()
    set_target_properties(bmqbrkr
    PROPERTIES OUTPUT_NAME "bmqbrkr.tsk")
  endif()

  # "-btextpsize:64K -bdatapsize:64K" Memory tweaks.  See internal ticket
  # D46833503
  target_link_options(bmqbrkr PRIVATE
    $<$<PLATFORM_ID:AIX>:$<$<CXX_COMPILER_ID:XL>:-btextpsize:64K>>
    $<$<PLATFORM_ID:AIX>:$<$<CXX_COMPILER_ID:XL>:-bdatapsize:64K>>)

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
      # dynamic symbol table by default, and AIX seems to
      # do something similar:
      # https://developer.ibm.com/articles/au-aix-symbol-visibility/
      "-Wl,--whole-archive"
      ${bmqbrkr_DEPENDS}
      "-Wl,--no-whole-archive")
  elseif(CMAKE_SYSTEM_NAME MATCHES "SunOS")
    target_link_libraries(bmqbrkr PRIVATE
      "-Wl,--whole-archive"
      ${bmqbrkr_DEPENDS}
      "-Wl,--no-whole-archive")
  elseif(CMAKE_SYSTEM_NAME MATCHES "AIX")
    _bmq_prelink_aix_modules()
    # Keeps old symbol exporting behavior
    target_link_options(bmqbrkr PRIVATE -bsvr4)
    target_link_libraries(bmqbrkr PUBLIC
      ${bmqbrkr_DEPENDS})
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
