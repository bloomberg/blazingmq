// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mwcst_statcontextuserdata.h                                        -*-C++-*-
#ifndef INCLUDED_MWCST_STATCONTEXTUSERDATA
#define INCLUDED_MWCST_STATCONTEXTUSERDATA

//@PURPOSE: Provide a protocol for user data associated with a 'StatContext'.
//
//@CLASSES:
// StatContextUserData: protocol for 'StatContext' user data
//
//@SEE_ALSO: mwcst_statcontext
//
//@DESCRIPTION: This component defines a pure protocol, 'StatContextUserData'
// which serves as the user data of a 'StatContext'.  It provides a single pure
// virtual function, 'snapshot' which is called by the associated 'StatContext'
// once it has completed making a new snapshot.  Typically, the 'snapshot'
// method is used take a snapshot of the concrete 'StatContextUserData's state
// to provide a consistent view to the stat processing thread that doesn't
// require any locking on its part, which may include reading data from the
// latest snapshot of its associated 'StatContext'.

namespace BloombergLP {
namespace mwcst {

// =========================
// class StatContextUserData
// =========================

/// Protocol for `mwcst::StatContext` user data
class StatContextUserData {
  public:
    // CREATORS

    /// Destroy this object.
    virtual ~StatContextUserData();

    // MANIPULATORS

    /// Notify this object that a new snapshot of its associated
    /// `StatContext` has completed, allowing this object to provide a
    /// consistent view of its state to the stat processing thread.  This
    /// method need not be thread safe, and should only be called from the
    /// stat processing thread via the `snapshot` method of the associated
    /// `StatContext`.
    virtual void snapshot() = 0;
};

}  // close package namespace
}  // close enterprise namespace

#endif
