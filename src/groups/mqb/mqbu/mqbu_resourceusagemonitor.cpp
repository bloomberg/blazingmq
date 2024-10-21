// Copyright 2016-2023 Bloomberg Finance L.P.
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

// mqbu_resourceusagemonitor.cpp                                      -*-C++-*-
#include <mqbu_resourceusagemonitor.h>

#include <mqbscm_version.h>

#include <bmqu_printutil.h>

// BDE
#include <bdlb_print.h>
#include <bsl_iostream.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace mqbu {

namespace {

const double k_LO_HI_THRESHOLD_RATIO = 0.50;
// Ratio of the low watermark threshold ratio to the high
// watermark threshold ratio, i.e. lowWatermarkRatio
// = highWatermarkRatio * k_LO_HI_THRESHOLD_RATIO
const double k_DEFAULT_HI_THRESHOLD = 0.80;
// Default high threshold ratio, in percent of the capacity

/// Load into the specified `watermarkRatio` the ratio of the specified
/// `watermark` to the specified `capacity`.  Behavior is undefined unless
/// `0 <= watermark <= capacity`.
void computeWatermarkRatio(double*            watermarkRatio,
                           bsls::Types::Int64 capacity,
                           bsls::Types::Int64 watermark)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(watermarkRatio);
    BSLS_ASSERT_SAFE(0 <= watermark && watermark <= capacity);

    if (capacity == 0) {
        *watermarkRatio = 0.0;
    }
    else {
        *watermarkRatio = static_cast<double>(watermark) / capacity;
    }
}

}  // close unnamed namespace

// --------------------------------
// struct ResourceUsageMonitorState
// --------------------------------

// CLASS METHODS
bsl::ostream&
ResourceUsageMonitorState::print(bsl::ostream&                   stream,
                                 ResourceUsageMonitorState::Enum value,
                                 int                             level,
                                 int spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ResourceUsageMonitorState::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char*
ResourceUsageMonitorState::toAscii(ResourceUsageMonitorState::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(STATE_NORMAL)
        CASE(STATE_HIGH_WATERMARK)
        CASE(STATE_FULL)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ------------------------------------------
// struct ResourceUsageMonitorStateTransition
// ------------------------------------------

// CLASS METHODS
bsl::ostream& ResourceUsageMonitorStateTransition::print(
    bsl::ostream&                             stream,
    ResourceUsageMonitorStateTransition::Enum value,
    int                                       level,
    int                                       spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << ResourceUsageMonitorStateTransition::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* ResourceUsageMonitorStateTransition::toAscii(
    ResourceUsageMonitorStateTransition::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(NO_CHANGE)
        CASE(LOW_WATERMARK)
        CASE(HIGH_WATERMARK)
        CASE(FULL)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// --------------------------
// class ResourceUsageMonitor
// --------------------------

// PRIVATE MANIPULATORS
ResourceUsageMonitorStateTransition::Enum
ResourceUsageMonitor::updateValueInternal(ResourceAttributes* attributes,
                                          bsls::Types::Int64  delta)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(attributes);

    // TYPES
    typedef ResourceUsageMonitorState           RUMState;
    typedef ResourceUsageMonitorStateTransition RUMStateTransition;

    RUMStateTransition::Enum ret          = RUMStateTransition::e_NO_CHANGE;
    const bsls::Types::Int64 lowWatermark = attributes->capacity() *
                                            attributes->lowWatermarkRatio();
    const bsls::Types::Int64 highWatermark = attributes->capacity() *
                                             attributes->highWatermarkRatio();

    attributes->setValue(attributes->value() + delta);

    // NOTE: What are the probabilities of the second level if statements?
    //       Let's consider a case where batch processing is in place such that
    //       we update bytes and messages multiple times before reaching the
    //       limit/capacity parameters. Then the common case is that we do not
    //       undergo a state transition and none of the second-level if
    //       statements hit true.
    if (delta > 0) {
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                attributes->state() == RUMState::e_STATE_NORMAL &&
                attributes->value() >= highWatermark)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // 'NORMAL' -> 'HIGH_WATERMARK'
            attributes->setState(RUMState::e_STATE_HIGH_WATERMARK);
            ret = RUMStateTransition::e_HIGH_WATERMARK;
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                attributes->state() == RUMState::e_STATE_HIGH_WATERMARK &&
                attributes->value() >= attributes->capacity())) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // 'HIGH_WATERMARK' -> 'FULL'
            attributes->setState(RUMState::e_STATE_FULL);
            ret = RUMStateTransition::e_FULL;
        }
    }
    else if (delta < 0) {
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(attributes->capacity() ==
                                                  0)) {
            // Capacity is set to 0. In other circumstances there might be a
            // change of state if the new value crosses from HIGH/FULL boundary
            // limit into low watermark.  However, in this specific case it
            // only logically makes sense to circumvent that logic and keep the
            // state full.
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            BSLS_ASSERT_SAFE(attributes->state() ==
                             mqbu::ResourceUsageMonitorState::e_STATE_FULL);

            return RUMStateTransition::e_NO_CHANGE;  // RETURN
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                attributes->state() == RUMState::e_STATE_FULL &&
                attributes->value() <= highWatermark)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // 'FULL' -> 'HIGH_WATERMARK'
            attributes->setState(RUMState::e_STATE_HIGH_WATERMARK);
            // no state change notification
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                attributes->state() == RUMState::e_STATE_HIGH_WATERMARK &&
                attributes->value() <= lowWatermark)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            // 'HIGH_WATERMARK' -> 'NORMAL'
            attributes->setState(RUMState::e_STATE_NORMAL);
            ret = RUMStateTransition::e_LOW_WATERMARK;
        }
    }

    return ret;
}

// CREATORS
ResourceUsageMonitor::ResourceUsageMonitor(bsls::Types::Int64 byteCapacity,
                                           bsls::Types::Int64 messageCapacity)
: d_bytes(byteCapacity,
          k_DEFAULT_HI_THRESHOLD * k_LO_HI_THRESHOLD_RATIO,
          k_DEFAULT_HI_THRESHOLD,
          0,
          ResourceUsageMonitorState::e_STATE_NORMAL)
, d_messages(messageCapacity,
             k_DEFAULT_HI_THRESHOLD * k_LO_HI_THRESHOLD_RATIO,
             k_DEFAULT_HI_THRESHOLD,
             0,
             ResourceUsageMonitorState::e_STATE_NORMAL)
{
    // If the byte and/or message-capacity is zero, explicitly set its state to
    // full.
    if (0 == byteCapacity) {
        d_bytes.setState(ResourceUsageMonitorState::e_STATE_FULL);
    }

    if (0 == messageCapacity) {
        d_messages.setState(ResourceUsageMonitorState::e_STATE_FULL);
    }
}

ResourceUsageMonitor::ResourceUsageMonitor(bsls::Types::Int64 byteCapacity,
                                           bsls::Types::Int64 messageCapacity,
                                           double byteLowWatermarkRatio,
                                           double byteHighWatermarkRatio,
                                           double messageLowWatermarkRatio,
                                           double messageHighWatermarkRatio)
: d_bytes(byteCapacity,
          byteLowWatermarkRatio,
          byteHighWatermarkRatio,
          0,
          ResourceUsageMonitorState::e_STATE_NORMAL)
, d_messages(messageCapacity,
             messageLowWatermarkRatio,
             messageHighWatermarkRatio,
             0,
             ResourceUsageMonitorState::e_STATE_NORMAL)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0.0 <= byteLowWatermarkRatio &&
                     byteLowWatermarkRatio <= byteHighWatermarkRatio &&
                     byteHighWatermarkRatio <= 1.0);
    BSLS_ASSERT_SAFE(0.0 <= messageLowWatermarkRatio &&
                     messageLowWatermarkRatio <= messageHighWatermarkRatio &&
                     messageHighWatermarkRatio <= 1.0);

    // If the byte and/or message-capacity is zero, explicitly set its state to
    // full.
    if (0 == byteCapacity) {
        d_bytes.setState(ResourceUsageMonitorState::e_STATE_FULL);
    }

    if (0 == messageCapacity) {
        d_messages.setState(ResourceUsageMonitorState::e_STATE_FULL);
    }
}

// MANIPULATORS
void ResourceUsageMonitor::reset()
{
    d_bytes.setValue(0);
    d_bytes.setState(ResourceUsageMonitorState::e_STATE_NORMAL);

    d_messages.setValue(0);
    d_messages.setState(ResourceUsageMonitorState::e_STATE_NORMAL);
}

void ResourceUsageMonitor::reset(bsls::Types::Int64 byteCapacity,
                                 bsls::Types::Int64 messageCapacity,
                                 bsls::Types::Int64 byteLowWatermark,
                                 bsls::Types::Int64 byteHighWatermark,
                                 bsls::Types::Int64 messageLowWatermark,
                                 bsls::Types::Int64 messageHighWatermark)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0.0 <= byteLowWatermark &&
                     byteLowWatermark <= byteHighWatermark &&
                     byteHighWatermark <= byteCapacity);
    BSLS_ASSERT_SAFE(0.0 <= messageLowWatermark &&
                     messageLowWatermark <= messageHighWatermark &&
                     messageHighWatermark <= messageCapacity);

    double byteLowWatermarkRatio;
    double byteHighWatermarkRatio;
    double messageLowWatermarkRatio;
    double messageHighWatermarkRatio;

    computeWatermarkRatio(&byteLowWatermarkRatio,
                          byteCapacity,
                          byteLowWatermark);

    computeWatermarkRatio(&byteHighWatermarkRatio,
                          byteCapacity,
                          byteHighWatermark);

    computeWatermarkRatio(&messageLowWatermarkRatio,
                          messageCapacity,
                          messageLowWatermark);

    computeWatermarkRatio(&messageHighWatermarkRatio,
                          messageCapacity,
                          messageHighWatermark);

    resetByRatio(byteCapacity,
                 messageCapacity,
                 byteLowWatermarkRatio,
                 byteHighWatermarkRatio,
                 messageLowWatermarkRatio,
                 messageHighWatermarkRatio);
}

void ResourceUsageMonitor::resetByRatio(bsls::Types::Int64 byteCapacity,
                                        bsls::Types::Int64 messageCapacity,
                                        double byteLowWatermarkRatio,
                                        double byteHighWatermarkRatio,
                                        double messageLowWatermarkRatio,
                                        double messageHighWatermarkRatio)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0.0 <= byteLowWatermarkRatio &&
                     byteLowWatermarkRatio <= byteHighWatermarkRatio &&
                     byteHighWatermarkRatio <= 1.0);
    BSLS_ASSERT_SAFE(0.0 <= messageLowWatermarkRatio &&
                     messageLowWatermarkRatio <= messageHighWatermarkRatio &&
                     messageHighWatermarkRatio <= 1.0);

    reset();

    d_bytes.setWatermarkRatios(byteLowWatermarkRatio, byteHighWatermarkRatio);
    d_bytes.setCapacity(byteCapacity);

    d_messages.setWatermarkRatios(messageLowWatermarkRatio,
                                  messageHighWatermarkRatio);
    d_messages.setCapacity(messageCapacity);
}

void ResourceUsageMonitor::reconfigure(bsls::Types::Int64 byteCapacity,
                                       bsls::Types::Int64 messageCapacity,
                                       bsls::Types::Int64 byteLowWatermark,
                                       bsls::Types::Int64 byteHighWatermark,
                                       bsls::Types::Int64 messageLowWatermark,
                                       bsls::Types::Int64 messageHighWatermark)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0.0 <= byteLowWatermark &&
                     byteLowWatermark <= byteHighWatermark &&
                     byteHighWatermark <= byteCapacity);
    BSLS_ASSERT_SAFE(0.0 <= messageLowWatermark &&
                     messageLowWatermark <= messageHighWatermark &&
                     messageHighWatermark <= messageCapacity);

    double byteLowWatermarkRatio;
    double byteHighWatermarkRatio;
    double messageLowWatermarkRatio;
    double messageHighWatermarkRatio;

    computeWatermarkRatio(&byteLowWatermarkRatio,
                          byteCapacity,
                          byteLowWatermark);

    computeWatermarkRatio(&byteHighWatermarkRatio,
                          byteCapacity,
                          byteHighWatermark);

    computeWatermarkRatio(&messageLowWatermarkRatio,
                          messageCapacity,
                          messageLowWatermark);

    computeWatermarkRatio(&messageHighWatermarkRatio,
                          messageCapacity,
                          messageHighWatermark);

    reconfigureByRatio(byteCapacity,
                       messageCapacity,
                       byteLowWatermarkRatio,
                       byteHighWatermarkRatio,
                       messageLowWatermarkRatio,
                       messageHighWatermarkRatio);
}

void ResourceUsageMonitor::reconfigureByRatio(
    bsls::Types::Int64 byteCapacity,
    bsls::Types::Int64 messageCapacity,
    double             byteLowWatermarkRatio,
    double             byteHighWatermarkRatio,
    double             messageLowWatermarkRatio,
    double             messageHighWatermarkRatio)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0.0 <= byteLowWatermarkRatio &&
                     byteLowWatermarkRatio <= byteHighWatermarkRatio &&
                     byteHighWatermarkRatio <= 1.0);
    BSLS_ASSERT_SAFE(0.0 <= messageLowWatermarkRatio &&
                     messageLowWatermarkRatio <= messageHighWatermarkRatio &&
                     messageHighWatermarkRatio <= 1.0);

    // NOTE: the resulting state of this object is the same as that of creating
    //       a new 'mqbu::ResourceUsageMonitor' object and calling 'update()'
    //       on it with this object's bytes and messages.
    const bsls::Types::Int64 bytes    = d_bytes.value();
    const bsls::Types::Int64 messages = d_messages.value();

    resetByRatio(byteCapacity,
                 messageCapacity,
                 byteLowWatermarkRatio,
                 byteHighWatermarkRatio,
                 messageLowWatermarkRatio,
                 messageHighWatermarkRatio);

    update(bytes, messages);

    // If the byte and/or message-capacity is zero, explicitly set its state to
    // full.
    if (0 == byteCapacity) {
        d_bytes.setState(ResourceUsageMonitorState::e_STATE_FULL);
    }

    if (0 == messageCapacity) {
        d_messages.setState(ResourceUsageMonitorState::e_STATE_FULL);
    }
}

ResourceUsageMonitorStateTransition::Enum
ResourceUsageMonitor::update(bsls::Types::Int64 bytesDelta,
                             bsls::Types::Int64 messagesDelta)
{
    // TYPES
    typedef ResourceUsageMonitorStateTransition RUMStateTransition;
    typedef ResourceUsageMonitorState           RUMState;

    RUMState::Enum stateBefore = state();

    RUMStateTransition::Enum byteStateTransition    = updateBytes(bytesDelta);
    RUMStateTransition::Enum messageStateTransition = updateMessages(
        messagesDelta);

    RUMState::Enum stateAfter = state();

    // An individual resource's state might have changed and yet the overall
    // state of the monitor has not.
    if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(stateBefore == stateAfter)) {
        // Monitor state has not changed
        return RUMStateTransition::e_NO_CHANGE;  // RETURN
    }

    BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

    // Monitor state has changed. At the time of this writing, this necessarily
    // means that there was a state transition in bytes, messages, or both.  We
    // return the change in state of the monitor with respect to the highest
    // limit reached for either bytes or messages.
    // NOTE: Below assumes that the ResourceUsageMonitorStateTransition Enum is
    //       in increasing order of limit reached (after 'e_NO_CHANGE' as it
    //       comes first)
    return bsl::max(byteStateTransition, messageStateTransition);
}

// ACCESSORS
bsl::ostream& ResourceUsageMonitor::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    stream << bmqu::PrintUtil::newlineAndIndent(level, spacesPerLevel)
           << state() << " " << "[Messages (" << messageState()
           << "): " << bmqu::PrintUtil::prettyNumber(messages()) << " ("
           << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                  d_messages.lowWatermarkRatio() * d_messages.capacity()))
           << " - "
           << bmqu::PrintUtil::prettyNumber(static_cast<bsls::Types::Int64>(
                  d_messages.highWatermarkRatio() * d_messages.capacity()))
           << " - " << bmqu::PrintUtil::prettyNumber(d_messages.capacity())
           << "), " << "Bytes (" << byteState()
           << "): " << bmqu::PrintUtil::prettyBytes(bytes()) << " ("
           << bmqu::PrintUtil::prettyBytes(d_bytes.lowWatermarkRatio() *
                                               d_bytes.capacity(),
                                           2)
           << " - "
           << bmqu::PrintUtil::prettyBytes(d_bytes.highWatermarkRatio() *
                                               d_bytes.capacity(),
                                           2)
           << " - " << bmqu::PrintUtil::prettyBytes(d_bytes.capacity())
           << ")] ";

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
