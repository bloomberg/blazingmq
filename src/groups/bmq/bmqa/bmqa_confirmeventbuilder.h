// Copyright 2016-2023 Bloomberg Finance L.P.
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

// bmqa_confirmeventbuilder.h                                         -*-C++-*-
#ifndef INCLUDED_BMQA_CONFIRMEVENTBUILDER
#define INCLUDED_BMQA_CONFIRMEVENTBUILDER

/// @file bmqa_confirmeventbuilder.h
///
/// @brief Provide a builder for batching confirmation messages.
///
/// This component implements a mechanism, @bbref{bmqa::ConfirmEventBuilder},
/// that can be used for batching CONFIRM messages.  The resulting batch can be
/// sent to the BlazingMQ broker using the @bbref{bmqa::Session} (refer to @ref
/// bmqa_session.h for details).  Wherever possible, a BlazingMQ consumer
/// should try to send a batch of CONFIRM messages, which is more efficient
/// than confirming messages individually.
///
/// The builder holds a batch of CONFIRM messages under construction, and
/// provides two flavors of addMessageConfirmation method to add a CONFIRM
/// message to the batch.  It also provides a routine to retrieve number of
/// CONFIRM messages added to the batch.  Once application is done creating the
/// batch, it can retrieve the blob (wire-representation) of the batch and send
/// it via @bbref{bmqa::Session}.  See [the usage
/// section](#bmqa_confirmeventbuilder_usage) for more details.
///
/// Usage                                     {#bmqa_confirmeventbuilder_usage}
/// =====
///
///   - An instance of @bbref{bmqa::ConfirmEventBuilder} (the builder) can be
///     used to create multiple batches of CONFIRM messages.  The recommended
///     approach is to create one instance of the builder and use it throughout
///     the lifetime of the task (if the task is multi-threaded, an instance
///     per thread must be created and maintained).  See [usage example
///     1](#bmqa_confirmeventbuilder_ex1) for an illustration.
///
///   - The lifetime of an instance of the builder is bound by the
///     @bbref{bmqa::Session} from which it was created.  In other words,
///     @bbref{bmqa::Session} instance must outlive the builder instance.
///
/// Example 1 - Basic Usage                     {#bmqa_confirmeventbuilder_ex1}
/// -----------------------
///
/// ```
/// // In this snippet, we will send a batch of CONFIRMs for all the
/// // 'bmqa::Message' messages received in a 'bmqa::MessageEvent'.
///
/// // Note that error handling is omitted from the snippet for the sake of
/// // brevity.
///
/// bmqa::Session session;
///     // Session start up logic elided.
///
/// // Create and load an instance of the ConfirmEventBuilder.  Note that in
/// // this example, we are creating the builder on the stack everytime a
/// // message event is received.  Another approach can be to maintain the
/// // builder as a data member and use it everytime.
///
/// bmqa::ConfirmEventBuilder builder;
/// session.loadConfirmEventBuilder(&builder);
///
/// // Assuming that a 'bmqa::MessageEvent' is received.
///
/// bmqa::MessageIterator iter = messageEvent.messageIterator();
/// while (iter.nextMessage()) {
///     const bmqa::Message& msg = iter.message();
///
///     // Business logic for processing 'msg' elided.
///
///     int rc = builder.addMessageConfirmation(msg);
///         // Error handling elided.
/// }
///
/// // All messages in the event have been processed and their corresponding
/// // CONFIRM messages have been batched.  Now its time to send the batch to
/// // the BlazingMQ broker.
///
/// int rc = session.confirmMessages(&builder);
///     // Error handling elided.  Note that in case of success, above method
///     // will also reset the 'builder'.
/// ```
///
/// Thread Safety                            {#bmqa_confirmeventbuilder_thread}
/// =============
///
/// This component is *NOT* thread safe.  If it is desired to create a batch of
/// CONFIRM messages from multiple threads, an instance of the builder must be
/// created and maintained *per* *thread*.

// BMQ

#include <bmqa_message.h>
#include <bmqt_resultcode.h>

// BDE
#include <bdlbb_blob.h>
#include <bsls_alignedbuffer.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqp {
class ConfirmEventBuilder;
}

namespace bmqa {

// ==============================
// struct ConfirmEventBuilderImpl
// ==============================

/// Component-private class.  Do not use.
struct ConfirmEventBuilderImpl {
    // CONSTANTS

    /// Constant representing the maximum size of a
    /// `bmqp::ConfirmEventBuilder` object, so that the below AlignedBuffer
    /// is big enough.  Note that since this buffer will hold a
    /// `bmqp::ConfirmEventBuilder` which holds a `bdlbb::Blob` data member,
    /// the size is different on 32 vs 64 bits.
    static const int k_MAX_SIZEOF_BMQP_CONFIRMEVENTBUILDER = 64;

    // PUBLIC DATA
    //   (for convenience)
    bsls::AlignedBuffer<k_MAX_SIZEOF_BMQP_CONFIRMEVENTBUILDER> d_buffer;

    bmqp::ConfirmEventBuilder* d_builder_p;

    // CREATORS
    ConfirmEventBuilderImpl();

  private:
    // NOT IMPLEMENTED
    ConfirmEventBuilderImpl(const ConfirmEventBuilderImpl&);  // = delete
    ConfirmEventBuilderImpl&
    operator=(const ConfirmEventBuilderImpl&);  // = delete
};

// =========================
// class ConfirmEventBuilder
// =========================

/// Mechanism to build a batch of CONFIRM messages.
class ConfirmEventBuilder {
  private:
    // DATA
    ConfirmEventBuilderImpl d_impl;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator not implemented
    ConfirmEventBuilder(const ConfirmEventBuilder& other);  // = delete
    ConfirmEventBuilder&
    operator=(const ConfirmEventBuilder& rhs);  // = delete

  public:
    // CREATORS

    /// Create an invalid instance.  Application should not create
    /// `ConfirmEventBuilder` themselves, but ask the `bmqa::Session` to
    /// give them one, by using `bmqa::Session::loadConfirmEventBuilder`.
    ConfirmEventBuilder();

    /// Destroy this instance.
    ~ConfirmEventBuilder();

    // MANIPULATORS

    /// Append a confirmation message for the specified `message`.  Return
    /// zero on success, and a non-zero value otherwise.  Behavior is
    /// undefined unless this instance was obtained using
    /// `bmqa::Session::loadConfirmEventBuilder`.
    bmqt::EventBuilderResult::Enum
    addMessageConfirmation(const Message& message);

    /// Append a confirmation message for the specified
    /// MessageConfirmationCookie `cookie`.  Return zero on success, and a
    /// non-zero value otherwise.  Behavior is undefined unless this
    /// instance was obtained using
    /// `bmqa::Session::loadConfirmEventBuilder`.
    bmqt::EventBuilderResult::Enum
    addMessageConfirmation(const MessageConfirmationCookie& cookie);

    /// Reset the builder, effectively discarding the batch of confirmation
    /// messages under construction.
    void reset();

    // ACCESSORS

    /// Return the number of messages currently in the ConfirmEvent being
    /// built.  Behavior is undefined unless this instance was obtained
    /// using `bmqa::Session::loadConfirmEventBuilder`.
    int messageCount() const;

    /// Return a reference not offering modifiable access to the blob of
    /// confirmation messages batch built by this builder.  If no messages
    /// were added, this will return an empty blob, i.e., a blob with
    /// `length == 0`.  Behavior is undefined unless this instance was
    /// obtained using `bmqa::Session::loadConfirmEventBuilder`.
    const bdlbb::Blob& blob() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------------------
// struct ConfirmEventBuilderImpl
// ------------------------------

// CREATORS
inline ConfirmEventBuilderImpl::ConfirmEventBuilderImpl()
: d_buffer()
, d_builder_p(0)
{
    // NOTHING
}

// -------------------------
// class ConfirmEventBuilder
// -------------------------

// CREATORS
inline ConfirmEventBuilder::ConfirmEventBuilder()
: d_impl()
{
    // NOTHING
}

// MANIPULATORS
inline bmqt::EventBuilderResult::Enum
ConfirmEventBuilder::addMessageConfirmation(const Message& message)
{
    return addMessageConfirmation(message.confirmationCookie());
}

}  // close package namespace
}  // close enterprise namespace

#endif
