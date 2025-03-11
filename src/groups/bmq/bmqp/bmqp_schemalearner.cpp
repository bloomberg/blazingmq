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

// bmqp_schemalearner.cpp                                             -*-C++-*-

// BMQ
#include <bmqp_schemalearner.h>
#include <bmqscm_version.h>

// BDE
#include <bdlf_bind.h>

namespace BloombergLP {
namespace bmqp {

// ============================
// struct SchemaLearner::Handle
// ============================

/// This struct keeps opaque data associated with each Schema
struct SchemaLearner::SchemaHandle {
    LRU::const_iterator d_listIterator;
    // Iterator in the list of keys which assists in
    // selecting least recently used Context.

    SchemaIdType d_id;
    // The unique result of translation of input id.

    SchemaPtr d_schema_sp;
    // Counted reference to Schema.

    bool d_isUpToDate;
    // If 'false', needs new translation.

    SchemaHandle(SchemaIdType id);

    ~SchemaHandle();
};

// CREATORS
SchemaLearner::SchemaHandle::SchemaHandle(SchemaIdType id)
: d_listIterator()
, d_id(id)
, d_schema_sp()
, d_isUpToDate(true)
{
    // NOTHING
}

SchemaLearner::SchemaHandle::~SchemaHandle()
{
    // NOTHING
}

// ============================
// struct SchemaLearner::Source
// ============================

/// This struct serves as an opaque context for SchemaLearner's clients
/// which have either their own unique ids (servers) or SchemaLearner
/// allocates unique ids for them.
struct SchemaLearner::InternalContext {
    const bool d_isForeign;
    // 'true' if created by
    // 'SchemaLearner::createContext(int foreignId)'

    HandlesMap d_handles;
    // Translation results and learned Schemas.

    InternalContext(bool isServer, bslma::Allocator* allocator);
};

// CREATORS
SchemaLearner::InternalContext::InternalContext(bool              isServer,
                                                bslma::Allocator* allocator)
: d_isForeign(isServer)
, d_handles(allocator)
{
    // NOTHING
}

// ======================
// struct SchemaLearner
// ======================

// CREATORS
SchemaLearner::SchemaLearner(bslma::Allocator* basicAllocator)
: d_allocator_p(bslma::Default::allocator(basicAllocator))
, d_capacity(k_MAX_SCHEMA)
, d_total(0)
, d_servers(d_allocator_p)
, d_muxCatalog(d_allocator_p)
, d_demuxCatalog(d_allocator_p)
, d_lru(basicAllocator)
{
    // NOTHING
}

SchemaLearner::~SchemaLearner()
{
    // NOTHING
}

// PUBLIC MANIULATORS
SchemaLearner::Context SchemaLearner::createContext()
{
    Context context(new (*d_allocator_p) InternalContext(false, d_allocator_p),
                    d_allocator_p);

    return context;
}

SchemaLearner::Context SchemaLearner::createContext(int foreignId)
{
    ContextMap::const_iterator context = d_servers.find(foreignId);

    if (context == d_servers.end()) {
        context = d_servers
                      .emplace(
                          foreignId,
                          Context(new (*d_allocator_p)
                                      InternalContext(true, d_allocator_p),
                                  d_allocator_p))
                      .first;
    }
    return context->second;
}

SchemaLearner::SchemaPtr*
SchemaLearner::observe(Context& context, const MessagePropertiesInfo& input)
{
    const SchemaIdType inputId = input.schemaId();

    if (!isPresentAndValid(inputId)) {
        // Nothing to do
        return 0;  // RETURN
    }

    BSLS_ASSERT_SAFE(context);

    // Lookup the schema within this source
    HandlesMap::iterator it = context->d_handles.find(inputId);

    if (it == context->d_handles.end()) {
        HandlePtr newHandle(new (*d_allocator_p) SchemaHandle(inputId),
                            d_allocator_p);
        it = context->d_handles.emplace(inputId, newHandle, d_allocator_p)
                 .first;
    }

    const HandlePtr& contextHandle = it->second;

    BSLS_ASSERT_SAFE(contextHandle);

    if (input.isRecycled()) {
        contextHandle->d_schema_sp.reset();
        // Must destroy previously learned Schema
    }

    return &contextHandle->d_schema_sp;
}

MessagePropertiesInfo
SchemaLearner::multiplex(Context& context, const MessagePropertiesInfo& input)
{
    // The main idea is to keep everything known about each schema for each
    // context within the context itself.  The 'service' information about
    // schema is kept in 'Handle' struct.  It keeps translated id and a counted
    // reference to Schema inside, so their relationship is always 1:1.
    // 'Handle' keeps opaque info which is not returned to callers unlike
    // 'Schema'.
    // Each context references a handle for each id received within the
    // context.  In addition, there is a catalog of all translated ids.
    // When an id is recycled, the corresponding handle gets marked as recycled
    // and new handle reference replaces the old one.  The old handle may still
    // be referenced by some context in which case next call for that context
    // will have to find another id.
    // All references are 'shared_ptr'.

    SchemaIdType inputId = input.schemaId();

    if (!isPresentAndValid(inputId)) {
        // Nothing to do
        return input;  // RETURN
    }

    BSLS_ASSERT_SAFE(context);

    typedef bsl::pair<HandlesMap::iterator, bool> InsertResult;

    SchemaIdType outputId = k_NO_SCHEMA;

    // Lookup the schema within this source
    InsertResult        insertResult  = context->d_handles.emplace(inputId,
                                                           HandlePtr());
    bool                isRecycled    = input.isRecycled();
    bool                isUpToDate    = false;
    HandlePtr&          contextHandle = insertResult.first->second;
    LRU::const_iterator entryInLRU    = d_lru.end();
    // LRU tracking for 'outputId'

    if (contextHandle) {
        if (isRecycled) {
            contextHandle->d_schema_sp.reset();
            // Must release reference to the previously learned Schema.
            // Note that existing 'MessageProperties' instances can
            // continue using the old schema; only new 'MessageProperties'
            // need to learn new schema.
        }
        isUpToDate = contextHandle->d_isUpToDate;
        entryInLRU = contextHandle->d_listIterator;
    }

    if (isUpToDate) {
        BSLS_ASSERT_SAFE(contextHandle);

        outputId = contextHandle->d_id;
        // Even if 'inputId' is recycled, keep the translation.
    }
    else {
        // Need to allocate new id and set 'contextHandle->d_id'.

        // This is the entry in `d_muxCatalog` corresponding to `outputId`.
        HandlesMap::iterator catalogEntry;

        if (d_total < d_capacity) {
            // Allocate new schema id and new Handle.
            outputId = ++d_total;

            InsertResult insert = d_muxCatalog.emplace(outputId, HandlePtr());
            BSLS_ASSERT_SAFE(insert.second);

            catalogEntry = insert.first;

            // Start tracking it in LRU
            d_lru.push_back(outputId);
            entryInLRU = --d_lru.end();
        }
        else {
            // There is no room for new id.  Recycle an existing one.
            BSLS_ASSERT_SAFE(!d_lru.empty());

            // Recycle id associated with the front of 'd_lru'.
            outputId     = d_lru.front();
            catalogEntry = d_muxCatalog.find(outputId);
            BSLS_ASSERT_SAFE(catalogEntry != d_muxCatalog.end());

            catalogEntry->second->d_isUpToDate = false;
            // If some other instance of 'Context' refers to 'result', it
            // will have to find new 'outputId' next time.
            // If there is previously learned 'Schema'
            // ('catalogEntry->second->d_schema'), keep it, it still works.
            entryInLRU = catalogEntry->second->d_listIterator;
            // 'entryInLRU' corresponds to the recycled 'outputId'.

            // Neither 'catalogEntry' nor 'entryInLRU' get erased.  Instead,
            // 'catalogEntry' will point to the new 'contextHandle' and
            // 'entryInLRU' will get pushed back.
        }
        BSLS_ASSERT_SAFE(outputId != k_NO_SCHEMA);

        if (contextHandle) {
            contextHandle->d_id         = outputId;
            contextHandle->d_isUpToDate = true;
            // If there is a previously learned 'Schema'
            // ('handle->d_schema') for the 'inputSchemaId', keep it.
            // It still works, we have just changed the id translation.
        }
        else {
            contextHandle.load(new (*d_allocator_p) SchemaHandle(outputId),
                               d_allocator_p);
        }

        catalogEntry->second = contextHandle;
        isRecycled           = true;
    }

    BSLS_ASSERT_SAFE(entryInLRU != d_lru.end());
    BSLS_ASSERT_SAFE(outputId != k_NO_SCHEMA);

    // Update LRU
    BSLS_ASSERT_SAFE(entryInLRU != d_lru.end());
    if (--d_lru.end() != entryInLRU) {
        d_lru.splice(d_lru.end(), d_lru, entryInLRU);
    }
    // Update 'contextHandle' with the LRU tracking
    contextHandle->d_listIterator = entryInLRU;

    return MessagePropertiesInfo(input.isPresent(), outputId, isRecycled);
}

MessagePropertiesInfo
SchemaLearner::demultiplex(Context&                     context,
                           const MessagePropertiesInfo& input)
{
    const SchemaIdType inputId = input.schemaId();

    if (!isPresentAndValid(inputId)) {
        // Nothing to do
        return input;  // RETURN
    }

    BSLS_ASSERT_SAFE(context);

    typedef bsl::pair<HandlesMap::iterator, bool> InsertResult;

    // Lookup the schema within this source
    InsertResult lookupOrInsert = context->d_handles.emplace(inputId,
                                                             HandlePtr());
    bool         isRecycled     = true;
    HandlePtr&   contextHandle  = lookupOrInsert.first->second;

    if (contextHandle) {
        if (input.isRecycled()) {
            contextHandle->d_schema_sp.reset();
            // Must destroy previously learned Schema
        }
        else {
            isRecycled = !contextHandle->d_isUpToDate;
        }
    }

    if (isRecycled) {
        // Update  the catalog with new 'HandlePtr'
        InsertResult catalogLookupOrInsert =
            d_demuxCatalog.emplace(inputId, HandlePtr());

        if (!catalogLookupOrInsert.second) {
            BSLS_ASSERT_SAFE(catalogLookupOrInsert.first->second);
            if (input.isRecycled()) {
                catalogLookupOrInsert.first->second->d_isUpToDate = false;
                // Force other Context to recycle
                catalogLookupOrInsert.first->second->d_schema_sp.reset();
            }
        }
        else {
            // First time demux'ing the 'inputId'
            BSLS_ASSERT_SAFE(!contextHandle);
        }
        // replace the 'HandlePtr' in the catalog with the new one
        catalogLookupOrInsert.first->second.load(new (*d_allocator_p)
                                                     SchemaHandle(inputId),
                                                 d_allocator_p);

        contextHandle = catalogLookupOrInsert.first->second;
    }

    return MessagePropertiesInfo(input.isPresent(), inputId, isRecycled);
}

int SchemaLearner::read(Context&                     context,
                        MessageProperties*           mps,
                        const MessagePropertiesInfo& messagePropertiesInfo,
                        const bdlbb::Blob&           blob) const
{
    enum RcEnum { rc_SUCCESS = 0, rc_PARSING_ERROR = -1 };

    BSLS_ASSERT_SAFE(mps);

    int          rc            = rc_SUCCESS;
    SchemaIdType inputSchemaId = messagePropertiesInfo.schemaId();

    if (!isPresentAndValid(inputSchemaId)) {
        // Invalid schema or old style
        rc = mps->streamIn(blob, messagePropertiesInfo.isExtended());
        return rc;  // RETURN
    }

    // Lookup the schema within this source
    HandlePtr& schemaHandle = context->d_handles[inputSchemaId];

    if (!schemaHandle) {
        schemaHandle.load(new (*d_allocator_p) SchemaHandle(inputSchemaId),
                          d_allocator_p);
    }
    else if (messagePropertiesInfo.isRecycled()) {
        // forget the schema
        schemaHandle->d_schema_sp.reset();
    }

    rc = mps->streamIn(blob, messagePropertiesInfo, schemaHandle->d_schema_sp);

    if (rc == 0) {
        if (!schemaHandle->d_schema_sp) {
            // Learn new schema.
            schemaHandle->d_schema_sp = mps->makeSchema(d_allocator_p);
        }
    }
    else {
        rc = 10 * rc + rc_PARSING_ERROR;
    }

    // even if the downstream to upstream mapping has changed, the schema
    // is still good unless it is recycled

    return rc;
}

// CLASS METHODS
bool SchemaLearner::isPresentAndValid(SchemaIdType schemaId)
{
    if (schemaId > k_MAX_SCHEMA) {
        // Invalid schema
        return false;  // RETURN
    }
    if (schemaId == k_NO_SCHEMA) {
        // Do not learn it.
        return false;  // RETURN
    }

    return true;
}

}  // close package namespace
}  // close enterprise namespace
