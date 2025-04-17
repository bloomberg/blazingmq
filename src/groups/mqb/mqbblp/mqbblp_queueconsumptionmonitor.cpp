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
#include <mqbcmd_messages.h>
#include <mqbi_queueengine.h>
#include <mqbi_storage.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

#include <bmqtsk_alarmlog.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

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
    stream << bmqu::PrintUtil::indent(level, spacesPerLevel)
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
    stream << bmqu::PrintUtil::indent(level, spacesPerLevel)
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

QueueConsumptionMonitor::SubStreamInfo::SubStreamInfo()
: d_lastKnownGoodTimer(0)
, d_messageSent(true)
, d_state(State::e_ALIVE)
{
    // NOTHING
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
        iter->second = SubStreamInfo();
    }

    return *this;
}

void QueueConsumptionMonitor::registerSubStream(const bsl::string& id)
{
    // Should always be called from the queue thread, but will be invoked from
    // the cluster thread once upon queue creation.

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_subStreamInfos.find(id) == d_subStreamInfos.end());

    d_subStreamInfos.insert(bsl::make_pair(id, SubStreamInfo()));
}

void QueueConsumptionMonitor::unregisterSubStream(const bsl::string& id)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    SubStreamInfoMapConstIter iter = d_subStreamInfos.find(id);
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

    return;  // RETURN

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_maxIdleTime == 0)) {
        // monitoring is disabled
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return;  // RETURN
    }

    BSLS_ASSERT_SAFE(currentTimer >= d_currentTimer);


    BALL_LOG_WARN << "QueueConsumptionMonitor::onTimer d_currentTimer: " << d_currentTimer
                  << ", currentTimer: " << currentTimer <<  ", d_maxIdleTime: " << d_maxIdleTime << '\n';

    d_currentTimer = currentTimer;

    for (SubStreamInfoMapIter iter = d_subStreamInfos.begin(),
                              last = d_subStreamInfos.end();
         iter != last;
         ++iter) {
        SubStreamInfo&          info   = iter->second;
        const bsl::string&      id     = iter->first;
        if (info.d_messageSent) {
            // Queue is 'alive' because at least one message was sent
            // since the last 'timer'.

            BALL_LOG_WARN << "d_messageSent is set!!!!!\n";
            info.d_messageSent        = false;
            info.d_lastKnownGoodTimer = d_currentTimer;

            if (info.d_state == State::e_IDLE) {
                // object was in idle state
                onTransitionToAlive(&info, id);
                BALL_LOG_WARN << "onTimer after onTransitionToAlive, continue\n";
                continue;  // CONTINUE
            }

            BSLS_ASSERT_SAFE(info.d_state == State::e_ALIVE);
            BALL_LOG_WARN << "onTimer continue\n";
            continue;  // CONTINUE
        }

        if (d_currentTimer - info.d_lastKnownGoodTimer > d_maxIdleTime) {

            BALL_LOG_WARN << "maxIdleTime EXCEEDED, call ALARM";

            // No delivered messages in the last 'maxIdleTime'.

            // Call callback to log alarm if there are undelivered messages.
            const bool haveUndelivered = d_loggingCb(id,
                                                     info.d_state ==
                                                         State::e_ALIVE);

            if (haveUndelivered) {
                // There are undelivered messages, transition to idle.
                if (info.d_state == State::e_ALIVE) {
                    info.d_state = State::e_IDLE;
                }
            }
            else {
                // The queue is at its head (no more
                // messages to deliver to this substream),
                // so transition to alive.
                if (info.d_state == State::e_IDLE) {
                    info.d_lastKnownGoodTimer = d_currentTimer;
                    onTransitionToAlive(&info, id);
                }
            }
        }
        else {
            BALL_LOG_WARN << "maxIdleTime NOT exceeded, skip alarmn";
        }
    }
}

void QueueConsumptionMonitor::onTransitionToAlive(SubStreamInfo* subStreamInfo,
                                                  const bsl::string& id)
{
    // executed by the *QUEUE DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_queueState_p->queue()->dispatcher()->inDispatcherThread(
        d_queueState_p->queue()));

    subStreamInfo->d_state = State::e_ALIVE;

    bdlma::LocalSequentialAllocator<2048> localAllocator(0);

    bmqt::UriBuilder uriBuilder(d_queueState_p->uri(), &localAllocator);
    uriBuilder.setId(id);

    bmqt::Uri uri(&localAllocator);
    uriBuilder.uri(&uri);

    BALL_LOG_INFO << "Queue '" << uri << "' no longer appears to be stuck.";
}

}  // close package namespace
}  // close enterprise namespace
