// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqscm_version.h                                                   -*-C++-*-
#ifndef INCLUDED_BMQSCM_VERSION
#define INCLUDED_BMQSCM_VERSION

//@PURPOSE: Provide source control management (versioning) information.
//
//@CLASSES:
//  bmqscm::Version: namespace for SCM versioning information for 'bmq'
//
//@DESCRIPTION: This component provides source control management (versioning)
// information for the 'bmq' package group.  In particular, this component
// embeds RCS-style and SCCS-style version strings in binary executable files
// that use one or more components from the 'bmq' package group.  This version
// information may be extracted from binary files using common UNIX utilities
// (e.g., 'ident' and 'what').  In addition, the 'version' 'static' member
// function in the 'bmqscm::Version' struct can be used to query version
// information for the 'bmq' package group at runtime.  The following USAGE
// examples illustrate these two basic capabilities.
//
// Note that unless the 'version' method will be called, it is not necessary to
// "#include" this component header file to get 'bmq' version information
// embedded in an executable.  It is only necessary to use one or more 'bmq'
// components (and, hence, link in the 'bmq' library).
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
/// Example 1: Embedding Version Information
///  - - - - - - - - - - - - - - - - - - - -
// The version of the 'bmqscm' package group linked into a program can be
// obtained at runtime using the 'version' static member function as follows:
//..
//
//
//        assert(0 != bmqscm::Version::version());
//
//        bsl::cout << "bmqscm version: " << bmqscm::Version::version()
//                  << bsl::endl;
//..
// Output similar to the following will be printed to 'stdout':
//..
//        BMQ_SCM version: BLP_LIB_BMQ_SCM_0.01.0
//..
// The "0.01.0" portion of the string distinguishes different versions of the
// 'bmqscm' package group.
//
/// Example 2: Accessing the Embedded Version information
///- - - - - - - - - - - - - - - - - - - - - - - - - - -
// The versioning information embedded into a binary file by this component can
// be examined under UNIX using several well-known utilities.  For example:
//..
//        $ ident a.out
//        a.out:
//             $Id: BLP_LIB_BMQ_SCM_0.01.0 $
//
//        $ what a.out | grep BMQ_SCM
//                BLP_LIB_BMQ_SCM_0.01.0
//
//        $ strings a.out | grep BMQ_SCM
//        $Id: BLP_LIB_BMQ_SCM_0.01.0 $
//        @(#)BLP_LIB_BMQ_SCM_0.01.0
//        BLP_LIB_BMQ_SCM_0.01.0
//..
// Note that 'ident' and 'what' typically displays many version strings
// unrelated to 'bmqscm' depending on the libraries used by 'a.out'.

// BMQ
#include <bmqscm_versiontag.h>

// BDE
#include <bsls_linkcoercion.h>

namespace BloombergLP {
namespace bmqscm {

struct Version {
    // PUBLIC CLASS DATA
    static const char* s_ident;
    static const char* s_what;

#define BMQSCM_CONCAT2(a, b, c, d, e) a##b##c##d##e
#define BMQSCM_CONCAT(a, b, c, d, e) BMQSCM_CONCAT2(a, b, c, d, e)

// 'BMQSCM_S_VERSION' is a symbol whose name warns users of version mismatch
// linking errors.  Note that the exact string "compiled_this_object" must be
// present in this version coercion symbol.  Tools may look for this pattern to
// warn users of mismatches.
#define BMQSCM_S_VERSION                                                      \
    BMQSCM_CONCAT(d_version_BMQ_,                                             \
                  BMQ_VERSION_MAJOR,                                          \
                  _,                                                          \
                  BMQ_VERSION_MINOR,                                          \
                  _compiled_this_object)

    static const char* BMQSCM_S_VERSION;

    static const char* s_dependencies;
    static const char* s_buildInfo;
    static const char* s_timestamp;
    static const char* s_sourceControlInfo;

    // CLASS METHODS

    /// Return the formatted string corresponding to the version.  Format is
    /// BLP_LIB_BMQ_<major>.<minor>.<patch>
    static const char* version();

    /// Return the int corresponding to the version, using the following
    /// formula: `(major) * 10000 + (minor) * 100 + (patch)`
    static int versionAsInt();
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class bmqscm::Version
// ---------------------

inline const char* Version::version()
{
    return BMQSCM_S_VERSION;
}

inline int Version::versionAsInt()
{
    return BMQ_EXT_VERSION;
}

}  // close package namespace

BSLS_LINKCOERCION_FORCE_SYMBOL_DEPENDENCY(const char*,
                                          bmqscm_version_assertion,
                                          bmqscm::Version::BMQSCM_S_VERSION)

}  // close enterprise namespace

#endif
