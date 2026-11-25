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

#ifndef INCLUDED_BMQP_ACKMESSAGEITERATOR
#define INCLUDED_BMQP_ACKMESSAGEITERATOR

//@PURPOSE: Provide a mechanism to iterate over the messages of an AckEvent.
//
//@CLASSES:
//  bmqp::AckMessageIterator: read-only sequential iter on 'AckEvent'.
//
//@DESCRIPTION: 'bmqp::AckMessageIterator' is an iterator-like mechanism
// providing read-only sequential access to messages contained into a AckEvent.
//
/// Error handling: Logging and Assertion
///-------------------------------------
//: o logging: This iterator will not log anything in case of invalid data:
//:   this is the caller's responsibility to check the return value of
//:   'isValid()' and/or 'next()' and take action (the 'dumpBlob()' method can
//:    be used to print some helpful information).
//: o assertion: When built in SAFE mode, the iterator will assert when
//:   inconsistencies between the blob and the headers are detected.
//
/// Usage
///-----
// Typical usage of this iterator should follow the following pattern:
//..
//  int rc = 0;
//  while ((rc = ackMessageIterator.next()) == 1) {
//    // Use ackMessageIterator accessors, such as ..
//    int status = ackMessageIterator.message().status();
//  }
//  if (rc < 0) {
//    // Invalid AckMessage Event
//    BALL_LOG_ERROR_BLOCK {
//        BALL_LOG_OUTPUT_STREAM << "Invalid 'AckMessageEvent' [rc: " << rc
//                               << "]\n";
//        ackMessageIterator.dumpBlob(BALL_LOG_OUTPUT_STREAM);
//    }
//  }
//..

// BMQ

#include <bmqp_fixedsizemessageiterator.h>
#include <bmqp_protocol.h>

// BDE
#include <bdlbb_blob.h>

namespace BloombergLP {
namespace bmqp {

// ========================
// class AckMessageIterator
// ========================

/// An iterator providing read-only sequential access to messages contained
/// in a `AckEvent`.  See `bmqp_fixedsizemessageiterator` for the iteration
/// and error-handling contract.
class AckMessageIterator
: public FixedSizeMessageIterator<AckHeader, AckMessage> {
  private:
    // PRIVATE TYPES
    typedef FixedSizeMessageIterator<AckHeader, AckMessage> Base;

  public:
    // CREATORS

    /// Create an invalid instance.  Only valid operations on an invalid
    /// instance are assignment, `reset` and `isValid`.
    AckMessageIterator();

    /// Initialize a new instance using the specified `blob` and
    /// `eventHeader`.  Behavior is undefined if the `blob` pointer is null,
    /// or the pointed-to blob does not contain enough bytes to fit at least
    /// the `eventHeader`.
    AckMessageIterator(const bdlbb::Blob* blob,
                       const EventHeader& eventHeader);

    // The compiler-generated copy constructor and copy-assignment operator
    // forward to 'Base', which copies the 'BlobObjectProxy' members.
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------
// class AckMessageIterator
// ------------------------

// CREATORS
inline AckMessageIterator::AckMessageIterator()
: Base()
{
    // NOTHING
}

inline AckMessageIterator::AckMessageIterator(const bdlbb::Blob* blob,
                                              const EventHeader& eventHeader)
: Base(blob, eventHeader)
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace

#endif
