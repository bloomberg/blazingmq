// plugins_version.h                                                -*-C++-*-
#ifndef INCLUDED_PLUGINS_VERSION
#define INCLUDED_PLUGINS_VERSION

//@PURPOSE: Provide source control management (versioning) information.
//
//@CLASSES:
//  plugins::Version: namespace for 'plugins' SCM versioning information
//
//@DESCRIPTION: This component provides source control management (versioning)
// information for the 'plugins' plugin.  In particular, this component
// embeds RCS-style and SCCS-style version strings in binary executable files
// that use one or more components from the 'plugins' plugin.  This version
// information may be extracted from binary files using common UNIX utilities
// (e.g., 'ident' and 'what').  In addition, the 'version' 'static' member
// function in the 'plugins::Version' struct can be used to query version
// information for the 'plugins' plugin at runtime.  The following USAGE
// examples illustrate these two basic capabilities.
//
// Note that unless the 'version' method will be called, it is not necessary to
// "#include" this component header file to get 'plugins' version information
// embedded in an executable.  It is only necessary to use one or more
// 'plugins' components (and, hence, link in the 'plugins' library).

// PLUGINS
#include <plugins_versiontag.h>

// BDE
#include <bsls_linkcoercion.h>


namespace BloombergLP {
namespace plugins {

struct Version {
    // PUBLIC CLASS DATA
    static const char *s_ident;
    static const char *s_what;

#define PLUGINS_CONCAT2(a,b,c,d,e) a ## b ## c ## d ## e
#define PLUGINS_CONCAT(a,b,c,d,e)  PLUGINS_CONCAT2(a,b,c,d,e)

// 'PLUGINS_S_VERSION' is a symbol whose name warns users of version mismatch
// linking errors.  Note that the exact string "compiled_this_object" must be
// present in this version coercion symbol.  Tools may look for this pattern to
// warn users of mismatches.
#define PLUGINS_S_VERSION PLUGINS_CONCAT(d_version_PLUGINS_,     \
                                             PLUGINS_VERSION_MAJOR,  \
                                              _,                       \
                                             PLUGINS_VERSION_MINOR,  \
                                             _compiled_this_object)

    static const char *PLUGINS_S_VERSION;

    static const char *s_dependencies;
    static const char *s_buildInfo;
    static const char *s_timestamp;
    static const char *s_sourceControlInfo;

    // CLASS METHODS
    static const char *version();
        // Return the formatted string corresponding to the version. Format is
        // BLP_LIB_PLUGINS_<major>.<minor>.<patch>

    static int versionAsInt();
        // Return the int corresponding to the version, using the following
        // formula: '(major) * 10000 + (minor) * 100 + (patch)'
};


// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

                           // ------------------------
                           // class plugins::Version
                           // ------------------------

inline
const char*
Version::version()
{
    return PLUGINS_S_VERSION;
}

inline
int
Version::versionAsInt()
{
    return PLUGINS_EXT_VERSION;
}

}  // close package namespace


BSLS_LINKCOERCION_FORCE_SYMBOL_DEPENDENCY(
                                       const char *,
                                       plugins_version_assertion,
                                       plugins::Version::PLUGINS_S_VERSION)

}  // close enterprise namespace

#endif

// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2023
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ------------------------------- END-OF-FILE --------------------------------
