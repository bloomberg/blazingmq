// Copyright 2016-2025 Bloomberg Finance L.P.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDED_AUTHNFAIL_VERSION
#define INCLUDED_AUTHNFAIL_VERSION

//@PURPOSE: Provide source control management (versioning) information.
//
//@CLASSES:
//  bmqauthnfail::Version: namespace for 'bmqauthnfail' SCM versioning
//  information
//
//@DESCRIPTION: This component provides source control management (versioning)
// information for the 'bmqauthnfail' plugin.  In particular, this component
// embeds RCS-style and SCCS-style version strings in binary executable files
// that use one or more components from the 'bmqauthnfail' plugin.  This
// version information may be extracted from binary files using common UNIX
// utilities (e.g., 'ident' and 'what').  In addition, the 'version' 'static'
// member function in the 'bmqauthnfail::Version' struct can be used to query
// version information for the 'bmqauthnfail' plugin at runtime.  The
// following USAGE examples illustrate these two basic capabilities.
//
// Note that unless the 'version' method will be called, it is not necessary to
// "#include" this component header file to get 'bmqauthnfail' version
// information embedded in an executable.  It is only necessary to use one or
// more 'bmqauthnfail' components (and, hence, link in the 'bmqauthnfail'
// library).

// AUTHNFAIL
#include <bmqauthnfail_versiontag.h>

// BDE
#include <bsls_linkcoercion.h>

namespace BloombergLP {
namespace bmqauthnfail {

struct Version {
    // PUBLIC CLASS DATA
    static const char* s_ident;
    static const char* s_what;

#define AUTHNFAIL_CONCAT2(a, b, c, d, e) a##b##c##d##e
#define AUTHNFAIL_CONCAT(a, b, c, d, e) AUTHNFAIL_CONCAT2(a, b, c, d, e)

// 'AUTHNFAIL_S_VERSION' is a symbol whose name warns users of version
// mismatch linking errors.  Note that the exact string "compiled_this_object"
// must be present in this version coercion symbol.  Tools may look for this
// pattern to warn users of mismatches.
#define AUTHNFAIL_S_VERSION                                                   \
    AUTHNFAIL_CONCAT(d_version_AUTHNFAIL_,                                    \
                     AUTHNFAIL_VERSION_MAJOR,                                 \
                     _,                                                       \
                     AUTHNFAIL_VERSION_MINOR,                                 \
                     _compiled_this_object)

    static const char* AUTHNFAIL_S_VERSION;

    static const char* s_dependencies;
    static const char* s_buildInfo;
    static const char* s_timestamp;
    static const char* s_sourceControlInfo;

    // CLASS METHODS
    static const char* version();
    // Return the formatted string corresponding to the version. Format is
    // BLP_LIB_AUTHNFAIL_<major>.<minor>.<patch>

    static int versionAsInt();
    // Return the int corresponding to the version, using the following
    // formula: '(major) * 10000 + (minor) * 100 + (patch)'
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------
// class bmqauthnfail::Version
// ---------------------------

inline const char* Version::version()
{
    return AUTHNFAIL_S_VERSION;
}

inline int Version::versionAsInt()
{
    return AUTHNFAIL_EXT_VERSION;
}

}  // close package namespace

}  // close enterprise namespace

#endif
