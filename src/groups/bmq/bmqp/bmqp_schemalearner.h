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

// bmqp_schemalearner.h                                               -*-C++-*-

#ifndef INCLUDED_BMQP_SCHEMALEARNER
#define INCLUDED_BMQP_SCHEMALEARNER

//@PURPOSE: Provide a mechanism to track MessageProperties Schema ids.
//
//@CLASSES:
//  bmqp::SchemaLearner: mechanism to track MessageProperties Schema ids.
//
//@DESCRIPTION: 'multiplex' all incoming ids from multiple sources into unique
// values and keep learned Schema for each id for each Context.  'demultiplex'
// outgoing unique ids indicating recycling to each Context in a stream-like
// manner.
// All operations require a context uniquely identifying input source / target
// output.  Each client maintains an instance of opaque 'Context' returned by
// 'createContext' and supplies the instance of 'Context' each time it needs to
// 'multiplex' / 'demultiplex' / 'read' / 'observe' Schema.
// Multiplexing, observing, and reading are loosely coupled.  Clients must call
// either 'multiplex' or 'observe' every time a message is received in order to
// detect id recycling.  'observe' does not return a unique id, while
// 'multiplex' does.
//
// Use cases:
// 1. When receiving messages from multiple sources, 'multiplex' returns a
//    unique id.
//    Callers are 'ClientSession' and 'FileBackedStorage'.
// 2. When loading MessageProperties, if the message has a known Schema within
//    the given 'Context', 'read' shortcuts the reading by loading the Schema
//    into the MessageProperties for future reading.  If Schema is unknown,
//    'read' learns the Schema and stores it for the given 'Context'.
//    Callers are 'bmqa::Message' and Queue Engines.
// 3. When sending messages to multiple targets, 'demultiplex' makes sure each
//    target 'Context' gets recycling indication in a stream-like manner
//    (supporting redeliveries).
//    Caller is 'QueueHandle'.
// 4. When receiving messages as a single target, 'observe' makes sure
//    previously learned 'Schema' is reset upon recycling indication.
//    Caller is 'bmqimp::BrokerSession'
//
/// Thread Safety
///-------------
// NOT thread safe as the only application for the moment is by an instance of
// 'Queue'.
//
/// Usage
///-----
//  ...
//  bmqp::SchemaLearner          theLearner(d_allocator_p);
//  bmqp::SchemaLearner::Context clientContext = theLearner.createContext();
//  bmqp::SchemaLearner::Context serverContext = theLearner.createContext();
//  ...
//  const bdlbb::Blob&           blob          = receivedData;
//  const bmqp::PutHeader&       putHeader     = receivedHeader;
//  bmqp::MessagePropertiesInfo  receivedInfo    (putHeader);
//  bmqp::MessagePropertiesInfo  translation   =
//                                          theLearner.multiplex(clientContext,
//                                                               receivedInfo);
//  ...
//
//  bmqp::MessageProperties      mps(d_allocator_p);
//  const bdlbb::Blob&           properties  = receivedProperties;
//  int                          rc;
//
//  ...The following two calls have the same effect:
//  rc = theLearner.read(clientContext, &mps, receivedInfo, properties);
//  ...or
//  rc = theLearner.read(serverContext, &mps, translation, properties);
//  ...provided the 'serverContext' is not used to 'multiplex'.
//

// BMQ

#include <bmqp_messageproperties.h>
#include <bmqp_protocol.h>

// BDE
#include <bsl_list.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace bmqp {

// =====================
// class SchemaLearner
// =====================

/// This mechanism tracks all Schema ids.  It performs translation of
/// multiple incoming ids into a unique one.  It learns and recognizes
/// previously learned Schemata. It detects recycling.
/// The main idea is to keep everything known about each schema for each
/// `Context` within the context itself.  The additional, hidden from
/// callers information about Schema is in the `Handle` struct.  It keeps
/// translated id and a counted reference to the Schema inside.
/// `Handle` is opaque info and it is not returned to callers unlike
/// `Schema`.
/// Each context references a handle for each id received within the
/// context.  In addition, there is a global catalog of all translated ids.
/// When an id is recycled, the corresponding handle gets marked as recycled
/// and new handle reference replaces the old one.  The old handle may still
/// be referenced by some context in which case next call for that context
/// will have to find another id.
/// Finally, all references are shared_ptr.
class SchemaLearner {
  private:
    // PRIVATE TYPES
    struct InternalContext;
    // Forward declaration to opaque type representing unique context.

    struct SchemaHandle;

    /// Internal.
    typedef bsl::shared_ptr<SchemaHandle> HandlePtr;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(SchemaLearner, bslma::UsesBslmaAllocator)

  public:
    // PUBLIC TYPES

    typedef MessagePropertiesInfo::SchemaIdType    SchemaIdType;
    typedef MessageProperties::SchemaPtr           SchemaPtr;
    typedef const bsl::shared_ptr<InternalContext> Context;

    // Opaque type representing unique context.

    // PUBLIC CONSTANTS
    static const SchemaIdType k_MAX_SCHEMA =
        MessagePropertiesInfo::k_MAX_SCHEMA;
    static const SchemaIdType k_NO_SCHEMA = MessagePropertiesInfo::k_NO_SCHEMA;

  private:
    // PRIVATE TYPES

    /// There are two kinds of context depending on the source of unique id:
    ///  1) generated by the learner;
    ///  2) supplied by the caller (as in the case of `primary lease id`.
    /// `ContextMap` keeps the latter kind.
    /// The key represents `foreignId` from `createContext` call.
    typedef bsl::unordered_map<int, const Context> ContextMap;

    /// Map schema id to HandlePtr.  This type is used in three cases
    ///  1) In `Source` where the id is unique within the context;
    ///  2) In `muxCatalog` where id is the unique result of translation.
    ///  3) In `demuxCatalog` where d is the unique result of translation.
    typedef bsl::unordered_map<SchemaIdType, HandlePtr> HandlesMap;

    typedef SchemaIdType   Key;
    typedef bsl::list<Key> LRU;

  private:
    // PRIVATE DATA
    bslma::Allocator* d_allocator_p;

    SchemaIdType d_capacity;
    // the default is k_MAX_SCHEMA

    SchemaIdType d_total;
    // current number of schemas

    ContextMap d_servers;
    // non-auto-generated context ids;

    HandlesMap d_muxCatalog;
    // mapping unique ids to Schemas to assists
    // recycling when multiplexing
    // (translating PUTs from multiple producers).

    HandlesMap d_demuxCatalog;
    // mapping unique ids to Schemas to ensure
    // recycling indication when demultiplexing
    // (pushing to multiple consumers).

    LRU d_lru;
    // List of all keys in the order of their use.
    // LRU is the front.
  public:
    // CREATORS
    explicit SchemaLearner(bslma::Allocator* basicAllocator = 0);
    ~SchemaLearner();

    // PUBLIC MANIULATORS
    Context createContext();
    Context createContext(int foreignId);

    /// Set up the specified `mps` to retrieve Message Properties from the
    /// specified `blob`.  If the sequence of Properties denoted by the
    /// specified `logic; is known, the `mps' will just keep a reference to
    /// to the schema and consult it when retrieving Properties.  Otherwise,
    /// populate the `mps` by parsing the `blob` and remember the sequence.
    /// The `logic` must be a result of previous `translate` call.
    int read(Context&                     context,
             MessageProperties*           mps,
             const MessagePropertiesInfo& messagePropertiesInfo,
             const bdlbb::Blob&           blob);

    /// Reset previously learned schema accumulated with the specified
    /// `context` and associated with the id in specified `input` if the
    /// `input` indicates recycling.
    void observe(Context& context, const MessagePropertiesInfo& input);

    /// Return `MessagePropertiesLogic` with a unique id associated with the
    /// specified `context` and the id in the specified `input`.  If there
    /// are no ids available, find and use a least used one (minimum number
    /// of hits), and dis-associated it from other Context referencing it.
    MessagePropertiesInfo multiplex(Context&                     context,
                                    const MessagePropertiesInfo& input);

    /// Return `MessagePropertiesLogic` with the same id as in the specified
    /// `input` and with `isRecycled` set to `true` if the id got recycled
    /// since the last call with the specified `context`.
    MessagePropertiesInfo demultiplex(Context&                     context,
                                      const MessagePropertiesInfo& input);

    /// For testing only.
    void _setCapacity(SchemaIdType maxSchemaId);

    // CLASS METHODS
    static bool isPresentAndValid(SchemaIdType schemaId);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------
// class SchemaLearner
// -------------------

// MANIPULATORS
inline void SchemaLearner::_setCapacity(SchemaIdType maxSchema)
{
    BSLS_ASSERT_OPT(maxSchema > d_total && "Shrinking is not supported");
    d_capacity = maxSchema;
}

}  // close package namespace
}  // close enterprise namespace

#endif
