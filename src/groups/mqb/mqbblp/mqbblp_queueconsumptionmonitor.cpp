// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbblp_queueconsumptionmonitor.cpp                                 -*-C++-*-
#include <mqbblp_queueconsumptionmonitor.h>

#include <mqbscm_version.h>
// MBQ
#include <mqbblp_queuehandlecatalog.h>
#include <mqbblp_queuestate.h>
#include <mqbcmd_messages.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

// MWC
#include <mwctsk_alarmlog.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <ball_record.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_iomanip.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_stdallocator.h>
#include <bsls_performancehint.h>
#include <bslstl_stringref.h>

namespace BloombergLP {
namespace mqbblp {

// -------------------------------------
// struct QueueConsumptionMonitor::State
// -------------------------------------

bsl::ostream& QueueConsumptionMonitor::State::print(
    bsl::ostream&                        stream,
    QueueConsumptionMonitor::State::Enum value,
    int                                  level,
    int                                  spacesPerLevel)
{
    stream << mwcu::PrintUtil::indent(level, spacesPerLevel)
           << QueueConsumptionMonitor::State::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* QueueConsumptionMonitor::State::toAscii(
    QueueConsumptionMonitor::State::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(ALIVE)
        CASE(IDLE)
    default: return "(* UNKNOWN *)";
    }

#undef case
}

// ------------------------------------------
// struct QueueConsumptionMonitor::Transition
// ------------------------------------------

bsl::ostream& QueueConsumptionMonitor::Transition::print(
    bsl::ostream&                             stream,
    QueueConsumptionMonitor::Transition::Enum value,
    int                                       level,
    int                                       spacesPerLevel)
{
    stream << mwcu::PrintUtil::indent(level, spacesPerLevel)
           << QueueConsumptionMonitor::Transition::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* QueueConsumptionMonitor::Transition::toAscii(
    QueueConsumptionMonitor::Transition::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(UNCHANGED)
        CASE(ALIVE)
        CASE(IDLE)
    default: return "(* UNKNOWN *)";
    }

#undef case
}

// ---------------------------------------------
// struct QueueConsumptionMonitor::SubStreamInfo
// ---------------------------------------------

QueueConsumptionMonitor::SubStreamInfo::SubStreamInfo(const HeadCb& headCb)
: d_lastKnownGoodTimer(0)
, d_messageSent(true)
, d_state(State::e_ALIVE)
, d_headCb(headCb)
{
    BSLS_ASSERT_SAFE(d_headCb);
}

QueueConsumptionMonitor::SubStreamInfo::SubStreamInfo(
    const SubStreamInfo& other)
: d_lastKnownGoodTimer(other.d_lastKnownGoodTimer)
, d_messageSent(other.d_messageSent)
, d_state(other.d_state)
, d_headCb(other.d_headCb)
{
    BSLS_ASSERT_SAFE(d_headCb);
}

// -----------------------------
// class QueueConsumptionMonitor
// -----------------------------

// CREATORS
QueueConsumptionMonitor::QueueConsumptionMonitor(QueueState*       queueState,
                                                 const LoggingCb&  loggingCb,
                                                 bslma::Allocator* allocator)
: d_queueState_p(queueState)
, d_maxIdleTime(0)
, d_currentTimer(0)
, d_subStreamInfos(allocator)
, d_loggingCb(loggingCb)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p);
    BSLS_ASSERT_SAFE(d_loggingCb);
}

// MANIPULATORS
QueueConsumptionMonitor&
QueueConsumptionMonitor::setMaxIdleTime(bsls::Types::Int64 value)
{
    // Should always be called from the queue thread, but will be invoked from
    // the cluster thread once upon queue creation.

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(value >= 0);

    d_maxIdleTime = value;

    for (SubStreamInfoMapIter iter = d_subStreamInfos.begin(),
                              last = d_subStreamInfos.end();
         iter != last;
         ++iter) {
        iter->second = SubStreamInfo(iter->second.d_headCb);
    }

    return *this;
}

void QueueConsumptionMonitor::registerSubStream(const mqbu::StorageKey& key,
                                                const HeadCb&           headCb)
{
    // Should always be called from the queue thread, but will be invoked from
    // the cluster thread once upon queue creation.

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(key != mqbu::StorageKey::k_NULL_KEY ||
                     d_subStreamInfos.empty());
    BSLS_ASSERT_SAFE(headCb);
    BSLS_ASSERT_SAFE(d_subStreamInfos.find(mqbu::StorageKey::k_NULL_KEY) ==
                     d_subStreamInfos.end());
    BSLS_ASSERT_SAFE(d_subStreamInfos.find(key) == d_subStreamInfos.end());

    d_subStreamInfos.insert(bsl::make_pair(key, SubStreamInfo(headCb)));
}

void QueueConsumptionMonitor::unregisterSubStream(const mqbu::StorageKey& key)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    SubStreamInfoMapConstIter iter = d_subStreamInfos.find(key);
    BSLS_ASSERT_SAFE(iter != d_subStreamInfos.end());
    d_subStreamInfos.erase(iter);
}

void QueueConsumptionMonitor::reset()
{
    // Should always be called from the queue thread, but will be invoked from
    // the cluster thread once upon queue creation.

    d_maxIdleTime  = 0;
    d_currentTimer = 0;
    d_subStreamInfos.clear();
}

void QueueConsumptionMonitor::onTimer(bsls::Types::Int64 currentTimer)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_maxIdleTime == 0)) {
        // monitoring is disabled
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(currentTimer >= d_currentTimer);

    d_currentTimer = currentTimer;

    // TBD: 'queue empty' is not the best condition to test.  The queue may
    // contain messages that have been sent but not yet confirmed.  A better
    // test would be to check whether the message iterator in the engine points
    // to the end of storage, but we don't have access to these.  A solution
    // would be to have QueueEngine::beforeMessageRemoved notify this monitor,
    // via a new method on this component. Not implemented yet because Engines
    // are about to undergo overhaul.

    for (SubStreamInfoMapIter iter = d_subStreamInfos.begin(),
                              last = d_subStreamInfos.end();
         iter != last;
         ++iter) {
        SubStreamInfo& info = iter->second;
        BSLS_ASSERT_SAFE(info.d_headCb);
        bslma::ManagedPtr<mqbi::StorageIterator> head = info.d_headCb();

        if (head) {
            if (head->atEnd()) {
                head.reset();
            }
        }
        if (info.d_messageSent || !head) {
            // Queue is 'alive' either because at least one message was sent
            // since the last 'timer', or the queue is at its head (no more
            // messages to deliver to this substream).

            info.d_messageSent        = false;
            info.d_lastKnownGoodTimer = d_currentTimer;

            if (info.d_state == State::e_IDLE) {
                // object was in idle state
                onTransitionToAlive(&(iter->second), iter->first);
                continue;  // CONTINUE
            }

            BALL_LOG_WARN << "info.d_messageSent || !head, d_messageSent: "
                          << info.d_messageSent;
            BSLS_ASSERT_SAFE(info.d_state == State::e_ALIVE);
            continue;  // CONTINUE
        }

        if (info.d_state == State::e_IDLE) {
            // state was already idle, nothing more to do
            continue;  // CONTINUE
        }

        BSLS_ASSERT_SAFE(info.d_state == State::e_ALIVE);

        if (d_currentTimer - info.d_lastKnownGoodTimer > d_maxIdleTime) {
            // No delivered messages in the last 'maxIdleTime'.
            onTransitionToIdle(&(iter->second), iter->first, head);
            continue;  // CONTINUE
        }
    }
}

void QueueConsumptionMonitor::onTransitionToAlive(
    SubStreamInfo*          subStreamInfo,
    const mqbu::StorageKey& appKey)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    subStreamInfo->d_state = State::e_ALIVE;

    bdlma::LocalSequentialAllocator<2048> localAllocator(0);

    bmqt::UriBuilder uriBuilder(d_queueState_p->uri(), &localAllocator);
    bsl::string      appId;

    if (!appKey.isNull() &&
        d_queueState_p->storage()->hasVirtualStorage(appKey, &appId)) {
        uriBuilder.setId(appId);
    }

    bmqt::Uri uri(&localAllocator);
    uriBuilder.uri(&uri);

    BALL_LOG_INFO << "Queue '" << uri << "' no longer appears to be stuck.";
}

void QueueConsumptionMonitor::onTransitionToIdle(
    SubStreamInfo*                                  subStreamInfo,
    const mqbu::StorageKey&                         appKey,
    const bslma::ManagedPtr<mqbi::StorageIterator>& head)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));
    BSLS_ASSERT_SAFE(d_loggingCb);

    subStreamInfo->d_state = State::e_IDLE;

    // Call logging callback to log alarm info.
    d_loggingCb(appKey, head);
}

}  // close package namespace
}  // close enterprise namespace
