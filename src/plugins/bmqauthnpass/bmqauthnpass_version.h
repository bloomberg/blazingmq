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

#ifndef INCLUDED_AUTHNPASS_VERSION
#define INCLUDED_AUTHNPASS_VERSION

//@PURPOSE: Provide source control management (versioning) information.
//
//@CLASSES:
//  bmqauthnpass::Version: namespace for 'bmqauthnpass' SCM versioning
//  information
//
//@DESCRIPTION: This component provides source control management (versioning)
// information for the 'bmqauthnpass' plugin.  In particular, this component
// embeds RCS-style and SCCS-style version strings in binary executable files
// that use one or more components from the 'bmqauthnpass' plugin.  This
// version information may be extracted from binary files using common UNIX
// utilities (e.g., 'ident' and 'what').  In addition, the 'version' 'static'
// member function in the 'bmqauthnpass::Version' struct can be used to query
// version information for the 'bmqauthnpass' plugin at runtime.  The
// following USAGE examples illustrate these two basic capabilities.
//
// Note that unless the 'version' method will be called, it is not necessary to
// "#include" this component header file to get 'bmqauthnpass' version
// information embedded in an executable.  It is only necessary to use one or
// more 'bmqauthnpass' components (and, hence, link in the 'bmqauthnpass'
// library).

// AUTHNPASS
#include <bmqauthnpass_versiontag.h>

// BDE
#include <bsls_linkcoercion.h>

namespace BloombergLP {
namespace bmqauthnpass {

struct Version {
    // PUBLIC CLASS DATA
    static const char* s_ident;
    static const char* s_what;

#define AUTHNPASS_CONCAT2(a, b, c, d, e) a##b##c##d##e
#define AUTHNPASS_CONCAT(a, b, c, d, e) AUTHNPASS_CONCAT2(a, b, c, d, e)

// 'AUTHNPASS_S_VERSION' is a symbol whose name warns users of version
// mismatch linking errors.  Note that the exact string "compiled_this_object"
// must be present in this version coercion symbol.  Tools may look for this
// pattern to warn users of mismatches.
#define AUTHNPASS_S_VERSION                                                   \
    AUTHNPASS_CONCAT(d_version_AUTHNPASS_,                                    \
                     AUTHNPASS_VERSION_MAJOR,                                 \
                     _,                                                       \
                     AUTHNPASS_VERSION_MINOR,                                 \
                     _compiled_this_object)

    static const char* AUTHNPASS_S_VERSION;

    static const char* s_dependencies;
    static const char* s_buildInfo;
    static const char* s_timestamp;
    static const char* s_sourceControlInfo;

    // CLASS METHODS
    static const char* version();
    // Return the formatted string corresponding to the version. Format is
    // BLP_LIB_AUTHNPASS_<major>.<minor>.<patch>

    static int versionAsInt();
    // Return the int corresponding to the version, using the following
    // formula: '(major) * 10000 + (minor) * 100 + (patch)'
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------
// class bmqauthnpass::Version
// ---------------------------

inline const char* Version::version()
{
    return AUTHNPASS_S_VERSION;
}

inline int Version::versionAsInt()
{
    return AUTHNPASS_EXT_VERSION;
}

}  // close package namespace

}  // close enterprise namespace

#endif
