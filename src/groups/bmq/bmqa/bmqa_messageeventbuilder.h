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

// bmqa_messageeventbuilder.h                                         -*-C++-*-
#ifndef INCLUDED_BMQA_MESSAGEEVENTBUILDER
#define INCLUDED_BMQA_MESSAGEEVENTBUILDER

// Clang-format warns about an overlong line in this comment, which gives a
// Markdown anchor to a header.  Unfortunately, by Markdown syntax rules, this
// has to on the same line as the header, meaning we cannot introduce a
// line-break here.

// clang-format off

/// @file bmqa_messageeventbuilder.h
///
/// @brief Provide a builder for @bbref{bmqa::MessageEvent} objects.
///
/// This component implements a mechanism, @bbref{bmqa::MessageEventBuilder},
/// that can be used for creating message events containing one or multiple
/// messages.  The resulting MessageEvent can be sent to the BlazingMQ broker
/// using the @bbref{bmqa::Session} (refer to @ref bmqa_session.h for details).
///
/// The builder holds a @bbref{bmqa::MessageEvent} under construction, and
/// provides methods to return a reference to the current message (in order to
/// set its various members), as well as to append a new message.  Once the
/// application is done, the builder provides a method to get the message event
/// that has been constructed.  See @ref bmqa_messageeventbuilder_usage section
/// for more details.
///
/// Note that publishing events containing multiple messages may be more
/// efficient than events limited to a single message for applications that
/// send large volume of small messages.
///
/// Usage                                     {#bmqa_messageeventbuilder_usage}
/// =====
///
///   - An instance of @bbref{bmqa::MessageEventBuilder} (the builder) can be
///     used to create multiple @bbref{bmqa::MessageEvent}'s, as long as the
///     instance is reset in between.  This reset is preferably to do right
///     after sending the event to guarantee that all user resources bound to
///     the @bbref{bmqa::MessageEvent} (e.g.  CorrelationIds with user's
///     shared_ptr) are kept no longer than expected (e.g. until related ACK
///     event is received).  The recommended approach is to create one instance
///     of the builder and use that throughout the lifetime of the task (if the
///     task is multi-threaded, an instance per thread must be created and
///     maintained).  See [usage example 1](#bmqa_messageeventbuilder_ex1) for
///     an illustration.
///
///   - The lifetime of an instance of the builder is bound by the
///     @bbref{bmqa::Session} from which it was created.  In other words,
///     @bbref{bmqa::Session} instance must outlive the builder instance.
///
///   - If it is desired to post the same @bbref{bmqa::Message} to different
///     queues, @bbref{bmqa::MessageEventBuilder::packMessage} can be called
///     multiple times in a row with different queue IDs.  The builder will
///     append the previously packed message with the new queue ID to the
///     underlying message event.  Note that after calling
///     @bbref{bmqa::MessageEventBuilder::packMessage}, the message keeps all
///     the attributes (payload, properties, etc) that were previously set
///     (except for the `correlationId` which must be set explicitly for each
///     individual message).  If desired, any attribute can be tweaked before
///     being packing the message again.  Refer to [usage example
///     2](#bmqa_messageeventbuilder_ex2) for an illustration.
///
/// Example 1 - Basic Usage                     {#bmqa_messageeventbuilder_ex1}
/// -----------------------
///
/// ```
/// // Note that error handling is omitted below for the sake of brevity
///
/// bmqa::Session session;
///     // Session start up logic omitted for brevity.
///
/// // Obtain a valid instance of message properties.
/// bmqa::MessageProperties properties;
/// session.loadMessageProperties(&properties);
///
/// // Set common properties that will be applicable to all messages sent by
/// // this application.
/// int rc = properties.setPropertyAsChar(
///                                 "encoding",
///                                 static_cast<char>(MyEncodingEnum::e_BER));
///
/// rc = properties.setPropertyAsString("producerId", "MyUniqueId");
///
/// // Obtain a valid instance of message event builder.
/// bmqa::MessageEventBuilder builder;
/// session.loadMessageEventBuilder(&builder);
///
/// // Create and post a message event containing 1 message.  Associate
/// // properties with this message.
/// bmqa::Message& msg = builder.startMessage();
///
/// msg.setCorrelationId(myCorrelationId);
///
/// // Set payload (where 'myPayload' is of type 'bdlbb::Blob')
/// msg.setDataRef(&myPayload);
///
/// // Set current timestamp as one of the properties.
/// rc = properties.setPropertyAsInt64(
///              "timestamp",
///              bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::now()));
///
/// // Set properties.
/// msg.setPropertiesRef(&properties);
///
/// // Pack the message.
/// rc = builder.packMessage(myQueueId);
///
/// // Post message event
/// rc = session.post(builder.messageEvent());
///
///
/// // Create and post another message event containing 1 message.
///
/// // bmqa::MessageEventBuilder must be reset before reuse.
/// builder.reset();
///
/// // Start a new message.
/// bmqa::Message& msg = builder.startMessage();
///
/// msg.setCorrelationId(myAnotherCorrelationId);
/// msg.setDataRef(&myAnotherPayload);
///
/// // It's okay (and recommended) to use same properties instance.
/// rc = properties.setPropertyAsInt64(
///              "timestamp",
///              bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::now()));
///
/// msg.setPropertiesRef(&properties);
/// rc = builder.packMessage(myAnotherQueueId);
///
/// // Post second message event
/// rc = session.post(builder.messageEvent());
///
/// // Reset the builder to free resources earlier.
/// builder.reset();
/// ```
///
/// Example 2 - Packing multiple messages in a message event  {#bmqa_messageeventbuilder_ex2}
/// --------------------------------------------------------
///
/// ```
/// // Note that error handling is omitted below for the sake of brevity
///
/// bmqa::Session session;
/// // Session start up logic omitted for brevity.
///
/// // Obtain a valid instance of message properties.
/// bmqa::MessageProperties properties;
/// session.loadMessageProperties(&properties);
///
/// // Set common properties that will be applicable to all messages sent by
/// // this application.
/// int rc = properties.setPropertyAsChar(
///                                 "encoding",
///                                 static_cast<char>(MyEncodingEnum::e_BER));
///
/// rc = properties.setPropertyAsString("producerId", "MyUniqueId");
///
/// // Obtain a valid instance of message event builder.
/// bmqa::MessageEventBuilder builder;
/// session.loadMessageEventBuilder(&builder);
///
/// // Create and post a message event containing 4 messages.
/// bmqa::Message& msg = builder.startMessage();
///
/// // Pack message #1
/// msg.setCorrelationId(correlationId1);
/// msg.setDataRef(&payload1);  // where 'payload1' is of type 'bdlbb::Blob'
///
/// // Set current timestamp as one of the properties.
/// int rc = properties.setPropertyAsInt64(
///              "timestamp",
///              bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::now()));
///
/// // Pack the message.
/// rc = builder.packMessage(queueId1);
///
/// // Pack message #2
/// // We want to send message #1 to another queue.  In order to do so, we
/// // just update the correlation ID of message #1.  There is no need to set
/// // the payload or properties again.  Because 'payload1' and 'properties'
/// // objects are being reused for the second message, they should not be
/// // destroyed before packing the second message.
///
/// msg.setCorrelationId(correlationId2);
///
/// // Also note that the "timestamp" property for the second message will be
/// // updated for this message.  There is no need to invoke
/// // 'setPropertiesRef' on the message though.
/// rc = properties.setPropertyAsInt64(
///              "timestamp",
///              bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::now()));
///
/// rc = builder.packMessage(queueId2);
///
/// // 'payload1' can be safely destroyed at this point if it will not be
/// // reused again for another message.
///
/// // Pack message #3
/// // Message #3 has a different payload, no properties and destined to
/// // 'queueId1'.
/// msg.setCorrelationId(correlationId3);
/// msg.setDataRef(&payload2);  // where 'payload2' is of type 'bdlbb::Blob'
///
/// // We need to explicitly clear out the associated properties.
/// msg.clearProperties();
///
/// rc = builder.packMessage(queueId1);
///
/// // Pack message #4
/// // Message #4 has different payload and destined to 'queueId3'.
/// msg.setCorrelationId(correlationId4);
/// msg.setDataRef(&payload3);  // where 'payload3' is of type 'bdlbb::Blob'
///
/// // Update "timestamp" property.
/// rc = properties.setPropertyAsInt64(
///              "timestamp",
///              bdlt::EpochUtil::convertToTimeT64(bdlt::CurrentTime::now()));
///
/// // Need to associate properties with the message, since they were cleared
/// // out while packing message #3 above.
/// msg.setPropertiesRef(&properties);
///
/// rc = builder.packMessage(queueId3);
///
/// // Post second message event
/// rc = session.post(builder.messageEvent());
///
/// // Reset the builder to free resources earlier.
/// builder.reset();
/// ```
///
/// Thread Safety                            {#bmqa_messageeventbuilder_thread}
/// =============
///
/// This component is *NOT* thread safe.

// clang-format on

// BMQ

#include <bmqa_message.h>
#include <bmqa_messageevent.h>
#include <bmqp_messageproperties.h>
#include <bmqpi_dtcontext.h>
#include <bmqpi_dttracer.h>

// BDE
#include <bsl_memory.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqa {
class QueueId;
}
namespace bmqp {
class MessageGUIDGenerator;
class PutEventBuilder;
class MessageProperties;
}

namespace bmqa {

// ==============================
// struct MessageEventBuilderImpl
// ==============================

/// @private
/// Struct containing the internal (private) members of MessageEventBuilder
/// (That is so that bmqa::Session::loadMessageEventBuilder can access
/// private members of MessageEventBuilder to initialize it, without having
/// to expose them publicly).
struct MessageEventBuilderImpl {
    // PUBLIC DATA

    /// This is needed so that `getMessageEvent()` can return a const ref.
    MessageEvent d_msgEvent;

    /// This is needed so that `startMessage()` can return a ref.
    Message d_msg;

    // GUID generator object.
    bsl::shared_ptr<bmqp::MessageGUIDGenerator> d_guidGenerator_sp;

    /// The final number of messages in the current `d_msgEvent` cached on
    /// switching this MessageEvent from WRITE to READ mode.
    /// This cached value exists because we are not able to access the
    /// underlying PutEventBuilder once downgraded to READ.
    /// CONTRACT: the stored value is correct every moment when in READ mode,
    /// and the value is not guaranteed to be correct when in WRITE mode.
    int d_messageCountFinal;

    /// The final message event size of the current `d_msgEvent` cached on
    /// switching this MessageEvent from WRITE to READ mode.
    /// This cached value exists because we are not able to access the
    /// underlying PutEventBuilder once downgraded to READ.
    /// CONTRACT: the stored value is correct every moment when in READ mode,
    /// and the value is not guaranteed to be correct when in WRITE mode.
    int d_messageEventSizeFinal;

    /// Distributed tracing tracer object.
    bsl::shared_ptr<bmqpi::DTTracer> d_dtTracer_sp;

    /// Distributed tracing context object.
    bsl::shared_ptr<bmqpi::DTContext> d_dtContext_sp;
};

// =========================
// class MessageEventBuilder
// =========================

/// A builder for `MessageEvent` objects.
class MessageEventBuilder {
  private:
    // DATA
    MessageEventBuilderImpl d_impl;  // Impl

  public:
    // CREATORS

    /// Create an invalid instance.  Application should not create
    /// `MessageEventBuilder` themselves, but ask the `bmqa::Session` to
    /// give them one, by using `bmqa::Session::loadMessageEventBuilder`.
    MessageEventBuilder();

    // MANIPULATORS

    /// Reset the current message being built, and return a reference to the
    /// current message.  Note that returned `Message` is valid until this
    /// builder is reset or destroyed.  Behavior is undefined if this
    /// builder was earlier used to build an event *and* has not been reset
    /// since then.
    Message& startMessage();

    /// Add the current message into the message event under construction,
    /// setting the destination queue of the current message to match the
    /// specified `queueId`.  Return zero on success, non-zero value
    /// otherwise.  In case of failure, this method has no effect on the
    /// underlying message event being built.  The behavior is undefined
    /// unless all mandatory attributes of the current Message have been
    /// set, *and* `queueId` is valid *and* has been opened in WRITE mode.
    /// The result is one of the values from the
    /// `bmqt::EventBuilderResult::Enum` enum.  Note that this method can be
    /// called multiple times in a row with different `queueId` if the
    /// application desires to post the same `bmqa::Message` to multiple
    /// queues: all attributes on the message are unchanged, exception for
    /// the `correlationId` which is cleared.
    bmqt::EventBuilderResult::Enum packMessage(const bmqa::QueueId& queueId);

    /// Reset the builder, effectively discarding the `MessageEvent` under
    /// construction.
    void reset();

    /// Return the `MessageEvent` that was built.  Note that returned
    /// `MessageEvent` is valid until this builder is reset or destroyed,
    /// calling this method multiple times in a row will return the same
    /// bmqa::MessageEvent instance.  Also note that this builder must be
    /// reset before calling `startMessage` again.
    const MessageEvent& messageEvent();

    /// Return a reference to the current message.
    Message& currentMessage();

    // ACCESSORS

    /// Return the number of messages currently in the MessageEvent being
    /// built.
    int messageCount() const;

    /// Return the size in bytes of the event built after last successful
    /// call to `packMessage()`, otherwise return zero.  Note that returned
    /// value represents the length of entire message event, *including*
    /// BlazingMQ wire protocol overhead.
    int messageEventSize() const;

  private:
    // ACCESSORS

    /// Create a child distributed tracing span and inject it into the message
    /// properties.  Load into the specified `properties` a copy of the
    /// message properties from the specified `builder` with the trace
    /// context injected, and load into the specified `span` the created
    /// span.  Use the specified `queueId` for queue metadata.  If span
    /// creation or serialization fails, `properties` and `span` are left
    /// unchanged.  Note that the returned `properties` must be kept alive
    /// until message serialization completes.
    void copyPropertiesAndInjectDT(
        bsl::shared_ptr<bmqp::MessageProperties>* properties,
        bsl::shared_ptr<bmqpi::DTSpan>*           span,
        bmqp::PutEventBuilder*                    builder,
        const bmqa::QueueId&                      queueId) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
