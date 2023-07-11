// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbu_capacitymeter.cpp                                             -*-C++-*-
#include <mqbu_capacitymeter.h>

#include <mqbscm_version.h>
// MWC
#include <mwctsk_alarmlog.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>

// BDE
#include <bdlb_print.h>
#include <bsl_algorithm.h>
#include <bsl_ostream.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace mqbu {

namespace {
const double k_LO_HI_THRESHOLD_RATIO = 0.50;
// Ratio of the low watermark threshold ratio to the
// high watermark threshold ratio, i.e.
// lowWatermarkRatio =   highWatermarkRatio
//                     * k_LO_HI_THRESHOLD_RATIO
const double k_DEFAULT_HI_THRESHOLD = 0.80;
// Default high threshold ratio, in percent of the
// capacity
const bsls::Types::Int64 k_ZERO = 0;

}  // close unnamed namespace

// -------------------
// class CapacityMeter
// -------------------

void CapacityMeter::logOnMonitorStateTransition(
    ResourceUsageMonitorStateTransition::Enum stateTransition) const
{
    const size_t k_INITIAL_BUFFER_SIZE = 256;

    mwcu::MemOutStream categoryStream(k_INITIAL_BUFFER_SIZE);
    mwcu::MemOutStream stream(k_INITIAL_BUFFER_SIZE);

    categoryStream << "CAPACITY_" << d_monitor.state();
    stream << "for '" << name() << "':";

    stream << " [Messages (" << d_monitor.messageState()
           << "): " << mwcu::PrintUtil::prettyNumber(d_monitor.messages())
           << " (limit: "
           << mwcu::PrintUtil::prettyNumber(d_monitor.messageCapacity());

    if (d_nbMessagesReserved > 0) {
        stream << ", reserved: "
               << mwcu::PrintUtil::prettyNumber(d_nbMessagesReserved);
    }

    stream << "), Bytes (" << d_monitor.byteState()
           << "): " << mwcu::PrintUtil::prettyBytes(d_monitor.bytes())
           << " (limit: "
           << mwcu::PrintUtil::prettyBytes(d_monitor.byteCapacity());
    if (d_nbBytesReserved > 0) {
        stream << ", reserved: "
               << mwcu::PrintUtil::prettyBytes(d_nbBytesReserved);
    }
    stream << ")]";

    switch (stateTransition) {
    case ResourceUsageMonitorStateTransition::e_HIGH_WATERMARK:
    case ResourceUsageMonitorStateTransition::e_FULL: {
        MWCTSK_ALARMLOG_RAW_ALARM(categoryStream.str())
            << stream.str() << MWCTSK_ALARMLOG_END;
    } break;
    case ResourceUsageMonitorStateTransition::e_LOW_WATERMARK: {
        BALL_LOG_INFO << "[" << categoryStream.str() << "] " << stream.str();
    } break;
    case ResourceUsageMonitorStateTransition::e_NO_CHANGE: {
        BSLS_ASSERT_SAFE(false &&
                         "Unexpected ResourceUsageMonitorStateTransition");
    } break;
    default: {
        BSLS_ASSERT_SAFE(false &&
                         "Unknown ResourceUsageMonitorStateTransition");
    }
    }
}

CapacityMeter::CapacityMeter(const bsl::string& name,
                             bslma::Allocator*  allocator)
: d_name(name, allocator)
, d_isDisabled(false)
, d_parent_p(0)
, d_monitor(0,
            0,
            k_DEFAULT_HI_THRESHOLD * k_LO_HI_THRESHOLD_RATIO,
            k_DEFAULT_HI_THRESHOLD,
            k_DEFAULT_HI_THRESHOLD * k_LO_HI_THRESHOLD_RATIO,
            k_DEFAULT_HI_THRESHOLD)
// will be reconfigured in setLimits
, d_nbMessagesReserved(0)
, d_nbBytesReserved(0)
, d_lock(bsls::SpinLock::s_unlocked)
{
    // NOTHING
}

CapacityMeter::CapacityMeter(const bsl::string& name,
                             CapacityMeter*     parent,
                             bslma::Allocator*  allocator)
: d_name(name, allocator)
, d_isDisabled(false)
, d_parent_p(parent)
, d_monitor(0,
            0,
            k_DEFAULT_HI_THRESHOLD * k_LO_HI_THRESHOLD_RATIO,
            k_DEFAULT_HI_THRESHOLD,
            k_DEFAULT_HI_THRESHOLD * k_LO_HI_THRESHOLD_RATIO,
            k_DEFAULT_HI_THRESHOLD)
// will be reconfigured in setLimits
, d_nbMessagesReserved(0)
, d_nbBytesReserved(0)
, d_lock()
{
    // NOTHING
}

CapacityMeter& CapacityMeter::setLimits(bsls::Types::Int64 messages,
                                        bsls::Types::Int64 bytes)
{
    // NOTE: ResourceUsageMonitor.reconfigure() will preserve the values for
    //       bytes and messages being monitored.
    d_monitor.reconfigureByRatio(bytes,
                                 messages,
                                 d_monitor.byteLowWatermarkRatio(),
                                 d_monitor.byteHighWatermarkRatio(),
                                 d_monitor.messageLowWatermarkRatio(),
                                 d_monitor.messageHighWatermarkRatio());

    return *this;
}

CapacityMeter& CapacityMeter::setWatermarkThresholds(double messages,
                                                     double bytes)
{
    // PRECONDITIONS
    BSLS_ASSERT(0.0 <= messages && messages <= 1.0);
    BSLS_ASSERT(0.0 <= bytes && bytes <= 1.0);

    // NOTE: ResourceUsageMonitor.reconfigure() will preserve the values for
    //       bytes and messages being monitored.
    d_monitor.reconfigureByRatio(d_monitor.byteCapacity(),
                                 d_monitor.messageCapacity(),
                                 bytes * k_LO_HI_THRESHOLD_RATIO,
                                 bytes,
                                 messages * k_LO_HI_THRESHOLD_RATIO,
                                 messages);

    return *this;
}

void CapacityMeter::reserve(bsls::Types::Int64* nbMessagesAvailable,
                            bsls::Types::Int64* nbBytesAvailable,
                            bsls::Types::Int64  messages,
                            bsls::Types::Int64  bytes)
{
    if (d_isDisabled) {
        // Not enabled, authorize what was requested
        *nbMessagesAvailable = messages;
        *nbBytesAvailable    = bytes;
        return;  // RETURN
    }

    bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK

    // First check with self how much resource is available
    *nbMessagesAvailable = bsl::min(messages,
                                    bsl::max(k_ZERO,
                                             d_monitor.messageCapacity() -
                                                 d_monitor.messages() -
                                                 d_nbMessagesReserved));
    *nbBytesAvailable    = bsl::min(bytes,
                                 bsl::max(k_ZERO,
                                          d_monitor.byteCapacity() -
                                              d_monitor.bytes() -
                                              d_nbBytesReserved));

    // If we have an associated parent, reserve resources on it (but at most
    // what is currently available in self, not what was requested by the user)
    if (d_parent_p) {
        messages = *nbMessagesAvailable;
        bytes    = *nbBytesAvailable;

        d_parent_p->reserve(nbMessagesAvailable,
                            nbBytesAvailable,
                            messages,
                            bytes);
    }

    d_nbMessagesReserved += *nbMessagesAvailable;
    d_nbBytesReserved += *nbBytesAvailable;
}

void CapacityMeter::release(bsls::Types::Int64 messages,
                            bsls::Types::Int64 bytes)
{
    if (d_isDisabled) {
        return;  // RETURN
    }

    if (d_parent_p) {
        d_parent_p->release(messages, bytes);
    }

    {
        bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK

        d_nbMessagesReserved -= messages;
        d_nbBytesReserved -= bytes;
    }  // close lock guard scope

    // POSTCONDITIONS: We should never be releasing more than was reserved
    BSLS_ASSERT_SAFE(d_nbMessagesReserved >= 0);
    BSLS_ASSERT_SAFE(d_nbBytesReserved >= 0);
}

void CapacityMeter::commit(bsls::Types::Int64 messages,
                           bsls::Types::Int64 bytes)
{
    if (d_isDisabled) {
        return;  // RETURN
    }

    // PRECONDITIONS: Since resources must always be reserved prior to being
    //                committed, we should never be requested to put more than
    //                has been reserved or more than the configured capacity.
    BSLS_ASSERT_SAFE(d_nbMessagesReserved >= messages);
    BSLS_ASSERT_SAFE(d_nbBytesReserved >= bytes);
    BSLS_ASSERT_SAFE(d_monitor.messages() + messages <=
                     d_monitor.messageCapacity());
    BSLS_ASSERT_SAFE(d_monitor.bytes() + bytes <= d_monitor.byteCapacity());

    {
        bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK

        // Update the reserved counters
        d_nbMessagesReserved -= messages;
        d_nbBytesReserved -= bytes;

        // Update monitor
        ResourceUsageMonitorStateTransition::Enum monitorStateTransition =
            d_monitor.update(bytes, messages);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                monitorStateTransition !=
                ResourceUsageMonitorStateTransition::e_NO_CHANGE)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            logOnMonitorStateTransition(monitorStateTransition);
        }
    }  // close lock guard scope

    if (d_parent_p) {
        d_parent_p->commit(messages, bytes);
    }
}

CapacityMeter::CommitResult
CapacityMeter::commitUnreserved(bsls::Types::Int64 messages,
                                bsls::Types::Int64 bytes)
{
    if (d_isDisabled) {
        return e_SUCCESS;  // RETURN
    }

    bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK

    // NOTE: The 'messages' and 'bytes' parameters are not considered in the
    //       'hasCapacity' check because we want to allow to exceed the
    //       capacity for either messages or bytes *exactly* once, which also
    //       means that we want to log on 'STATE_FULL' exactly once.
    bool hasMessagesCapacity = d_monitor.messages() + d_nbMessagesReserved <=
                               d_monitor.messageCapacity();
    bool hasBytesCapacity = d_monitor.bytes() + d_nbBytesReserved <=
                            d_monitor.byteCapacity();

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!hasMessagesCapacity ||
                                              !hasBytesCapacity)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Self doesn't have enough capacity
        return !hasMessagesCapacity ? e_LIMIT_MESSAGES
                                    : e_LIMIT_BYTES;  // RETURN
    }

    // Self has enough capacity, if it has a parent, try to acquire resources
    // on the parent.
    if (d_parent_p) {
        CommitResult res = d_parent_p->commitUnreserved(messages, bytes);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(res != e_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            return res;  // RETURN
        }
    }

    // Update monitor
    ResourceUsageMonitorStateTransition::Enum monitorStateTransition =
        d_monitor.update(bytes, messages);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            monitorStateTransition !=
            ResourceUsageMonitorStateTransition::e_NO_CHANGE)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        logOnMonitorStateTransition(monitorStateTransition);
    }

    return e_SUCCESS;
}

void CapacityMeter::forceCommit(bsls::Types::Int64 messages,
                                bsls::Types::Int64 bytes)
{
    if (d_isDisabled) {
        return;  // RETURN
    }

    {
        bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK

        d_monitor.update(bytes, messages);
    }  // close lock guard scope

    if (d_parent_p) {
        d_parent_p->forceCommit(messages, bytes);
    }
}

void CapacityMeter::remove(bsls::Types::Int64 messages,
                           bsls::Types::Int64 bytes,
                           bool               silentMode)
{
    if (d_isDisabled) {
        return;  // RETURN
    }

    if (d_parent_p) {
        d_parent_p->remove(messages, bytes);
    }

    {
        bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK

        // Update monitor
        ResourceUsageMonitorStateTransition::Enum monitorStateTransition =
            d_monitor.update(-1 * bytes, -1 * messages);

        bool isLowWatermarkTransition =
            monitorStateTransition ==
            ResourceUsageMonitorStateTransition::e_LOW_WATERMARK;

        if (!silentMode && isLowWatermarkTransition) {
            logOnMonitorStateTransition(monitorStateTransition);
        }
    }  // close lock guard scope
}

void CapacityMeter::clear()
{
    if (d_isDisabled) {
        return;  // RETURN
    }

    bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK

    if (d_parent_p) {
        d_parent_p->remove(d_monitor.messages(), d_monitor.bytes());
    }

    d_monitor.reset();
}

// ACCESSORS
bsl::ostream&
CapacityMeter::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bdlb::Print::newlineAndIndent(stream, level, spacesPerLevel);

    if (d_isDisabled) {
        stream << "[ ** DISABLED ** ]";
        return stream;  // RETURN
    }

    {
        bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK

        stream << name() << ":"
               << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
               << "Messages: [current: "
               << mwcu::PrintUtil::prettyNumber(d_monitor.messages())
               << ", limit: "
               << mwcu::PrintUtil::prettyNumber(d_monitor.messageCapacity());
        if (d_nbMessagesReserved != 0) {
            stream << ", reserved: "
                   << mwcu::PrintUtil::prettyNumber(d_nbMessagesReserved);
        }
        stream << "]"
               << mwcu::PrintUtil::newlineAndIndent(level + 1, spacesPerLevel)
               << "Bytes   : [current: "
               << mwcu::PrintUtil::prettyBytes(d_monitor.bytes())
               << ", limit: "
               << mwcu::PrintUtil::prettyBytes(d_monitor.byteCapacity());
        if (d_nbBytesReserved != 0) {
            stream << ", reserved: "
                   << mwcu::PrintUtil::prettyBytes(d_nbBytesReserved);
        }
        stream << "]";
    }

    if (d_parent_p) {
        d_parent_p->print(stream, level, spacesPerLevel);
    }

    return stream;
}

bsl::ostream& CapacityMeter::printShortSummary(bsl::ostream& stream) const
{
    if (d_isDisabled) {
        stream << "** DISABLED **";
        return stream;  // RETURN
    }

    {
        bsls::SpinLockGuard guard(&d_lock);  // d_lock LOCK
        stream << "Messages [current: "
               << mwcu::PrintUtil::prettyNumber(d_monitor.messages()) << " / "
               << mwcu::PrintUtil::prettyNumber(d_monitor.messageCapacity())
               << "], Bytes [current: "
               << mwcu::PrintUtil::prettyBytes(d_monitor.bytes()) << " / "
               << mwcu::PrintUtil::prettyBytes(d_monitor.byteCapacity())
               << "]";
    }

    return stream;
}

// ------------------------
// struct CapacityMeterUtil
// ------------------------

void CapacityMeterUtil::loadState(mqbcmd::CapacityMeter* state,
                                  const CapacityMeter&   capacityMeter)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(state);

    state->name()                = capacityMeter.d_name;
    state->isDisabled()          = capacityMeter.d_isDisabled;
    state->numMessages()         = capacityMeter.d_monitor.messages();
    state->messageCapacity()     = capacityMeter.d_monitor.messageCapacity();
    state->numMessagesReserved() = capacityMeter.d_nbMessagesReserved;
    state->numBytes()            = capacityMeter.d_monitor.bytes();
    state->byteCapacity()        = capacityMeter.d_monitor.byteCapacity();
    state->numBytesReserved()    = capacityMeter.d_nbBytesReserved;

    if (capacityMeter.d_parent_p) {
        mqbcmd::CapacityMeter& parent = state->parent().makeValue();
        loadState(&parent, *capacityMeter.d_parent_p);
    }
}

}  // close package namespace
}  // close enterprise namespace
