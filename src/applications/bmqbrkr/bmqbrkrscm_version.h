// Copyright 2023 Bloomberg Finance L.P.
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

// bmqbrkrscm_version.h                                               -*-C++-*-
#ifndef INCLUDED_BMQBRKRSCM_VERSION
#define INCLUDED_BMQBRKRSCM_VERSION

//@PURPOSE: Provide source control management (versioning) information.
//
//@CLASSES:
//  bmqbrkrscm::Version: namespace for SCM versioning information for 'bmqbrkr'
//
//@DESCRIPTION: This component provides source control management (versioning)
// information for the 'bmqbrkr' package group.  In particular, this component
// embeds RCS-style and SCCS-style version strings in binary executable files
// that use one or more components from the 'bmqbrkr' package group.  This
// version information may be extracted from binary files using common UNIX
// utilities (e.g., 'ident' and 'what').  In addition, the 'version' 'static'
// member function in the 'bmqbrkrscm::Version' struct can be used to query
// version information for the 'bmqbrkr' package group at runtime.  The
// following USAGE examples illustrate these two basic capabilities.
//
// Note that unless the 'version' method will be called, it is not necessary to
// "#include" this component header file to get 'bmqbrkr' version information
// embedded in an executable.  It is only necessary to use one or more
// 'bmqbrkr' components (and, hence, link in the 'bmqbrkr' library).

#include <bmqbrkrscm_versiontag.h>

// BDE
#include <bsls_linkcoercion.h>

namespace BloombergLP {
namespace bmqbrkrscm {

struct Version {
    // PUBLIC CLASS DATA
    static const char* s_ident;
    static const char* s_what;

#define BMQBRKRSCM_CONCAT2(a, b, c, d, e) a##b##c##d##e
#define BMQBRKRSCM_CONCAT(a, b, c, d, e) BMQBRKRSCM_CONCAT2(a, b, c, d, e)

// 'BMQBRKRSCM_S_VERSION' is a symbol whose name warns users of version
// mismatch linking errors.  Note that the exact string "compiled_this_object"
// must be present in this version coercion symbol.  Tools may look for this
// pattern to warn users of mismatches.
#define BMQBRKRSCM_S_VERSION                                                  \
    BMQBRKRSCM_CONCAT(d_version_BMQBRKR,                                      \
                      BMQBRKR,                                                \
                      _,                                                      \
                      BMQBRKR,                                                \
                      _compiled_this_object)

    static const char* BMQBRKRSCM_S_VERSION;

    static const char* s_dependencies;
    static const char* s_buildInfo;
    static const char* s_timestamp;
    static const char* s_sourceControlInfo;

    // CLASS METHODS

    /// Return the formatted string corresponding to the version.  Format is
    /// BLP_LIB_BMQBRKR<major>.<minor>.<patch>
    static const char* version();

    /// Return the int corresponding to the version, using the following
    /// formula: `(major) * 10000 + (minor) * 100 + (patch)`
    static int versionAsInt();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------
// class bmqbrkrscm::Version
// -------------------------

inline const char* Version::version()
{
    return BMQBRKRSCM_S_VERSION;
}

inline int Version::versionAsInt()
{
    return BMQBRKR_EXT_VERSION;
}

}  // close package namespace

BSLS_LINKCOERCION_FORCE_SYMBOL_DEPENDENCY(
    const char*,
    bmqbrkrscm_version_assertion,
    bmqbrkrscm::Version::BMQBRKRSCM_S_VERSION)

}  // close enterprise namespace

#endif
