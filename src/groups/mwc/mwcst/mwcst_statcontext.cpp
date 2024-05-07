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

// mwcst_statcontext.cpp                                              -*-C++-*-
#include <mwcst_statcontext.h>

#include <mwcscm_version.h>
#include <mwcst_statcontextuserdata.h>

#include <ball_log.h>

#include <bslmf_allocatorargt.h>
#include <bslmt_lockguard.h>
#include <bslmt_once.h>

#include <bdlb_bitutil.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlt_timeunitratio.h>
#include <bsls_systemtime.h>

#include <bsl_algorithm.h>
#include <bsl_utility.h>
#include <bslim_printer.h>
#include <bsls_alignedbuffer.h>
#include <bsls_timeutil.h>

#include <mwcstm_values.h>

namespace BloombergLP {
namespace mwcst {

namespace {

// FUNCTIONS
size_t idHash(const bdlb::Variant2<bsl::string, bsls::Types::Int64>& id)
{
    static bsl::hash<bsl::string>        stringHash;
    static bsl::hash<bsls::Types::Int64> int64Hash;

    if (id.is<bsl::string>()) {
        return stringHash(id.the<bsl::string>());
    }
    else if (id.is<bsls::Types::Int64>()) {
        return int64Hash(id.the<bsls::Types::Int64>());
    }
    else {
        return 0;
    }
}

void snapshotValueVec(bsl::vector<StatValue>* vec,
                      bsls::Types::Int64      snapshotTime)
{
    if (vec) {
        for (size_t i = 0; i < vec->size(); ++i) {
            (*vec)[i].takeSnapshot(snapshotTime);
        }
    }
}

void addValues(bsl::vector<StatValue>* dest, const bsl::vector<StatValue>* src)
{
    if (dest) {
        // Add the 'src' stats to the 'dest' stats
        for (size_t i = 0; i < dest->size(); ++i) {
            if (src) {
                (*dest)[i].addSnapshot((*src)[i]);
            }
        }
    }
}

void syncValues(bsl::vector<StatValue>*       vec,
                const bsl::vector<StatValue>& syncVec)
{
    if (!vec || syncVec.empty()) {
        return;
    }

    for (size_t i = 0; i < vec->size(); ++i) {
        (*vec)[i].syncSnapshotSchedule(syncVec[i]);
    }
}

void clearStats(bsl::vector<StatValue>* values)
{
    if (values) {
        for (size_t i = 0; i < values->size(); ++i) {
            (*values)[i].clearCurrentStats();
        }
    }
}

/// Load into the specified `updates` corresponding updates from the
/// specified `values` vector based on the specified `mask`, if `values` is
/// non-zero, and do nothing otherwise.  If the specified `full` is `true`,
/// load a full `StatValue` update into each element of `values`, otherwise
/// load the changes since the last snapshot.
static void
loadUpdatesFromValues(bsl::vector<mwcstm::StatValueUpdate>* updates,
                      const bsl::vector<StatValue>*         values,
                      int                                   mask,
                      bool                                  full)
{
    if (values) {
        updates->resize(values->size());
        for (bsl::size_t i = 0; i < values->size(); ++i) {
            updates->at(i).reset();
            if (full) {
                StatValueUtil::loadFullUpdate(&updates->at(i),
                                              values->at(i),
                                              mask);
            }
            else {
                StatValueUtil::loadUpdate(&updates->at(i),
                                          values->at(i),
                                          mask);
            }
        }
    }
}

/// Return the offset between epoch time and the system timer.
static bsls::Types::Int64 epochOffset()
{
    static bsls::Types::Int64 offset;
    BSLMT_ONCE_DO
    {
        offset = (bsls::SystemTime::nowRealtimeClock().totalMilliseconds() *
                  bdlt::TimeUnitRatio::k_NANOSECONDS_PER_MILLISECOND) -
                 bsls::TimeUtil::getTimer();
    }
    return offset;
}

inline static bsls::Types::Int64 convertToEpoch(bsls::Types::Int64 timerTime)
{
    return epochOffset() + timerTime;
}

inline static bsls::Types::Int64 convertFromEpoch(bsls::Types::Int64 epochTime)
{
    return epochTime - epochOffset();
}

}  // close anonymous namespace

// -----------------
// class StatContext
// -----------------

// PRIVATE CLASS METHODS
void StatContext::statContextDeleter(void* context_vp, void* allocator_vp)
{
    StatContext* context        = (StatContext*)context_vp;
    context->d_isDeleted        = true;
    if (context->d_released.swap(true)) {
        // Context was already release by its parent context, therefore we are
        // responsible for deallocating it.

        bslma::Allocator* allocator = static_cast<bslma::Allocator*>(
            allocator_vp);
        allocator->deleteObject(context);
    }
}

void StatContext::datumDeleter(void* /*datum_vp*/, void* datumLock_vp)
{
    bsls::SpinLock* lock = static_cast<bsls::SpinLock*>(datumLock_vp);
    lock->unlock();
}

// PRIVATE MANIPULATORS
void StatContext::initValues(ValueVecPtr& vec, bsls::Types::Int64 initTime)
{
    if (!d_valueDefs_p) {
        return;
    }

    bsl::vector<StatValue>* newVec = d_valueVecPool_p->getObject();

    newVec->resize(d_valueDefs_p->size());
    for (size_t vIdx = 0; vIdx < d_valueDefs_p->size(); ++vIdx) {
        (*newVec)[vIdx].init((*d_valueDefs_p)[vIdx].d_sizes,
                             (*d_valueDefs_p)[vIdx].d_type,
                             initTime);
    }

    vec.load(newVec, d_valueVecPool_p.get());
}

void StatContext::clearDeletedSubcontexts(
    bsl::vector<ValueVec*>* expiredValuesVec)
{
    for (StatContextVector::iterator iter = d_deletedSubcontexts.begin();
         iter != d_deletedSubcontexts.end();
         ++iter) {
        if (expiredValuesVec) {
            for (size_t i = 0; i < expiredValuesVec->size(); ++i) {
                addValues((*expiredValuesVec)[i],
                          (*iter)->d_directValues_p.ptr());
            }
        }

        if (d_update_p) {
            // If we're collecting updates, we need to remove the update for
            // this deleted subcontext from the update list.  The update
            // pointer on the subcontext gives us an iterator into this vector,
            // that we use to erase the element.

            BSLS_ASSERT((*iter)->d_update_p);
            bsl::vector<mwcstm::StatContextUpdate>& updates =
                d_update_p->subcontexts();
            mwcstm::StatContextUpdate* update = (*iter)->d_update_p;
            BSLS_ASSERT(updates.begin() <= update && update < updates.end());

            // Swap the update to be deleted with the last update, update the
            // reference of the subcontext to the new update location, and then
            // erase the last element, corresponding to the deleted update.

            if (update != &updates.back()) {
                mwcstm::StatContextUpdate* last = &updates.back();
                StatContext* subcontext = d_subcontextsById[last->id()];
                bsl::swap(update, last);
                subcontext->d_update_p = update;
            }
            updates.resize(updates.size() - 1);
        }

        BSLS_ASSERT(d_subcontextsById.find((*iter)->d_uniqueId) !=
                    d_subcontextsById.end());
        d_subcontextsById.erase(d_subcontextsById.find((*iter)->d_uniqueId));

        d_allocator_p->deleteObject(*iter);
    }

    d_deletedSubcontexts.clear();
}

void StatContext::moveNewSubcontexts()
{
    StatContextVector localNewSubcontexts(d_allocator_p);
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_newSubcontextsLock);
        localNewSubcontexts.swap(d_newSubcontexts);
    }

    bsl::vector<mwcstm::StatContextUpdate>* updates = 0;
    if (d_update_p) {
        updates = &d_update_p->subcontexts();
        updates->reserve(updates->size() + localNewSubcontexts.size());
    }

    for (StatContextVector::iterator iter = localNewSubcontexts.begin();
         iter != localNewSubcontexts.end();
         ++iter) {
        StatContext*               context = *iter;
        StatContextMap::value_type val(context->d_id, context);
        d_subcontexts.insert(val);

        BSLS_ASSERT(d_subcontextsById.find(context->d_uniqueId) ==
                    d_subcontextsById.end());
        d_subcontextsById.insert(bsl::make_pair(context->d_uniqueId, context));

        if (updates) {
            // If we are collecting updates, pass the subcontext an object to
            // collect updates into.

            updates->resize(1 + d_update_p->subcontexts().size());
            context->d_update_p = &updates->back();
            context->initializeUpdate(context->d_update_p);
        }
    }
}

StatContext::ValueVec* StatContext::getTotalValuesVec()
{
    if (d_totalValues_p.ptr()) {
        return d_totalValues_p.ptr();
    }
    else {
        return d_directValues_p.ptr();
    }
}

void StatContext::snapshotSubcontext(StatContext*       subcontext,
                                     bsls::Types::Int64 snapshotTime)
{
    if (subcontext->d_numSnapshots == 0 && d_isTable && d_directValues_p) {
        // Sync the child context's values' snapshotSchedules with ours.
        syncValues(subcontext->d_totalValues_p.ptr(), *d_directValues_p);
        syncValues(subcontext->d_activeChildrenTotalValues_p.ptr(),
                   *d_directValues_p);
        syncValues(subcontext->d_directValues_p.ptr(), *d_directValues_p);
        syncValues(subcontext->d_expiredValues_p.ptr(), *d_directValues_p);
    }

    subcontext->snapshotImp(snapshotTime);

    // Don't just add the subcontext's total values because that will include
    // their expired children too, if it's keeping track of them
    addValues(d_activeChildrenTotalValues_p.ptr(),
              subcontext->d_directValues_p.ptr());
    addValues(d_activeChildrenTotalValues_p.ptr(),
              subcontext->d_activeChildrenTotalValues_p.ptr());
}

void StatContext::snapshotImp(bsls::Types::Int64 snapshotTime)
{
    if (d_preSnapshotCallback) {
        d_preSnapshotCallback(*this);
    }

    if (d_update_p) {
        // Update the timestamp, and clear our configuration and created flag
        // if this is our second snapshot.

        d_update_p->timeStamp() = convertToEpoch(snapshotTime);
        if (1 == d_numSnapshots) {
            d_update_p->configuration().reset();
            d_update_p->flags() = bdlb::BitUtil::withBitCleared(
                d_update_p->flags(),
                mwcstm::StatContextUpdateFlags::DMCSTM_CONTEXT_CREATED);
        }
    }

    moveNewSubcontexts();

    if (d_isTable && !d_subcontexts.empty() &&
        !d_activeChildrenTotalValues_p && d_directValues_p) {
        // Initialize 'd_activeChildrenTotalValues_p' if we have subtables
        initValues(d_activeChildrenTotalValues_p);
        syncValues(d_activeChildrenTotalValues_p.ptr(), *d_directValues_p);
    }

    clearStats(d_activeChildrenTotalValues_p.ptr());

    // Snapshot all subcontexts and, if we're a table, add them to our
    // children's total
    for (StatContextVector::iterator iter = d_deletedSubcontexts.begin();
         iter != d_deletedSubcontexts.end();
         ++iter) {
        snapshotSubcontext(*iter, snapshotTime);
    }

    for (StatContextMap::iterator iter = d_subcontexts.begin();
         iter != d_subcontexts.end();
         /*nothing*/) {
        snapshotSubcontext(iter->second, snapshotTime);

        if (iter->second->isDeleted()) {
            d_deletedSubcontexts.push_back(iter->second);
            mwcstm::StatContextUpdate* update = iter->second->d_update_p;
            if (update) {
                update->flags() = bdlb::BitUtil::withBitSet(
                    update->flags(),
                    mwcstm::StatContextUpdateFlags::DMCSTM_CONTEXT_DELETED);
            }
            d_subcontexts.erase(iter++);
        }
        else {
            ++iter;
        }
    }

    snapshotValueVec(d_activeChildrenTotalValues_p.ptr(), snapshotTime);
    snapshotValueVec(d_expiredValues_p.ptr(), snapshotTime);
    if (d_update_p) {
        // Collect updates from expired values now that they've been
        // snapshotted.

        loadUpdatesFromValues(&d_update_p->expiredValues(),
                              d_expiredValues_p.ptr(),
                              d_updateValueFieldMask,
                              0 == d_numSnapshots);
    }

    if (!d_totalValues_p &&
        (d_activeChildrenTotalValues_p || d_expiredValues_p)) {
        initValues(d_totalValues_p);
        if (d_isTable) {
            syncValues(d_totalValues_p.ptr(), *d_directValues_p);
        }
    }

    snapshotValueVec(d_directValues_p.ptr(), snapshotTime);
    if (d_update_p) {
        // Collect updates from direct values now that they've been
        // snapshotted.

        loadUpdatesFromValues(&d_update_p->directValues(),
                              d_directValues_p.ptr(),
                              d_updateValueFieldMask,
                              0 == d_numSnapshots);
    }

    clearStats(d_totalValues_p.ptr());
    addValues(d_totalValues_p.ptr(), d_directValues_p.ptr());
    addValues(d_totalValues_p.ptr(), d_activeChildrenTotalValues_p.ptr());
    addValues(d_totalValues_p.ptr(), d_expiredValues_p.ptr());
    snapshotValueVec(d_totalValues_p.ptr(), snapshotTime);

    ++d_numSnapshots;

    // Snapshot the user data.  This must happen last so that the user data may
    // trigger a read from the latest snapshot of data in this context.

    if (d_userData_p) {
        d_userData_p->snapshot();
    }
}

void StatContext::cleanupImp(bsl::vector<ValueVec*>* expiredValuesVec)
{
    // Only store expired values if we're a table.  Wouldn't make sense
    // otherwise
    if (d_isTable && d_storeExpiredValues && !d_expiredValues_p) {
        initValues(d_expiredValues_p);
        syncValues(d_expiredValues_p.ptr(), *d_directValues_p);
    }

    // Push our expiredValues onto the vector.  All children will add the
    // final values of any subcontexts being deleted to all the ValueVecs in
    // this vector.
    if (expiredValuesVec) {
        expiredValuesVec->push_back(d_expiredValues_p.ptr());
    }

    clearDeletedSubcontexts(expiredValuesVec);

    for (StatContextMap::iterator iter = d_subcontexts.begin();
         iter != d_subcontexts.end();
         ++iter) {
        iter->second->cleanupImp(expiredValuesVec);
    }

    if (expiredValuesVec) {
        expiredValuesVec->pop_back();
    }
}

void StatContext::applyUpdate(const mwcstm::StatContextUpdate& update)
{
    // Apply the update to all of our values.

    BSLS_ASSERT(update.directValues().size() == d_directValues_p->size());
    bsl::size_t numValues = bsl::min(d_directValues_p->size(),
                                     update.directValues().size());
    for (bsl::size_t i = 0; i < numValues; ++i) {
        d_directValues_p->at(i).setFromUpdate(update.directValues()[i]);
        if (d_storeExpiredValues) {
            d_expiredValues_p->at(i).setFromUpdate(update.expiredValues()[i]);
        }
    }

    // Create, update, or delete subcontexts as specified in the update.

    typedef mwcstm::StatContextUpdateFlags Flags;
    for (bsl::vector<mwcstm::StatContextUpdate>::const_iterator i =
             update.subcontexts().begin();
         i != update.subcontexts().end();
         ++i) {
        StatContext* context = 0;
        if (bdlb::BitUtil::isBitSet(i->flags(),
                                    Flags::DMCSTM_CONTEXT_CREATED)) {
            StatContextConfiguration config(*i);

            // Subcontexts added by an update are only deletable by another
            // update, but they must be cleaned up when this object is
            // destroyed.

            context             = addSubcontext(config).release().first;
            context->d_released = true;
        }
        else {
            StatContextIdMap::iterator si = d_subcontextsById.find(i->id());
            if (si == d_subcontextsById.end()) {
                continue;  // CONTINUE
            }
            context = si->second;
            context->applyUpdate(*i);
        }
        BSLS_ASSERT(context);

        if (bdlb::BitUtil::isBitSet(i->flags(),
                                    Flags::DMCSTM_CONTEXT_DELETED)) {
            context->d_isDeleted = true;
        }
    }
}

// CREATORS
StatContext::StatContext(const Config&     config,
                         bslma::Allocator* basicAllocator)
: d_id(config.d_id, basicAllocator)
, d_uniqueId(config.d_uniqueId)
, d_nextSubcontextId_p()
, d_released(false)
, d_isDeleted(false)
, d_isTable(config.d_isTable)
, d_storeExpiredValues(config.d_storeExpiredSubcontextValues)
, d_defaultHistorySizes(config.d_defaultHistorySizes, basicAllocator)
, d_valueDefs_p()
, d_valueVecPool_p()
, d_totalValues_p()
, d_activeChildrenTotalValues_p()
, d_directValues_p()
, d_expiredValues_p()
, d_subcontexts(0, &idHash, StatContextMap::key_equal(), basicAllocator)
, d_subcontextsById(basicAllocator)
, d_deletedSubcontexts(basicAllocator)
, d_newSubcontexts(basicAllocator)
, d_newSubcontextsLock()
, d_userData_p(config.d_userData_p.managedPtr())
, d_managedDatumLock(bsls::SpinLock::s_unlocked)
, d_managedDatum(basicAllocator)
, d_preSnapshotCallback(bsl::allocator_arg,
                        basicAllocator,
                        config.d_preSnapshotCallback)
, d_numSnapshots(0)
, d_update_p(config.d_updateCollector_p)
, d_updateValueFieldMask(config.d_updateValueFieldMask)
, d_statValueAllocator_p(config.d_statValueAllocator_p)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    if (0 < config.d_valueDefs.size()) {
        d_valueVecPool_p.createInplace(
            basicAllocator,
            -1,
            d_statValueAllocator_p ? d_statValueAllocator_p : basicAllocator);

        d_valueDefs_p.createInplace(basicAllocator,
                                    config.d_valueDefs,
                                    basicAllocator);

        // Configure any value defs without a history size with the default
        // history size.
        for (size_t i = 0; i < d_valueDefs_p->size(); ++i) {
            if ((*d_valueDefs_p)[i].d_sizes.empty()) {
                (*d_valueDefs_p)[i].d_sizes = config.d_defaultHistorySizes;
            }
        }

        initValues(d_directValues_p, bsls::TimeUtil::getTimer());
    }

    if (config.d_update_p) {
        applyUpdate(*config.d_update_p);
        snapshotImp(convertFromEpoch(config.d_update_p->timeStamp()));
    }

    if (d_update_p) {
        // If we are collecting updates, then we need to initialize the update
        // with our baseline configuration.

        initializeUpdate(d_update_p);
    }

    if (!config.d_nextSubcontextId_p.get()) {
        d_nextSubcontextId_p.createInplace(basicAllocator, 1);
    }
    else {
        d_nextSubcontextId_p = config.d_nextSubcontextId_p;
    }
}

// MANIPULATORS
bslma::ManagedPtr<StatContext> StatContext::addSubcontext(const Config& config)
{
    bdlma::LocalSequentialAllocator<1024> seqAlloc;

    StatContext* newContext = 0;
    Config       newConfig(config, &seqAlloc);
    newConfig.d_updateValueFieldMask = d_updateValueFieldMask;
    newConfig.d_nextSubcontextId_p   = d_nextSubcontextId_p;

    // Stash the 'update' to be applied to the subcontext so that we can wait
    // to apply it after we have completely initialized the subcontext.

    const mwcstm::StatContextUpdate* update = newConfig.d_update_p;
    newConfig.d_update_p                    = 0;

    if (d_isTable) {
        newConfig.d_isTable = true;
        newConfig.d_valueDefs.clear();

        newContext = new (*d_allocator_p)
            StatContext(newConfig, d_allocator_p);

        // Share the valueVecPool with all subtables since they'll all have
        // the same value vectors
        newContext->d_valueVecPool_p = d_valueVecPool_p;

        newContext->d_valueDefs_p = d_valueDefs_p;
        newContext->initValues(newContext->d_directValues_p);
    }
    else {
        newConfig.d_statValueAllocator_p = d_statValueAllocator_p;

        if (!d_defaultHistorySizes.empty()) {
            newConfig.d_defaultHistorySizes = d_defaultHistorySizes;
        }

        newContext = new (*d_allocator_p)
            StatContext(newConfig, d_allocator_p);
    }

    if (update) {
        newContext->applyUpdate(*update);
    }

    if (0 == newContext->d_uniqueId) {
        newContext->d_uniqueId = (*d_nextSubcontextId_p)++;
    }

    bslma::ManagedPtr<StatContext> ret(newContext,
                                       d_allocator_p,
                                       &StatContext::statContextDeleter);

    bslmt::LockGuard<bslmt::Mutex> guard(&d_newSubcontextsLock);  // LOCK
    d_newSubcontexts.push_back(newContext);

    return ret;
}

void StatContext::snapshot()
{
    snapshotImp(bsls::TimeUtil::getTimer());
}

void StatContext::cleanup()
{
    bdlma::LocalSequentialAllocator<256> seqAlloc;
    bsl::vector<ValueVec*>               expiredVecs(&seqAlloc);

    cleanupImp(&expiredVecs);
}

void StatContext::clearValues()
{
    moveNewSubcontexts();
    for (StatContextMap::iterator iter = d_subcontexts.begin();
         iter != d_subcontexts.end();
         ++iter) {
        iter->second->clearValues();
    }

    if (d_totalValues_p) {
        for (size_t i = 0; i < d_totalValues_p->size(); ++i) {
            (*d_totalValues_p)[i].clear(0);
        }
    }

    if (d_activeChildrenTotalValues_p) {
        for (size_t i = 0; i < d_activeChildrenTotalValues_p->size(); ++i) {
            (*d_activeChildrenTotalValues_p)[i].clear(0);
        }
    }

    if (d_directValues_p) {
        for (size_t i = 0; i < d_directValues_p->size(); ++i) {
            (*d_directValues_p)[i].clear(0);
        }
    }

    if (d_expiredValues_p) {
        for (size_t i = 0; i < d_expiredValues_p->size(); ++i) {
            (*d_expiredValues_p)[i].clear(0);
        }
    }
}

void StatContext::snapshotFromUpdate(const mwcstm::StatContextUpdate& update)
{
    applyUpdate(update);
    snapshotImp(convertFromEpoch(update.timeStamp()));
}

void StatContext::clearSubcontexts()
{
    moveNewSubcontexts();

    for (StatContextMap::iterator iter = d_subcontexts.begin();
         iter != d_subcontexts.end();
         ++iter) {
        if (iter->second->d_released.swap(true)) {
            // Subcontext has no external references, so we can safely delete
            // it.

            d_allocator_p->deleteObject(iter->second);
        }
    }
    d_subcontexts.clear();

    // If we're tracking updates, clearing the deleted subcontexts will try to
    // clean out the list in the update.  Since we're just clearing out
    // everything, it's better to temporarily unset the update collector so
    // that no incremental cleanup is performed, and then just clear it out
    // after that's done.

    mwcstm::StatContextUpdate* update = d_update_p;
    d_update_p                        = 0;

    clearDeletedSubcontexts(0);
    d_subcontextsById.clear();

    d_update_p = update;
    if (d_update_p) {
        d_update_p->subcontexts().clear();
    }
}

// ACCESSORS
int StatContext::valueIndex(const bslstl::StringRef& name) const
{
    for (size_t i = 0; i < d_valueDefs_p->size(); ++i) {
        if ((*d_valueDefs_p)[i].d_name == name) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void StatContext::initializeUpdate(mwcstm::StatContextUpdate* update) const
{
    update->id()    = d_uniqueId;
    update->flags() = bdlb::BitUtil::withBitSet(
        0u,
        mwcstm::StatContextUpdateFlags::DMCSTM_CONTEXT_CREATED);

    mwcstm::StatContextConfiguration& config =
        update->configuration().makeValue();
    config.flags() = 0;
    if (isTable()) {
        config.flags() = bdlb::BitUtil::withBitSet(
            config.flags(),
            mwcstm::StatContextConfigurationFlags::DMCSTM_IS_TABLE);
    }
    if (d_storeExpiredValues) {
        config.flags() = bdlb::BitUtil::withBitSet(
            config.flags(),
            mwcstm::StatContextConfigurationFlags::
                DMCSTM_STORE_EXPIRED_VALUES);
    }

    if (hasName()) {
        config.choice().makeName() = name();
    }
    else {
        config.choice().makeId() = id();
    }
    config.values().resize(numValues());
    update->directValues().resize(numValues());
    if (d_storeExpiredValues) {
        update->expiredValues().resize(numValues());
    }

    for (int i = 0; i < numValues(); ++i) {
        mwcstm::StatValueDefinition& definition = config.values()[i];
        const StatValue& val = value(StatContext::DMCST_DIRECT_VALUE, i);
        definition.name()    = valueName(i);
        definition.type()    = static_cast<mwcstm::StatValueType::Value>(
            val.type());
        definition.historySizes().resize(val.numLevels());
        for (int l = 0; l < val.numLevels(); ++l) {
            definition.historySizes()[l] = val.historySize(l);
        }
    }
}

void StatContext::loadFullUpdate(mwcstm::StatContextUpdate* update,
                                 int valueFieldMask) const
{
    initializeUpdate(update);

    if (d_directValues_p) {
        // 'context' doesn't store a timestamp, but the timestamp of each
        // value's latest snapshot is the same, so we just grab that.

        update->timeStamp() = convertToEpoch(
            value(StatContext::DMCST_DIRECT_VALUE, 0)
                .snapshot(0)
                .snapshotTime());

        loadUpdatesFromValues(&update->directValues(),
                              d_directValues_p.ptr(),
                              valueFieldMask,
                              true);
        loadUpdatesFromValues(&update->expiredValues(),
                              d_expiredValues_p.ptr(),
                              valueFieldMask,
                              true);
    }

    update->subcontexts().resize(numSubcontexts());
    StatContextIterator it = subcontextIterator();
    for (int i = 0; i < numSubcontexts(); ++i) {
        BSLS_ASSERT(it);
        it->loadFullUpdate(&(update->subcontexts()[i]), valueFieldMask);
        ++it;
    }
}

bsl::ostream&
StatContext::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    if (d_id.is<bsl::string>()) {
        printer.printAttribute("Name", d_id.the<bsl::string>());
    }
    else {
        printer.printAttribute("Id", d_id.the<bsls::Types::Int64>());
    }

    printer.printAttribute("Subcontexts", d_subcontexts);
    printer.printAttribute("DeletedSubcontexts", d_deletedSubcontexts);
    printer.printAttribute("NewSubcontexts", d_newSubcontexts);
    printer.end();

    return stream;
}

// ------------------------------
// class StatContextConfiguration
// ------------------------------

// CREATORS
StatContextConfiguration::StatContextConfiguration(
    const mwcstm::StatContextUpdate& update,
    bslma::Allocator*                basicAllocator)
: d_id(basicAllocator)
, d_uniqueId(0)
, d_valueDefs(basicAllocator)
, d_isTable(false)
, d_userData_p()
, d_storeExpiredSubcontextValues(false)
, d_preSnapshotCallback(bsl::allocator_arg, basicAllocator)
, d_update_p(&update)
, d_updateCollector_p(0)
, d_updateValueFieldMask(0)
, d_nextSubcontextId_p()
, d_statValueAllocator_p(0)
{
    BSLS_ASSERT(!update.configuration().isNull());
    BSLS_ASSERT(bdlb::BitUtil::isBitSet(
        update.flags(),
        mwcstm::StatContextUpdateFlags::DMCSTM_CONTEXT_CREATED));
    d_uniqueId = update.id();

    const mwcstm::StatContextConfiguration& config =
        update.configuration().value();

    if (config.choice().isNameValue()) {
        d_id.createInPlace<bsl::string>(config.choice().name());
    }
    else {
        d_id.createInPlace<bsls::Types::Int64>(config.choice().id());
    }

    d_isTable = bdlb::BitUtil::isBitSet(
        config.flags(),
        mwcstm::StatContextConfigurationFlags::DMCSTM_IS_TABLE);
    d_storeExpiredSubcontextValues = bdlb::BitUtil::isBitSet(
        config.flags(),
        mwcstm::StatContextConfigurationFlags::DMCSTM_STORE_EXPIRED_VALUES);

    d_valueDefs.resize(config.values().size());
    for (bsl::size_t i = 0; i < d_valueDefs.size(); ++i) {
        const mwcstm::StatValueDefinition& definition = config.values()[i];
        d_valueDefs[i].d_name                         = definition.name();
        d_valueDefs[i].d_type = static_cast<StatValue::Type>(
            definition.type());
        d_valueDefs[i].d_sizes.assign(definition.historySizes().begin(),
                                      definition.historySizes().end());
    }
}

}  // close package namespace
}  // close enterprise namespace
