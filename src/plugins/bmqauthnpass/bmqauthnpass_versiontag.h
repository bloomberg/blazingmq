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

#ifndef INCLUDED_AUTHNPASS_VERSIONTAG
#define INCLUDED_AUTHNPASS_VERSIONTAG

//@PURPOSE: Provide versioning information for the 'bmqauthnpass' plugin.
//
//@SEE_ALSO: bmqauthnpass::Version
//
//@DESCRIPTION: This component provides versioning information for the
// 'bmqauthnpass' plugin.  The 'AUTHNPASS_VERSION' macro that is supplied can
// be used for conditional-compilation based on 'bmqauthnpass' version
// information. The following usage example illustrates this basic capability.
//
/// Usage
///-----
// At compile time, the version of AUTHNPASS can be used to select an older or
// newer way to accomplish a task, to enable new functionality, or to
// accommodate an interface change.  For example, if the name of a function
// changes (a rare occurrence, but potentially disruptive when it does happen),
// the impact on affected code can be minimized by conditionally calling the
// function by its old or new name using conditional compilation.  In the
// following, the '#if' preprocessor directive compares 'AUTHNPASS_VERSION'
// (i.e., the latest AUTHNPASS version, excluding the patch version) to a
// specified major and minor version composed using the 'BDE_MAKE_VERSION'
// macro:
//..
//  #if AUTHNPASS_VERSION > BDE_MAKE_VERSION(1, 3)
//      // Call 'newFunction' for AUTHNPASS versions later than 1.3.
//      int result = newFunction();
//  #else
//      // Call 'oldFunction' for AUTHNPASS version 1.3 or earlier.
//      int result = oldFunction();
//  #endif
//..

/// AUTHNPASS release major version
#define AUTHNPASS_VERSION_MAJOR 99

/// AUTHNPASS release minor version
#define AUTHNPASS_VERSION_MINOR 99

/// AUTHNPASS patch level
#define AUTHNPASS_VERSION_PATCH 99

/// Construct a composite version number in the range [ 0 .. 999900 ] from
/// the specified 'major' and 'minor' version numbers.  The resulting value,
/// when expressed as a 6-digit decimal string, has "00" as the two
/// lowest-order decimal digits, 'minor' as the next two digits, and 'major'
/// as the highest-order digits.  The result is unique for each combination
/// of 'major' and 'minor', and is sortable such that a value composed from
/// a given 'major' version number will compare larger than a value composed
/// from a smaller 'major' version number (and similarly for 'minor' version
/// numbers).  Note that if 'major' and 'minor' are both compile-time
/// integral constants, then the resulting expression is also a compile-time
/// integral constant.  Also note that the patch version number is
/// intentionally not included.  The behavior is undefined unless 'major'
/// and 'minor' are integral values in the range '[ 0 .. 99 ]'.
#define AUTHNPASS_MAKE_VERSION(major, minor) ((major) * 10000 + (minor) * 100)

/// Similar to AUTHNPASS_MAKE_VERSION(), but include patch number as well.
#define AUTHNPASS_MAKE_EXT_VERSION(major, minor, patch)                       \
    ((major) * 10000 + (minor) * 100 + (patch))

/// Construct a composite version number in the range [ 0 .. 999900 ] from
/// the specified 'AUTHNPASS_VERSION_MAJOR' and 'AUTHNPASS_VERSION_MINOR'
/// numbers corresponding to the major and minor version numbers,
/// respectively, of the current (latest) AUTHNPASS release.  Note that the
/// patch version number is intentionally not included.  For example,
/// 'AUTHNPASS_VERSION' produces 10300 (decimal) for AUTHNPASS version
/// 1.3.1.
#define AUTHNPASS_VERSION                                                     \
    AUTHNPASS_MAKE_VERSION(AUTHNPASS_VERSION_MAJOR, AUTHNPASS_VERSION_MINOR)

/// Similar to AUTHNPASS_VERSION, but include the patch number as well
#define AUTHNPASS_EXT_VERSION                                                 \
    AUTHNPASS_MAKE_EXT_VERSION(AUTHNPASS_VERSION_MAJOR,                       \
                               AUTHNPASS_VERSION_MINOR,                       \
                               AUTHNPASS_VERSION_PATCH)

#endif
