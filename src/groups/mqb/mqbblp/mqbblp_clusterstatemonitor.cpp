// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mqbblp_clusterstatemonitor.cpp                                     -*-C++-*-
#include <mqbblp_clusterstatemonitor.h>

#include <mqbscm_version.h>
// MQB
#include <mqbc_clusterdata.h>
#include <mqbcmd_humanprinter.h>
#include <mqbcmd_messages.h>

// MWC
#include <mwcsys_time.h>
#include <mwctsk_alarmlog.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_functional.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_timeinterval.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbblp {

namespace {

const int k_CHECK_INTERVAL = 10;

}  // close unnamed namespace

// -----------
// class State
// -----------

ClusterStateMonitor::State::State()
: d_maxInvalid(0)
, d_maxThreshold(0)
, d_lastValid(0)
, d_lastThreshold(0)
, d_lastAlarm(0)
, d_state(e_INVALID)
{
    // NOTHING
}

ClusterStateMonitor::State::State(const bsls::TimeInterval& maxInvalid,
                                  const bsls::TimeInterval& maxThreshold,
                                  const bsls::TimeInterval& lastValid,
                                  const StateType&          state)
: d_maxInvalid(maxInvalid)
, d_maxThreshold(maxThreshold)
, d_lastValid(lastValid)
, d_lastThreshold(0)
, d_lastAlarm(0)
, d_state(state)
{
    // NOTHING
}

// -------------------------
// class ClusterStateMonitor
// -------------------------

void ClusterStateMonitor::notifyObserversIfNeededHelper(
    State*                                                  state,
    bool*                                                   shouldAlarm,
    const bsl::function<void(mqbc::ClusterStateObserver*)>& notificationCb,
    int                                                     thresholdTime,
    int                                                     maxTime,
    bsls::TimeInterval                                      now)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(dispatcherClient()));
    BSLS_ASSERT_SAFE(state);

    if (state->d_state == e_THRESHOLD || state->d_state == e_ALARMING) {
        // Emit threshold notification?
        if ((now - state->d_lastThreshold) >= thresholdTime) {
            for (ObserversSet::iterator iter = d_observers.begin();
                 iter != d_observers.end();
                 ++iter) {
                notificationCb(*iter);
            }
            state->d_lastThreshold = now;
        }

        // Emit periodic alarm?
        if ((state->d_state == e_ALARMING) &&
            (now - state->d_lastAlarm) >= maxTime) {
            *shouldAlarm       = true;
            state->d_lastAlarm = now;
        }
    }
}

// PRIVATE MANIPULATORS
void ClusterStateMonitor::notifyObserversIfNeeded()
{
    // invoked in the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(dispatcherClient()));

    const bsls::TimeInterval now = mwcsys::Time::nowMonotonicClock();

    // We do 2 things here:
    //   1. Periodically invoke threshold callbacks (onLeaderPassiveThreshold,
    //      etc.) as long as applicable
    //   2. Periodically continue to alarm "cluster still in bad state" as long
    //      as applicable.

    bool shouldAlarm = false;

    const mqbcfg::ClusterMonitorConfig& config =
        d_clusterData_p->clusterConfig().clusterMonitorConfig();

    // [1.1] Partitions primary state
    for (size_t i = 0; i != d_partitionStates.size(); ++i) {
        State* state = &d_partitionStates[i];
        bsl::function<void(mqbc::ClusterStateObserver*)> notificationCb =
            bdlf::BindUtil::bind(
                &mqbc::ClusterStateObserver::onPartitionOrphanThreshold,
                bdlf::PlaceHolders::_1,  // observer
                i);                      // partitionId
        notifyObserversIfNeededHelper(state,
                                      &shouldAlarm,
                                      notificationCb,
                                      config.thresholdMaster(),
                                      config.maxTimeMaster(),
                                      now);
    }

    // [1.2] Cluster nodes state
    const ClusterNodeSessionMap& nodeMap =
        d_clusterData_p->membership().clusterNodeSessionMap();
    for (ClusterNodeSessionMap::const_iterator cit = nodeMap.begin();
         cit != nodeMap.end();
         ++cit) {
        State* state = &d_nodeStates[cit->first->nodeId()];
        bsl::function<void(mqbc::ClusterStateObserver*)> notificationCb =
            bdlf::BindUtil::bind(
                &mqbc::ClusterStateObserver::onNodeUnavailableThreshold,
                bdlf::PlaceHolders::_1,  // observer
                cit->first);             // node
        notifyObserversIfNeededHelper(state,
                                      &shouldAlarm,
                                      notificationCb,
                                      config.thresholdNode(),
                                      config.maxTimeNode(),
                                      now);
    }

    // [1.3] Cluster leader state
    bsl::function<void(mqbc::ClusterStateObserver*)> notificationCb =
        bdlf::BindUtil::bind(
            &mqbc::ClusterStateObserver::onLeaderPassiveThreshold,
            bdlf::PlaceHolders::_1);  // observer

    notifyObserversIfNeededHelper(&d_leaderState,
                                  &shouldAlarm,
                                  notificationCb,
                                  config.thresholdLeader(),
                                  config.maxTimeLeader(),
                                  now);

    // [1.4] State of the failover process
    notificationCb = bdlf::BindUtil::bind(
        &mqbc::ClusterStateObserver::onFailoverThreshold,
        bdlf::PlaceHolders::_1);  // observer

    notifyObserversIfNeededHelper(&d_failoverState,
                                  &shouldAlarm,
                                  notificationCb,
                                  config.thresholdFailover(),
                                  config.maxTimeFailover(),
                                  now);

    // [2] Keep alarming that the cluster is still in bad state as long as
    //     applicable
    if (shouldAlarm) {
        mwcu::MemOutStream os;
        os << "'" << d_clusterData_p->identity().name() << "'"
           << " is still in a bad state.";
        MWCTSK_ALARMLOG_PANIC("CLUSTER_STATE_MONITOR")
            << os.str() << MWCTSK_ALARMLOG_END;
    }
}

ClusterStateMonitor::StateTransition
ClusterStateMonitor::checkAndUpdateState(State*             state,
                                         bool               isValid,
                                         bsls::TimeInterval now)
{
    // invoked in the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(dispatcherClient()));

    const StateType previousState = state->d_state;
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(isValid)) {
        state->d_state     = e_VALID;
        state->d_lastValid = now;
        if (previousState == e_ALARMING || previousState == e_THRESHOLD) {
            return e_HEALTHY;  // RETURN
        }
        else if (previousState == e_INVALID) {
            // State was invalid (but not yet alarming) and is now back to
            // normal, nothing to do.
        }
        return e_NO_CHANGE;  // RETURN
    }

    // isValid is false
    BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

    if (state->d_state == e_ALARMING) {
        // Already alarming nothing to do
        return e_NO_CHANGE;  // RETURN
    }

    const bsls::TimeInterval diff = now - state->d_lastValid;
    if (diff >= state->d_maxInvalid) {
        // we have to alarm, going from inactive -> alarming
        state->d_state     = e_ALARMING;
        state->d_lastAlarm = now;
        return e_BAD;  // RETURN
    }
    else if (diff >= state->d_maxThreshold) {
        // Either above threshold or reaching the threshold now
        if (state->d_state == e_THRESHOLD) {
            return e_NO_CHANGE;  // RETURN
        }
        state->d_state = e_THRESHOLD;
        return e_THRESHOLD_REACHED;  // RETURN
    }
    else {
        state->d_state = e_INVALID;
        return e_NO_CHANGE;  // RETURN
    }
}

void ClusterStateMonitor::verifyAllStates()
{
    // invoked in *ANY* thread

    dispatcher()->execute(
        bdlf::BindUtil::bind(&ClusterStateMonitor::verifyAllStatesDispatched,
                             this),
        dispatcherClient());
}

void ClusterStateMonitor::verifyAllStatesDispatched()
{
    // invoked in the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(dispatcherClient()));

    const bsls::TimeInterval now = mwcsys::Time::nowMonotonicClock();

    bool isCurrentlyHealthy = true;
    bool shouldAlarm        = false;
    bool reachedThreshold   = false;

    bool            status;
    State*          state;
    StateTransition stateTransition;

    // partitions primary state
    for (size_t i = 0; i != d_partitionStates.size(); ++i) {
        state              = &d_partitionStates[i];
        status             = d_clusterState_p->hasActivePrimary(i);
        isCurrentlyHealthy = isCurrentlyHealthy && status;
        stateTransition    = checkAndUpdateState(state, status, now);
        shouldAlarm        = shouldAlarm || (stateTransition == e_BAD);
    }

    const ClusterNodeSessionMap& nodeMap =
        d_clusterData_p->membership().clusterNodeSessionMap();
    // cluster nodes state
    for (ClusterNodeSessionMap::const_iterator cit = nodeMap.begin();
         cit != nodeMap.end();
         ++cit) {
        state  = &d_nodeStates[cit->first->nodeId()];
        status = ((cit->second->nodeStatus() ==
                   bmqp_ctrlmsg::NodeStatus::E_AVAILABLE)) ||
                 !cit->first->isAvailable();
        isCurrentlyHealthy = isCurrentlyHealthy && status;
        stateTransition    = checkAndUpdateState(state, status, now);
        shouldAlarm        = shouldAlarm || (stateTransition == e_BAD);
    }

    // cluster leader state
    {
        status             = d_clusterData_p->electorInfo().hasActiveLeader();
        isCurrentlyHealthy = isCurrentlyHealthy && status;
        stateTransition    = checkAndUpdateState(&d_leaderState, status, now);
        shouldAlarm        = shouldAlarm || (stateTransition == e_BAD);
        reachedThreshold   = (stateTransition == e_THRESHOLD_REACHED);
    }

    // failover state
    {
        status = !d_clusterData_p->cluster()->isFailoverInProgress();
        isCurrentlyHealthy = isCurrentlyHealthy && status;
        stateTransition  = checkAndUpdateState(&d_failoverState, status, now);
        shouldAlarm      = shouldAlarm || (stateTransition == e_BAD);
        reachedThreshold = (stateTransition == e_THRESHOLD_REACHED);
    }

    // Update state, etc.
    d_isHealthy = isCurrentlyHealthy;

    if (shouldAlarm && !d_hasAlarmed) {
        d_hasAlarmed = true;
        onMonitorStateChange(e_ALARMING);
    }
    else if (reachedThreshold && !d_hasAlarmed && !d_thresholdReached) {
        d_thresholdReached = true;
        onMonitorStateChange(e_THRESHOLD);
    }
    else if (d_isHealthy && (d_hasAlarmed || d_thresholdReached)) {
        d_thresholdReached = false;
        d_hasAlarmed       = false;
        onMonitorStateChange(e_VALID);
    }

    d_clusterData_p->stats().setHealthStatus(d_isHealthy);

    notifyObserversIfNeeded();
}

mqbi::Dispatcher* ClusterStateMonitor::dispatcher()
{
    return d_clusterData_p->dispatcherClientData().dispatcher();
}

mqbi::DispatcherClient* ClusterStateMonitor::dispatcherClient()
{
    return d_clusterData_p->cluster();
}

void ClusterStateMonitor::onMonitorStateChange(const StateType& state)
{
    // executed by the *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(dispatcherClient()));

    switch (state) {
    case ClusterStateMonitor::e_THRESHOLD: {
        mwcu::MemOutStream os;
        os << "'" << d_clusterData_p->identity().name() << "' has been in "
           << "invalid state above the threshold amount of time.\n";
        // Log only a summary in the alarm
        d_clusterData_p->cluster()->printClusterStateSummary(os, 0, 4);
        BALL_LOG_INFO << os.str();
    } break;  // BREAK
    case ClusterStateMonitor::e_ALARMING: {
        mwcu::MemOutStream os;
        os << "'" << d_clusterData_p->identity().name() << "' is in a bad "
           << "state.\n";
        // Log the entire cluster state in the alarm
        mqbcmd::Result        result;
        mqbcmd::ClusterResult clusterResult;
        d_clusterData_p->cluster()->loadClusterStatus(&clusterResult);
        if (clusterResult.isClusterStatusValue()) {
            result.makeClusterStatus(clusterResult.clusterStatus());
        }
        else {
            result.makeClusterProxyStatus(clusterResult.clusterProxyStatus());
        }
        mqbcmd::HumanPrinter::print(os, result, 0, 4);

        MWCTSK_ALARMLOG_PANIC("CLUSTER_STATE_MONITOR")
            << os.str() << MWCTSK_ALARMLOG_END;
    } break;  // BREAK
    case ClusterStateMonitor::e_VALID: {
        BALL_LOG_INFO << "'" << d_clusterData_p->identity().name() << "' "
                      << "is back to healthy state.";
    } break;  // BREAK
    case ClusterStateMonitor::e_INVALID:
    default: {
        // we should never be here
        BSLS_ASSERT_SAFE(false &&
                         "Cluster Monitor 'onMonitorStateChange' with invalid"
                         " state type");
    }
    }
}

// CREATORS
ClusterStateMonitor::ClusterStateMonitor(
    mqbc::ClusterData*        clusterData,
    const mqbc::ClusterState* clusterState,
    bslma::Allocator*         allocator)
: d_isStarted(false)
, d_isHealthy(false)
, d_hasAlarmed(false)
, d_thresholdReached(false)
, d_leaderState()
, d_nodeStates(allocator)
, d_partitionStates(allocator)
, d_failoverState()
, d_scheduler_p(clusterData->scheduler())
, d_eventHandle()
, d_clusterData_p(clusterData)
, d_clusterState_p(clusterState)
, d_observers(allocator)
{
    // PRECONDITIONS
    const mqbcfg::ClusterMonitorConfig& config =
        d_clusterData_p->clusterConfig().clusterMonitorConfig();

    BALL_LOG_INFO << "Cluster state monitor configuration: " << config;

    BSLS_ASSERT_SAFE(config.thresholdLeader() < config.maxTimeLeader() &&
                     "thresholdLeader must be less than maxTimeLeader");
    BSLS_ASSERT_SAFE(config.thresholdMaster() < config.maxTimeMaster() &&
                     "thresholdPrimary must be less than maxTimePrimary");
    BSLS_ASSERT_SAFE(config.thresholdNode() < config.maxTimeNode() &&
                     "thresholdNode must be less than maxTimeNode");
    BSLS_ASSERT_SAFE(config.thresholdFailover() < config.maxTimeFailover() &&
                     "thresholdFailover must be less than maxTimeFailover");
}

ClusterStateMonitor::~ClusterStateMonitor()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_isStarted &&
                     "stop() must be called before destruction");

    stop();
}

int ClusterStateMonitor::start(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!d_isStarted && "Already started");

    const bsls::TimeInterval now = mwcsys::Time::nowMonotonicClock();

    const mqbc::ClusterState::PartitionsInfo& partitions =
        d_clusterState_p->partitions();
    const mqbcfg::ClusterMonitorConfig& config =
        d_clusterData_p->clusterConfig().clusterMonitorConfig();

    for (size_t i = 0; i != partitions.size(); ++i) {
        State toInsert(bsls::TimeInterval(config.maxTimeMaster(), 0),
                       bsls::TimeInterval(config.thresholdMaster(), 0),
                       now,
                       e_INVALID);
        d_partitionStates.emplace_back(toInsert);
    }

    d_leaderState.d_maxInvalid = bsls::TimeInterval(config.maxTimeLeader(), 0);
    d_leaderState.d_maxThreshold = bsls::TimeInterval(config.thresholdLeader(),
                                                      0);
    d_leaderState.d_lastValid    = now;
    d_leaderState.d_state        = e_INVALID;

    const ClusterNodeSessionMap& nodeMap =
        d_clusterData_p->membership().clusterNodeSessionMap();

    for (ClusterNodeSessionMap::const_iterator cit = nodeMap.begin();
         cit != nodeMap.end();
         ++cit) {
        State toInsert(bsls::TimeInterval(config.maxTimeNode(), 0),
                       bsls::TimeInterval(config.thresholdNode(), 0),
                       now,
                       e_INVALID);
        d_nodeStates.insert(bsl::make_pair(cit->first->nodeId(), toInsert));
    }

    d_failoverState.d_maxInvalid = bsls::TimeInterval(config.maxTimeFailover(),
                                                      0);
    d_failoverState.d_maxThreshold =
        bsls::TimeInterval(config.thresholdFailover(), 0);
    d_failoverState.d_lastValid = now;
    d_failoverState.d_state     = e_INVALID;

    // Schedule the recurring event checking the states, and invoke it
    // immediately to update to the current states.
    d_scheduler_p->scheduleRecurringEvent(
        &d_eventHandle,
        bsls::TimeInterval(k_CHECK_INTERVAL, 0),
        bdlf::BindUtil::bind(&ClusterStateMonitor::verifyAllStates, this),
        bsls::TimeInterval(0));

    d_isStarted = true;

    // Cluster is in an un-healthy state (default value of d_isHealthy)
    d_clusterData_p->stats().setHealthStatus(d_isHealthy);

    return 0;
}

void ClusterStateMonitor::registerObserver(
    mqbc::ClusterStateObserver* observer)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(dispatcherClient()));

    d_observers.insert(observer);
}

void ClusterStateMonitor::unregisterObserver(
    mqbc::ClusterStateObserver* observer)
{
    // executed by the cluster *DISPATCHER* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(dispatcher()->inDispatcherThread(dispatcherClient()));

    d_observers.erase(observer);
}

void ClusterStateMonitor::stop()
{
    d_scheduler_p->cancelEventAndWait(d_eventHandle);
    d_isStarted = false;
}

}  // close package namespace
}  // close enterprise namespace
