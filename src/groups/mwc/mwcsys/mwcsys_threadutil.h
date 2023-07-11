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

// mwcsys_threadutil.h                                                -*-C++-*-
#ifndef INCLUDED_MWCSYS_THREADUTIL
#define INCLUDED_MWCSYS_THREADUTIL

//@PURPOSE: Provide utilities related to thread management.
//
//@CLASSES:
//  mwcsys::ThreadUtil: utilities related to thread management.
//
//@DESCRIPTION: 'mwcsys::ThreadUtil' provide a utility namespace for operations
// related to thread management, such as naming threads.  Each operation may be
// platform specific, please refer to the associated function documentation for
// individual support explanation.
//
/// NOTE
///----
// It looks like, in the event of a crash, the generated core name takes the
// currently active thread name instead of the process name, so to make it
// easier to locate the matching core, each thread name we set should start
// with a small recognizable prefix ('BMQ').

// MWC

// BDE
#include <bsl_string.h>
#include <bslmt_threadattributes.h>

namespace BloombergLP {
namespace mwcsys {

// =================
// struct ThreadUtil
// =================

/// Utility namespace for thread management
struct ThreadUtil {
    // CONSTANTS

    /// Boolean constant indicating whether the current platform supports
    /// naming thread.
    static const bool k_SUPPORT_THREAD_NAME;

    // CLASS METHODS

    /// Return `bslmt::ThreadAttributes` object pre-initialized with default
    /// thread parameter values set for the local operating system.
    static bslmt::ThreadAttributes defaultAttributes();

    /// Set the name of the current thread to the specified `value`.  This
    /// method is a no-op if `k_SUPPORT_THREAD_NAME` is false.
    ///
    /// PLATFORM NOTE:
    ///   - this functionality is only supported on LINUX, and the name can
    ///     be up to 15 characters.
    static void setCurrentThreadName(const bsl::string& value);

    /// Set the name of the current thread to the specified `value`.  This
    /// method is a no-op if `k_SUPPORT_THREAD_NAME` is false.  Unlike
    /// `setCurrentThreadName`, this method uses a thread local variable to
    /// ensure this is done only once per thread.
    ///
    /// PLATFORM NOTE:
    ///   - this functionality is only supported on LINUX, and the name can
    ///     be up to 15 characters.
    static void setCurrentThreadNameOnce(const bsl::string& value);
};

}  // close package namespace
}  // close enterprise namespace

#endif
