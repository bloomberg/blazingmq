// Copyright 2018-2023 Bloomberg Finance L.P.
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

// bmqp_controlmessageutil.h                                          -*-C++-*-
#ifndef INCLUDED_BMQP_CONTROLMESSAGEUTIL
#define INCLUDED_BMQP_CONTROLMESSAGEUTIL

//@PURPOSE: Provide utilities related to control messages.
//
//@CLASSES:
//  bmqp::ControlMessageUtil: utilities related to control messages.
//
//@DESCRIPTION: 'bmqp::ControlMessageUtil' provide utilities related to control
// messages.

// BMQ

#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_string.h>

namespace BloombergLP {
namespace bmqp {

// =========================
// struct ControlMessageUtil
// =========================

/// Provide utilities related to control messages.
struct ControlMessageUtil {
    // CLASS METHODS

    /// Load into the specified `controlMessage` a status message having the
    /// specified `category`, `code`, and `message`.
    static void makeStatus(bmqp_ctrlmsg::ControlMessage*       controlMessage,
                           bmqp_ctrlmsg::StatusCategory::Value category,
                           int                                 code,
                           const bslstl::StringRef&            message);

    /// Return 0 if the specified `controlMessage` is well formatted and a
    /// non-zero error code otherwise.
    static int validate(const bmqp_ctrlmsg::ControlMessage& controlMessage);
};

}  // close package namespace
}  // close enterprise namespace

#endif
