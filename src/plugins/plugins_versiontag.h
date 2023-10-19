// plugins_versiontag.h                                             -*-C++-*-
#ifndef INCLUDED_PLUGINS_VERSIONTAG
#define INCLUDED_PLUGINS_VERSIONTAG

//@PURPOSE: Provide versioning information for the 'plugins' plugin.
//
//@SEE_ALSO: plugins::Version
//
//@DESCRIPTION: This component provides versioning information for the
// 'plugins' plugin.  The 'PLUGINS_VERSION' macro that is supplied can be
// used for conditional-compilation based on 'plugins' version information.
// The following usage example illustrates this basic capability.
//
/// Usage
///-----
// At compile time, the version of PLUGINS can be used to select an older or
// newer way to accomplish a task, to enable new functionality, or to
// accommodate an interface change.  For example, if the name of a function
// changes (a rare occurrence, but potentially disruptive when it does happen),
// the impact on affected code can be minimized by conditionally calling the
// function by its old or new name using conditional compilation.  In the
// following, the '#if' preprocessor directive compares 'PLUGINS_VERSION'
// (i.e., the latest PLUGINS version, excluding the patch version) to a
// specified major and minor version composed using the 'BDE_MAKE_VERSION'
// macro:
//..
//  #if PLUGINS_VERSION > BDE_MAKE_VERSION(1, 3)
//      // Call 'newFunction' for PLUGINS versions later than 1.3.
//      int result = newFunction();
//  #else
//      // Call 'oldFunction' for PLUGINS version 1.3 or earlier.
//      int result = oldFunction();
//  #endif
//..

#define PLUGINS_VERSION_MAJOR 99
// PLUGINS release major version

#define PLUGINS_VERSION_MINOR 99
// PLUGINS release minor version

#define PLUGINS_VERSION_PATCH 99
// PLUGINS patch level

#define PLUGINS_MAKE_VERSION(major, minor) ((major)*10000 + (minor)*100)
// Construct a composite version number in the range [ 0 .. 999900 ] from
// the specified 'major' and 'minor' version numbers.  The resulting value,
// when expressed as a 6-digit decimal string, has "00" as the two
// lowest-order decimal digits, 'minor' as the next two digits, and 'major'
// as the highest-order digits.  The result is unique for each combination
// of 'major' and 'minor', and is sortable such that a value composed from
// a given 'major' version number will compare larger than a value composed
// from a smaller 'major' version number (and similarly for 'minor' version
// numbers).  Note that if 'major' and 'minor' are both compile-time
// integral constants, then the resulting expression is also a compile-time
// integral constant.  Also note that the patch version number is
// intentionally not included.  The behavior is undefined unless 'major'
// and 'minor' are integral values in the range '[ 0 .. 99 ]'.

#define PLUGINS_MAKE_EXT_VERSION(major, minor, patch)                         \
    ((major)*10000 + (minor)*100 + (patch))
// Similar to PLUGINS_MAKE_VERSION(), but include patch number as well.

#define PLUGINS_VERSION                                                       \
    PLUGINS_MAKE_VERSION(PLUGINS_VERSION_MAJOR, PLUGINS_VERSION_MINOR)
// Construct a composite version number in the range [ 0 .. 999900 ] from
// the specified 'PLUGINS_VERSION_MAJOR' and 'PLUGINS_VERSION_MINOR'
// numbers corresponding to the major and minor version numbers,
// respectively, of the current (latest) PLUGINS release.  Note that the
// patch version number is intentionally not included.  For example,
// 'PLUGINS_VERSION' produces 10300 (decimal) for PLUGINS version
// 1.3.1.

#define PLUGINS_EXT_VERSION                                                   \
    PLUGINS_MAKE_EXT_VERSION(PLUGINS_VERSION_MAJOR,                           \
                             PLUGINS_VERSION_MINOR,                           \
                             PLUGINS_VERSION_PATCH)
// Similar to PLUGINS_VERSION, but include the patch number as well

#endif

// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2023
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ----------------------------------
