// Copyright 2021-2023 Bloomberg Finance L.P.
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

// bmqu_temputil.h                                                    -*-C++-*-
#ifndef INCLUDED_BMQU_TEMPUTIL
#define INCLUDED_BMQU_TEMPUTIL

//@PURPOSE: Provide an utility for resolving the effective temporary directory.
//
//@CLASSES:
//  bmqu::TempUtil: Utilities for temporary directories and files.
//
//@DESCRIPTION: This component provides an utility, 'bmqu::TempUtil', to
// resolve the effective temporary directory.  This directory is defined by one
// of a set of environment variables, overriding each other in a pre-defined
// order.  The definition of the default effective temporary directory is
// platform-specific.
//
/// Unix
///----
// On Unix platforms, the effective system temporary directory is defined first
// by the environment variable "TMPDIR", then falling back to the environment
// variables "TMP", then "TEMP", in that order.  If none of these environment
// variables is defined, the effective system temporary directory is assumed to
// be "/tmp".
//
/// Windows
///-------
// On Windows platforms, the effective system temporary directory is defined
// first by the environment variable "TMPDIR", then falling back to the
// environment variables "TMP", then "TEMP", then "USERPROFILE", in that order.
// If none of these environment variables is defined, the effective system
// temporary directory is assumed to be the Windows directory.
//
/// Thread Safety
///-------------
// This component is thread safe.

// BDE
#include <bsl_string.h>

namespace BloombergLP {
namespace bmqu {

// ===============
// struct TempUtil
// ===============

/// This struct provides utilities for implementing temporary directory and
/// file guards.  This struct is thread safe.
struct TempUtil {
    // CLASS METHODS

    /// Return the effective temporary directory defined for the user and
    /// the system.  The result is guaranteed to have a trailing path
    /// separator, even if the definition of the environment variable does
    /// not.
    static bsl::string tempDir();

    /// Return the default temporary directory when no environment variables
    /// are defined.  The result is guaranteed to have a trailing path
    /// separator.
    static bsl::string tempDirDefault();
};

}  // close package namespace
}  // close enterprise namespace

#endif
