include_guard()

# :: BBPROJECT_CHECK_INSTALL_TARGET :::::::::::::::::::::::::::::::::::::::::::
# This function sets the specified 'out' to 'yes' or 'no' whether the specified
# 'target' is in the install list (i.e. the 'INSTALL_TARGETS' list variable).
# o If 'INSTALL_TARGETS' is not defined, this function assumes everything is to
#   be installed and so sets the return value to 'yes' (this is because
#   typically this variable is set in the debian file, and left empty for
#   development, where we want all targets);
# o otherwise, it will set it to 'yes' if 'target' is found in the
#   'INSTALL_TARGETS' else to 'no'.
#
function( BBPROJECT_CHECK_INSTALL_TARGET target out )
  # INSTALL_TARGETS can be all uppercase; make sure 'target' is
  # uppercase as well so that the string match will work
  string( TOUPPER ${target} upperTarget )

  if( NOT DEFINED INSTALL_TARGETS )
    set( ${out} "YES" PARENT_SCOPE )
  else()
    string( TOUPPER "${INSTALL_TARGETS}" UPPER_INSTALL_TARGETS )
    list( FIND UPPER_INSTALL_TARGETS ${upperTarget} inTarget )
    if( inTarget EQUAL -1 )
      set( ${out} "NO" PARENT_SCOPE )
    else()
      set( ${out} "YES" PARENT_SCOPE )
    endif()
  endif()
endfunction()

# Configure compiler options for a specific target for a specific combination
# of languages for the specified compiler.
#
# target_configure_compile_options(target
#   COMPILER_ID XL
#   CONFIGS Release RelWithDebInfo
#   LANGUAGES C CXX
#   OPTIONS -qfuncsect
# )
function( target_configure_compile_options target )
  cmake_parse_arguments( PARSE_ARGV 1 CCO "" "COMPILER_ID" "CONFIGS;LANGUAGES;OPTIONS" )
  if( NOT DEFINED CCO_COMPILER_ID )
    message( FATAL_ERROR "COMPILER_ID is required" )
    return()
  endif()
  if( NOT DEFINED CCO_LANGUAGES )
    # Add Fortran support once CMake supports `Fortran_COMPILER_ID`
    set( CCO_LANGUAGES C CXX )
  endif()
  if( NOT DEFINED CCO_OPTIONS )
    message( FATAL_ERROR "OPTIONS are required" )
    return()
  endif()
  if( DEFINED CCO_CONFIGS )
    foreach( config IN LISTS CCO_CONFIGS )
    foreach( lang IN LISTS CCO_LANGUAGES )
      target_compile_options( ${target} PRIVATE $<$<AND:$<CONFIG:${config}>,$<COMPILE_LANGUAGE:${lang}>,$<${lang}_COMPILER_ID:${CCO_COMPILER_ID}>>:${CCO_OPTIONS}> )
    endforeach()
    endforeach()
  else()
    # When explicit configurations are not specified, remove the condition
    # so that the configured options apply to all configurations, even
    # non-standard ones supplied by alternate toolchains.
    foreach( lang IN LISTS CCO_LANGUAGES )
      target_compile_options( ${target} PRIVATE $<$<AND:$<COMPILE_LANGUAGE:${lang}>,$<${lang}_COMPILER_ID:${CCO_COMPILER_ID}>>:${CCO_OPTIONS}> )
    endforeach()
  endif()
endfunction()

