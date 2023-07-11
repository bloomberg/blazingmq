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

// bmqp_schemagenerator.cpp                                           -*-C++-*-

// BMQ
#include <bmqp_schemagenerator.h>
#include <bmqp_schemalearner.h>
#include <bmqscm_version.h>

// BDE
#include <bdlma_localsequentialallocator.h>

namespace BloombergLP {
namespace bmqp {

// ===============================
// struct SchemaGenerator::Context
// ===============================

/// This struct represents result of id generation - the id and reference
/// to the list of least recently used items where LRU is the front.
struct SchemaGenerator::Context {
    SchemaIdType d_id;
    // The generated id.

    LRU::const_iterator d_listIterator;
    // Iterator in the list of keys which assists in
    // selecting least recently used Context.

    explicit Context(const LRU::const_iterator& cit);
};

// CREATORS
inline SchemaGenerator::Context::Context(const LRU::const_iterator& cit)
: d_id(k_NO_SCHEMA)
, d_listIterator(cit)
{
    // NOTHING
}

// ======================
// struct SchemaGenerator
// ======================

// CREATORS
SchemaGenerator::SchemaGenerator(bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_capacity(k_MAX_SCHEMA)
, d_currentId(0)
, d_contextMap(d_allocator_p)
, d_lru(d_allocator_p)
, d_lock(bsls::SpinLock::s_unlocked)
{
    // NOTHING
}

SchemaGenerator::~SchemaGenerator()
{
    // NOTHING
}

// MANIPULATORS
MessagePropertiesInfo
SchemaGenerator::getSchemaId(const MessageProperties* mps)
{
    // Do not send empty MPs.  This has to agree with
    // 'bmqp::PutEventBuilder::packMessage'

    if (mps == 0 || mps->numProperties() == 0) {
        return MessagePropertiesInfo();  // RETURN
    }

    bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
    MessagePropertiesIterator             it(mps);
    bsl::string                           key(&localAllocator);

    while (it.hasNext()) {
        key += "_" + it.name();
    }

    typedef bsl::pair<ContextMap::iterator, bool> InsertOrLookup;

    bsls::SpinLockGuard guard(&d_lock);  // LOCK

    InsertOrLookup insertOrLookup = d_contextMap.emplace(key, d_lru.end());
    const Context& context        = insertOrLookup.first->second;
    SchemaIdType   result         = context.d_id;
    bool           isNew          = insertOrLookup.second;
    LRU::const_iterator current   = context.d_listIterator;

    if (isNew) {
        // Either 1st time using this 'key' or it was 'recycled' and erased.

        if (d_currentId < d_capacity) {
            // Assign new schema
            result = ++d_currentId;
        }
        else {
            // There is no room for new id; need to recycle existing one.
            // Get the least used schema which the front of 'd_listIterator'

            BSLS_ASSERT_OPT(!d_lru.empty());

            // Recycle id associated with the front of 'd_listIterator'.
            ContextMap::iterator recycled = d_contextMap.find(d_lru.front());
            BSLS_ASSERT_OPT(recycled != d_contextMap.end());

            result = recycled->second.d_id;

            // Erase recycled context
            d_contextMap.erase(recycled);
            d_lru.pop_front();
        }

        BSLS_ASSERT_OPT(result != k_NO_SCHEMA);

        // Initialize newly allocated 'Context'
        insertOrLookup.first->second.d_id = result;
        // Start tracking it in LRU
        d_lru.push_back(key);
        current = --d_lru.end();
    }
    else {
        BSLS_ASSERT_OPT(current != d_lru.end());
        BSLS_ASSERT_OPT(insertOrLookup.first->second.d_id != k_NO_SCHEMA);

        // reorder LRU
        if (--d_lru.end() != current) {
            d_lru.splice(d_lru.end(), d_lru, current);
        }
    }

    // Update 'Context' with the LRU tracking
    insertOrLookup.first->second.d_listIterator = current;

    return MessagePropertiesInfo(true, result, isNew);
}

}  // close package namespace
}  // close enterprise namespace
