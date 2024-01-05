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

// bmqscm_versiontag.h                                                -*-C++-*-
#ifndef INCLUDED_BMQSCM_VERSIONTAG
#define INCLUDED_BMQSCM_VERSIONTAG

//@PURPOSE: Provide versioning information for the 'bmq' package group.
//
//@SEE_ALSO: bmqscm::Version
//
//@DESCRIPTION: This component provides versioning information for the 'bmq'
// package group.  The 'BMQ_VERSION' macro that is supplied can be used for
// conditional-compilation based on 'bmq' version information.  The following
// usage example illustrates this basic capability.
//
/// Usage
///-----
// At compile time, the version of BlazingMQ can be used to select an older or
// new way to accomplish a task, to enable new functionality, or to accommodate
// an interface change.  For example, if the name of a function changes (a rare
// occurrence, but potentially disruptive when it does happen), the impact on
// affected code can be minimized by conditionally calling the function by its
// old or new name using conditional compilation.  In the following, the '#if'
// preprocessor directive compares 'BMQ_VERSION' (i.e., the latest BlazingMQ
// version, excluding the patch version) to a specified major and minor version
// composed using the 'BDE_MAKE_VERSION' macro:
//..
//  #if BMQ_VERSION > BDE_MAKE_VERSION(1, 3)
//      // Call 'newFunction' for BlazingMQ versions later than 1.3.
//      int result = newFunction();
//  #else
//      // Call 'oldFunction' for BlazingMQ version 1.3 or earlier.
//      int result = oldFunction();
//  #endif
//..

#define BMQ_VERSION_MAJOR 99
// BlazingMQ release major version

#define BMQ_VERSION_MINOR 99
// BlazingMQ release minor version

#define BMQ_VERSION_PATCH 99
// BlazingMQ patch level

#define BMQ_MAKE_VERSION(major, minor) ((major) * 10000 + (minor) * 100)
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

#define BMQ_MAKE_EXT_VERSION(major, minor, patch)                             \
    ((major) * 10000 + (minor) * 100 + (patch))
// Similar to BMQ_MAKE_VERSION(), but include the patch number as well.

#define BMQ_VERSION BMQ_MAKE_VERSION(BMQ_VERSION_MAJOR, BMQ_VERSION_MINOR)
// Construct a composite version number in the range [ 0 .. 999900 ] from
// the specified 'BMQ_VERSION_MAJOR' and 'BMQ_VERSION_MINOR' numbers
// corresponding to the major and minor version numbers, respectively, of
// the current (latest) BlazingMQ release.  Note that the patch version
// number is intentionally not included.  For example, 'BMQ_VERSION'
// produces 10300 (decimal) for BlazingMQ version 1.3.1.

#define BMQ_EXT_VERSION                                                       \
    BMQ_MAKE_EXT_VERSION(BMQ_VERSION_MAJOR,                                   \
                         BMQ_VERSION_MINOR,                                   \
                         BMQ_VERSION_PATCH)
// Similar to BMQ_VERSION, but include the patch number as well

#endif
