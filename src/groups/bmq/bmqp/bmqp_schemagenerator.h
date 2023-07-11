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

// bmqp_schemagenerator.h                                             -*-C++-*-

#ifndef INCLUDED_BMQP_SCHEMAGENERATOR
#define INCLUDED_BMQP_SCHEMAGENERATOR

//@PURPOSE: Provide a mechanism to map a sequence of (property) names to a
//          unique id.
//@CLASSES:
//  bmqp::SchemaGenerator: mechanism to generate a unique id out of
//  MessageProperties.
//
//@DESCRIPTION: generate an integer (schema id) in the range [1, k_MAX_SCHEMA]
//  in such a way that another sequence of the same names maps to the same id.
// The logic assumes that all sequences are sorted in the same order and that
// there is no more than one occurrence of a name in any sequence.
//
// Use cases:
// When formatting a PUT message with MesageProperties, call `getSchemaId` and
// store the return in the PUT header.
//
/// Thread Safety
///-------------
// Thread safety is enforced by a (spin) lock.
//
/// Usage
///-----
//  ...
//  const MessageProperties&    input = prepopulatedMPs;
//  bmqp::SchemaGenerator       theGenerator(d_allocator_p);
//  bmqp::MessagePropertiesInfo info  = theGenerator.getSchemaId(&input);
//  bmqp::PutHeader             outgoingHeader;
//
//  info.applyTo(&outgoingHeader);
//

// BMQ

#include <bmqp_messageproperties.h>
#include <bmqp_protocol.h>

// BDE
#include <bsl_list.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_allocator.h>
#include <bsls_spinlock.h>

namespace BloombergLP {
namespace bmqp {

// =====================
// class SchemaGenerator
// =====================

/// This class implements a mechanism mapping a sequence of (property) names
/// represented by an instance of `MessageProperties` to an integer (schema
/// id) in the range [1, k_MAX_SCHEMA] in such a way that another sequence
/// of the same names maps to the same schema id.
/// The logic assumes that all sequences are sorted in the same order and
/// that there is no more than occurrence of a name in any sequence.
///
/// The implementation iterates given sequence and constructs a key as a
/// concatenation of all names using `_` as delimiter.
///
/// REVISIT: it is possible to make this class a template to make it work
/// with a sequence representation other than `MessageProperties`.
class SchemaGenerator {
  public:
    // PUBLIC TYPES
    typedef MessagePropertiesInfo::SchemaIdType SchemaIdType;

    // CONSTANTS
    static const SchemaIdType k_MAX_SCHEMA =
        MessagePropertiesInfo::k_MAX_SCHEMA;
    static const SchemaIdType k_NO_SCHEMA = MessagePropertiesInfo::k_NO_SCHEMA;

  private:
    // PRIVATE TYPES

    struct Context;

    /// Key is a concatenation of all properties names.
    /// It uniquely identifies each sequence.
    typedef bsl::string Key;

    /// Maps sequence key to a context holding generated id.
    typedef bsl::unordered_map<Key, Context> ContextMap;

    typedef bsl::list<Key> LRU;

  private:
    // PRIVATE DATA
    bslma::Allocator* d_allocator_p;

    SchemaIdType d_capacity;
    // the default is k_MAX_SCHEMA

    SchemaIdType d_currentId;
    // next available id

    ContextMap d_contextMap;
    // information about each Schema id

    LRU d_lru;
    // List of all keys in the order of their use.
    // LRU is the front.

    mutable bsls::SpinLock d_lock;
    // Spin lock for thread-safe manipulators

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(SchemaGenerator, bslma::UsesBslmaAllocator)

  public:
    // CREATORS
    explicit SchemaGenerator(bslma::Allocator* basicAllocator = 0);
    ~SchemaGenerator();

    // PUBLIC MANIPULATORS
    MessagePropertiesInfo getSchemaId(const MessageProperties* mps);

    /// For testing only.
    void _setCapacity(SchemaIdType maxSchema);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// class SchemaGenerator
// ---------------------

// MANIPULATORS
inline void SchemaGenerator::_setCapacity(SchemaIdType maxSchema)
{
    BSLS_ASSERT_OPT(maxSchema > d_currentId && "Shrinking is not supported");
    d_capacity = maxSchema;
}

}  // close package namespace
}  // close enterprise namespace

#endif
