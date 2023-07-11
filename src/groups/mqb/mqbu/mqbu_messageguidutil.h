// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbu_messageguidutil.h                                             -*-C++-*-
#ifndef INCLUDED_MQBU_MESSAGEGUIDUTIL
#define INCLUDED_MQBU_MESSAGEGUIDUTIL

//@PURPOSE: Provide a utility component for bmqt::MessageGUID.
//
//@CLASSES:
//  mqbu::MessageGUIDUtil : Utility methods for bmqt::MessageGUID
//
//@SEE_ALSO:
// bmqt::MessageGUID
//
//@DESCRIPTION: 'mqbu::MessageGUIDUtil' provide a method to generate
// 'bmqt::MessageGUID'.
//

// MQB

// BMQ
#include <bmqt_messageguid.h>

// BDE
#include <ball_log.h>
#include <bsl_iosfwd.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace mqbu {

// =====================
// class MessageGUIDUtil
// =====================

/// This class provides a utility component to generate bmqt::MessageGUIDs.
class MessageGUIDUtil {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBU.MESSAGEGUIDUTIL");

  public:
    // CLASS METHODS

    /// Perform a one-time initialization before generating GUIDs.  It is
    /// undefined behavior to call this method more than once during an
    /// application's lifetime.  It is undefined behavior to call
    /// `generateGUID()` unless this method has been called once before.
    /// Note that this method is not thread-safe.
    static void initialize();

    /// Generate a new MessageGUID. This method can be called simultaneously
    /// from multiple threads.  Behavior is undefined unless `initialize`
    /// has been called once before this method's invocation.  Behavior is
    /// undefined unless specified `guid` is non-null.
    static void generateGUID(bmqt::MessageGUID* guid);

    /// Return the hexadecimal representation of the unique id associated to
    /// this broker (refer to the `Implementation notes` section in the
    /// associated .cpp file for details about how it is computed).  The
    /// behavior is undefined unless `initialize()` has been called.
    static const char* brokerIdHex();

    /// --------------------------
    /// For testing/debugging only
    /// --------------------------

    /// Load into the specified `version`, `counter`, `timerTick` and
    /// `brokerId` the corresponding fields from the specified `guid`.  Note
    /// that `brokerId` will be loaded in hex format.
    static void extractFields(int*                     version,
                              unsigned int*            counter,
                              bsls::Types::Int64*      timerTick,
                              bsl::string*             brokerId,
                              const bmqt::MessageGUID& guid);

    /// Print internal details of the specified `guid` to the specified
    /// `stream` in a human-readable format.  This method is for
    /// testing/debugging only.
    static bsl::ostream& print(bsl::ostream&            stream,
                               const bmqt::MessageGUID& guid);
};

}  // close package namespace
}  // close enterprise namespace

#endif
