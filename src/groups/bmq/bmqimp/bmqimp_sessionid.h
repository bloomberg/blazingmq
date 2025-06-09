// Copyright 2025 Bloomberg Finance L.P.
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

// bmqimp_sessionid.h                                             -*-C++-*-
#ifndef INCLUDED_BMQIMP_SESSIONID
#define INCLUDED_BMQIMP_SESSIONID

//@PURPOSE: Provide the type for logging Application / Session instance number
//
//
//@CLASSES:
//  bmqimp::SessionId: Cache the instance number
//
//@DESCRIPTION: Overload operator<< as a single point for logging the number.
//
/// Thread Safety
///-------------
// Thread safe.

#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bslim_printer.h>

namespace BloombergLP {

namespace bmqimp {

struct SessionId {
    // VST to hold the instance number (and more if needed)

    const int d_sessionId;

    SessionId();

    SessionId(const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage);
};

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
inline bsl::ostream& operator<<(bsl::ostream&            stream,
                                const bmqimp::SessionId& rhs)
{
    if (rhs.d_sessionId) {
        stream << "[#" << rhs.d_sessionId << "] ";
    }

    return stream;
}

inline SessionId::SessionId()
: d_sessionId(0)
{
    // NOTHING
}

inline SessionId::SessionId(
    const bmqp_ctrlmsg::NegotiationMessage& negotiationMessage)
: d_sessionId(negotiationMessage.isClientIdentityValue()
                  ? negotiationMessage.clientIdentity().sessionId()
                  : 0)
{
    // NOTHING
}

}  // close package namespace

}  // close enterprise namespace

#endif
