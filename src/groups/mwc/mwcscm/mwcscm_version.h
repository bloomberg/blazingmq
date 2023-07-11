// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mwcscm_version.h                                                   -*-C++-*-
#ifndef INCLUDED_MWCSCM_VERSION
#define INCLUDED_MWCSCM_VERSION

#ifndef MWC_INTERNAL_USAGE
#error "MWC is a private library and its usage is restricted !"
// The 'mwc' library is an internal only library used by the middleware team,
// and it's usage is restricted and limited.  Please contact the middleware
// team if you are seeing this error.
//
// Do *NOT* ever define 'MWC_PRIVATE_USAGE' yourself.
//
// NOTE: Since 'mwcscm_version.h' is included by all headers of the 'mwc'
//       library, we only need to put that validation check here.
#endif

//@PURPOSE: Provide source control management (versioning) information.
//
//@CLASSES:
//  mwcscm::Version: namespace for SCM versioning information for 'mwc'
//
//@DESCRIPTION: This component provides source control management (versioning)
// information for the 'mwc' package group.  In particular, this component
// embeds RCS-style and SCCS-style version strings in binary executable files
// that use one or more components from the 'mwc' package group.  This version
// information may be extracted from binary files using common UNIX utilities
// (e.g., 'ident' and 'what').  In addition, the 'version' 'static' member
// function in the 'mwcscm::Version' struct can be used to query version
// information for the 'mwc' package group at runtime.  The following USAGE
// examples illustrate these two basic capabilities.
//
// Note that unless the 'version' method will be called, it is not necessary to
// "#include" this component header file to get 'mwc' version information
// embedded in an executable.  It is only necessary to use one or more 'mwc'
// components (and, hence, link in the 'mwc' library).

// MWC
#include <mwcscm_versiontag.h>

// BDE
#include <bsls_linkcoercion.h>

namespace BloombergLP {
namespace mwcscm {

struct Version {
    // PUBLIC CLASS DATA
    static const char* s_ident;
    static const char* s_what;

#define MWCSCM_CONCAT2(a, b, c, d, e) a##b##c##d##e
#define MWCSCM_CONCAT(a, b, c, d, e) MWCSCM_CONCAT2(a, b, c, d, e)

// 'MWCSCM_S_VERSION' is a symbol whose name warns users of version mismatch
// linking errors.  Note that the exact string "compiled_this_object" must be
// present in this version coercion symbol.  Tools may look for this pattern to
// warn users of mismatches.
#define MWCSCM_S_VERSION                                                      \
    MWCSCM_CONCAT(d_version_MWC_,                                             \
                  MWC_VERSION_MAJOR,                                          \
                  _,                                                          \
                  MWC_VERSION_MINOR,                                          \
                  _compiled_this_object)

    static const char* MWCSCM_S_VERSION;

    static const char* s_dependencies;
    static const char* s_buildInfo;
    static const char* s_timestamp;
    static const char* s_sourceControlInfo;

    // CLASS METHODS

    /// Return the formatted string corresponding to the version. Format is
    /// BLP_LIB_MWC_<major>.<minor>.<patch>
    static const char* version();

    /// Return the int corresponding to the version, using the following
    /// formula: `(major) * 10000 + (minor) * 100 + (patch)`
    static int versionAsInt();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class mwcscm::Version
// ---------------------

inline const char* Version::version()
{
    return MWCSCM_S_VERSION;
}

inline int Version::versionAsInt()
{
    return MWC_EXT_VERSION;
}

}  // close package namespace

BSLS_LINKCOERCION_FORCE_SYMBOL_DEPENDENCY(const char*,
                                          mwcscm_version_assertion,
                                          mwcscm::Version::MWCSCM_S_VERSION)

}  // close enterprise namespace

#endif
