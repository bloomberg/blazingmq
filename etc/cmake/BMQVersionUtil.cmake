## BMQVersionUtil.cmake
## ~~~~~~~~~~~~~~~~~~~~
#
## OVERVIEW
## --------
#
# This module provides utility functions for parsing version information from
# files.
#
## USAGE EXAMPLE
## -------------
#
#..
#:  get_bmqbrkr_version( brkrVers )
#:  message( "BMQBrkr version: ${brkrVers_MAJOR}.${brkrVers.minor}.${brkrVers.path}" )
#:
#:  get_bmqbrkrcfg_version( cfgVers )
#:  message( "BMQBrkrcfg version: ${cfgVers_MAJOR}.${cfgVers.minor}.${cfgVers.path}" )
#..
#

include_guard()

# :::::::::::::::::::::::::::::::::::::::::::::::::::::: GET_BMQBRKR_VERSION ::
# Parse the MQB scm file, and return in the specified '${output}_MAJOR',
# '${output}_MINOR' and '${output}_PATCH' the major, minor and patch portions
# of the version, respectively.
function(GET_BMQBRKR_VERSION output)
  file(READ
       "${PROJECT_SOURCE_DIR}/src/groups/mqb/mqbscm/mqbscm_versiontag.h"
       _content)

  # Extract the lines of interest for each part of the version information
  string(REGEX MATCH "#define MQB_VERSION_MAJOR [0-9]+" majorLine ${_content})
  string(REGEX MATCH "#define MQB_VERSION_MINOR [0-9]+" minorLine ${_content})
  string(REGEX MATCH "#define MQB_VERSION_PATCH [0-9]+" patchLine ${_content})

  # Strip out to keep only the version value
  string(REGEX REPLACE ".*_MAJOR " "" _major ${majorLine})
  string(REGEX REPLACE ".*_MINOR " "" _minor ${minorLine})
  string(REGEX REPLACE ".*_PATCH " "" _patch ${patchLine})

  set(${output}_MAJOR ${_major} PARENT_SCOPE)
  set(${output}_MINOR ${_minor} PARENT_SCOPE)
  set(${output}_PATCH ${_patch} PARENT_SCOPE)
endfunction()

# ::::::::::::::::::::::::::::::::::::::::::::::::::: GET_BMQBRKRCFG_VERSION ::
# Parse the 'bmqbrkr.cfg' file and return in the specified '${output}_MAJOR',
# '${output}_MINOR' and '${output}_PATCH' the respective major, minor and patch
# portions of the version; as extracted from the corresponding "configVersion"
# in the file.
function( GET_BMQBRKRCFG_VERSION output )
  file(READ ${PROJECT_SOURCE_DIR}/src/applications/bmqbrkr/bmqbrkr.cfg _content)

  # Extract the line of interest, and then the version from it
  string(REGEX MATCH   "k_CFG_VERSION = ([0-9]+)"      versionLine ${_content})
  string(REGEX REPLACE "k_CFG_VERSION *([0-9]+)" "\\1" version     ${versionLine})

  # Compute the version parts
  math(EXPR major "${version} / 10000")
  math(EXPR minor "(${version} - ${major} * 10000) / 100")
  math(EXPR patch "${version} % 100")

  # Populate the returned values
  set(${output}_MAJOR ${major} PARENT_SCOPE)
  set(${output}_MINOR ${minor} PARENT_SCOPE)
  set(${output}_PATCH ${patch} PARENT_SCOPE)
endfunction()
