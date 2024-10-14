// Copyright 2020-2023 Bloomberg Finance L.P.
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

// bmqu_samethreadchecker.h                                           -*-C++-*-
#ifndef INCLUDED_BMQU_SAMETHREADCHECKER
#define INCLUDED_BMQU_SAMETHREADCHECKER

//@PURPOSE: Provide a mechanism to check if a call is performed in same thread.
//
//@CLASSES:
//  SameThreadChecker: the checker mechanism
//
//@DESCRIPTION:
// This component provides a mechanism, 'bmqu::SameThreadChecker', to check if
// a function call is always performed in the same thread.
//
/// Thread safety
///-------------
// 'bmqu::SameThreadChecker' is fully thread-safe, meaning that multiple
// threads may use their own instances of the class or use a shared instance
// without further synchronization.
//
/// Usage
///-----
// Lets say there is a 'MessageProvider' protocol, that takes a message
// callback and guarantees that it will always be invoked in the same thread.
// We are implementing a 'MessageProcessor' class that relies on the
// 'MessageProvider' to supply messages.  Based on the threading guarantees
// provided by the message provider, and the fact that our data is only
// accessed from the message callback, we decide not to use any additional
// synchronization.  However, we would like to play it safe, and assert if the
// provider breaks the guarantees.  Here is how it can be done using this
// component.
//
//..
//  class MessageProvider {
//      // Provides a usage example message provider protocol.
//
//    public:
//      // TYPES
//      typedef bsl::function<void(int message)> MessageCallback;
//
//    public:
//      // MANIPULATORS
//      virtual
//      void
//      registerMessageCallback(const MessageCallback& messageCallback) = 0;
//          // Register the specified 'messageCallback' to be invoked from the
//          // IO thread when a message is available.
//  };
//
//  class MessageProcessor {
//      // Provides a usage example message processor.
//
//    private:
//      // PRIVATE DATA
//      MessageProvider         *d_messageProvider_p;
//          // Used to read messages.
//
//      bmqu::SameThreadChecker  d_ioThreadChecker;
//          // Used to check is the message callback is always invoked from the
//          // same thread.
//
//      int                      d_numMessagesReceived;
//          // Number of messages received.
//
//    private:
//      // PRIVATE MANIPULATORS
//      void onMessage(int message);
//          // Callback invoked by the message provider to notify this object a
//          // message is available.
//          //
//          // Note that this callback is invoked from the provider's IO
//          // thread.
//
//    private:
//      // NOT IMPLEMENTED
//      MessageProcessor(const MessageProcessor&)         BSLS_KEYWORD_DELETED;
//      MessageProcessor operator=(const MessageProcessor&)
//                                                        BSLS_KEYWORD_DELETED;
//
//    public:
//      // CREATORS
//      explicit
//      MessageProcessor(MessageProvider *messageProvider);
//          // Create a 'MessageProcessor' object. Specify a 'messageProvider'
//          // to supply messages.
//
//    public:
//      // MANIPULATORS
//      void start();
//          // Start processing messages.
//
//    private:
//      // NOT IMPLEMENTED
//      MessageProcessor(const MessageProcessor&) BSLS_KEYWORD_DELETED;
//      MessageProcessor operator=(const MessageProcessor&)
//      BSLS_KEYWORD_DELETED;
//
//    public:
//      // CREATORS
//      explicit
//      MessageProcessor(MessageProvider *messageProvider);
//          // Create a 'MessageProcessor' object. Specify a 'messageProvider'
//          to
//          // supply messages.
//
//    public:
//      // MANIPULATORS
//      void start();
//          // Start processing messages.
//  };
//
//  // PRIVATE MANIPULATORS
//  void
//  MessageProcessor::onMessage(int message)
//  {
//      // check this callback is always invoked from the same thread
//      BSLS_ASSERT(d_ioThreadChecker.inSameThread());
//
//      d_numMessagesReceived += message;
//      bsl::cout << d_numMessagesReceived << " messages received"
//                << bsl::endl;
//  }
//
//  // CREATORS
//  MessageProcessor::MessageProcessor(MessageProvider *messageProvider)
//  : d_messageProvider_p(messageProvider)
//  , d_ioThreadChecker()
//  , d_numMessagesReceived(0)
//  {
//      // NOTHING
//  }

//  // MANIPULATORS
//  void
//  MessageProcessor::start()
//  {
//      // subscribe to messages
//      d_messageProvider_p->registerMessageCallback(
//                 bdlf::MemFnUtil::memFn(&MessageProcessor::onMessage, this));
//  }
//..

// BDE
#include <bsls_atomic.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace bmqu {

// =======================
// class SameThreadChecker
// =======================

/// A mechanism to check if a function call is always performed in the same
/// thread.
class SameThreadChecker {
  private:
    // PRIVATE DATA

    // Id of the specific thread.
    bsls::AtomicUint64 d_threadId;

  private:
    // NOT IMPLEMENTED
    SameThreadChecker(const SameThreadChecker&) BSLS_KEYWORD_DELETED;
    SameThreadChecker operator=(const SameThreadChecker&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `SameThreadChecker` object.
    SameThreadChecker() BSLS_KEYWORD_NOEXCEPT;

  public:
    // MANIPULATORS

    /// If this function is called for the first time, store the id of
    /// the calling thread and return `true`.  Otherwise, return `true` if
    /// the id of the calling thread matches the previously stored id, and
    /// `false` otherwise.
    bool inSameThread() BSLS_KEYWORD_NOEXCEPT;

    /// Reset this object to a default-constructed state.
    void reset() BSLS_KEYWORD_NOEXCEPT;
};

}  // close package namespace
}  // close enterprise namespace

#endif
