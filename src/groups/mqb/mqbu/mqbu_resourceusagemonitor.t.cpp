// Copyright 2023 Bloomberg Finance L.P.
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

// mqbu_resourceusagemonitor.t.cpp                                    -*-C++-*-
#include <mqbu_resourceusagemonitor.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// BMQ
#include <bmqu_memoutstream.h>

// BDE
#include <bsls_types.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

/// Increment bytes and messages being monitored by the specified `monitor`
/// from `NO_CHANGE` to `HIGH_WATERMARK` and then from `HIGH_WATERMARK` to
/// `FULL`. Behavior is undefined unless `monitor` is non-null and is in the
/// `NORMAL` state with respect to bytes and messages.
void testIncrementFromZeroToFullCapacity(mqbu::ResourceUsageMonitor* monitor)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(monitor);
    BSLS_ASSERT_SAFE(0 == monitor->bytes());
    BSLS_ASSERT_SAFE(0 == monitor->messages());
    BSLS_ASSERT_SAFE(monitor->byteState() ==
                     mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);
    BSLS_ASSERT_SAFE(monitor->messageState() ==
                     mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    const bsls::Types::Int64 byteCapacity = monitor->byteCapacity();
    const bsls::Types::Int64 byteHighWatermark =
        byteCapacity * monitor->byteHighWatermarkRatio();
    const bsls::Types::Int64 messageCapacity = monitor->messageCapacity();
    const bsls::Types::Int64 messageHighWatermark =
        messageCapacity * monitor->messageHighWatermarkRatio();

    // BYTES
    ASSERT_EQ(monitor->byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);
    // Not needed here but for logical consistency

    // Increment by 1 until hitting high watermark
    for (int i = monitor->bytes() + 1; i < byteHighWatermark; ++i) {
        ASSERT_EQ_D(i,
                    monitor->updateBytes(1),
                    mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
        ASSERT_EQ_D(i, i, monitor->bytes());
        ASSERT_EQ(monitor->byteState(),
                  mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);
    }

    // bytes = byteHighWatermark - 1
    ASSERT_EQ(monitor->updateBytes(1),
              mqbu::ResourceUsageMonitorStateTransition::e_HIGH_WATERMARK);
    ASSERT_EQ(byteHighWatermark, monitor->bytes());
    ASSERT_EQ(monitor->byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);

    // Increment by 1 until hitting full capacity
    for (int i = byteHighWatermark + 1; i < byteCapacity; ++i) {
        ASSERT_EQ_D(i,
                    monitor->updateBytes(1),
                    mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
        ASSERT_EQ_D(i, i, monitor->bytes());
        ASSERT_EQ(monitor->byteState(),
                  mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);
    }

    // bytes = byteCapacity - 1
    ASSERT_EQ(monitor->updateBytes(1),
              mqbu::ResourceUsageMonitorStateTransition::e_FULL);
    ASSERT_EQ(byteCapacity, monitor->bytes());
    ASSERT_EQ(monitor->byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    ASSERT_EQ(0, monitor->messages());  // no change in messages

    // MESSAGES
    ASSERT_EQ(monitor->messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // Increment by 1 until hitting high watermark
    for (int i = 1; i < messageHighWatermark; ++i) {
        ASSERT_EQ_D(i,
                    monitor->updateMessages(1),
                    mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
        ASSERT_EQ_D(i, i, monitor->messages());
        ASSERT_EQ(monitor->messageState(),
                  mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);
    }

    // messages = messageHighWatermark - 1
    ASSERT_EQ(monitor->updateMessages(1),
              mqbu::ResourceUsageMonitorStateTransition::e_HIGH_WATERMARK);
    ASSERT_EQ(messageHighWatermark, monitor->messages());
    ASSERT_EQ(monitor->messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);

    // Increment by 1 until hitting capacity
    for (int i = messageHighWatermark + 1; i < messageCapacity; ++i) {
        ASSERT_EQ_D(i,
                    monitor->updateMessages(1),
                    mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
        ASSERT_EQ_D(i, i, monitor->messages());
        ASSERT_EQ(monitor->messageState(),
                  mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);
    }

    // messages = messageCapacity - 1
    ASSERT_EQ(monitor->updateMessages(1),
              mqbu::ResourceUsageMonitorStateTransition::e_FULL);
    ASSERT_EQ(messageCapacity, monitor->messages());
    ASSERT_EQ(monitor->messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    ASSERT_EQ(byteCapacity, monitor->bytes());
    // no change in bytes

    // No change in the exposed parameters used to configure the monitor
    ASSERT_EQ(byteCapacity, monitor->byteCapacity());
    ASSERT_EQF(byteHighWatermark,
               byteCapacity * monitor->byteHighWatermarkRatio());
    ASSERT_EQ(messageCapacity, monitor->messageCapacity());
    ASSERT_EQF(messageHighWatermark,
               messageCapacity * monitor->messageHighWatermarkRatio());
}

/// Decrement bytes and messages being monitored by the specified `monitor`
/// from a state `FULL` to `LOW_WATERMARK`.  Behavior is undefined unless
/// the `monitor` is not null, is in the `FULL` state with respect to both
/// bytes and messages, and has bytes and messages exactly at their capacity
/// values.
void testDecrementFromFullCapacityToLowWatermark(
    mqbu::ResourceUsageMonitor* monitor)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(monitor);
    BSLS_ASSERT_SAFE(monitor->byteState() ==
                     mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    BSLS_ASSERT_SAFE(monitor->messageState() ==
                     mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    BSLS_ASSERT_SAFE(monitor->bytes() == monitor->byteCapacity());
    BSLS_ASSERT_SAFE(monitor->messages() == monitor->messageCapacity());

    const bsls::Types::Int64 byteLowWatermark =
        monitor->byteCapacity() * monitor->byteLowWatermarkRatio();
    const bsls::Types::Int64 byteHighWatermark =
        monitor->byteCapacity() * monitor->byteHighWatermarkRatio();
    const bsls::Types::Int64 messageLowWatermark =
        monitor->messageCapacity() * monitor->messageLowWatermarkRatio();
    const bsls::Types::Int64 messageHighWatermark =
        monitor->messageCapacity() * monitor->messageHighWatermarkRatio();

    // BYTES
    ASSERT_EQ(monitor->byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    // Not necessary here but for logical consistency

    // Decrement by 1 until hitting high watermark
    for (int i = monitor->bytes() - 1; i > byteHighWatermark; --i) {
        ASSERT_EQ_D(i,
                    monitor->updateBytes(-1),
                    mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
        ASSERT_EQ_D(i, i, monitor->bytes());
        ASSERT_EQ(monitor->byteState(),
                  mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    }

    // bytes = byteHighWatermark + 1
    ASSERT_EQ(monitor->updateBytes(-1),
              mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
    ASSERT_EQ(byteHighWatermark, monitor->bytes());
    ASSERT_EQ(monitor->byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);

    // Decrement by 1 until hitting low watermark
    for (int i = monitor->bytes() - 1; i > byteLowWatermark; --i) {
        ASSERT_EQ_D(i,
                    monitor->updateBytes(-1),
                    mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
        ASSERT_EQ_D(i, i, monitor->bytes());
        ASSERT_EQ(monitor->byteState(),
                  mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);
    }

    // bytes = byteLowWatermark + 1
    ASSERT_EQ(monitor->updateBytes(-1),
              mqbu::ResourceUsageMonitorStateTransition::e_LOW_WATERMARK);
    ASSERT_EQ(byteLowWatermark, monitor->bytes());
    ASSERT_EQ(monitor->byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // Decrement one more and observe no change
    ASSERT_EQ(monitor->updateBytes(-1),
              mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
    ASSERT_EQ(byteLowWatermark - 1, monitor->bytes());
    ASSERT_EQ(monitor->byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // Restore bytes value to low watermark point
    monitor->updateBytes(1);
    ASSERT_EQ(byteLowWatermark, monitor->bytes());

    // MESSAGES
    ASSERT_EQ(monitor->messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    // Not necessary here but for logical consistency

    // Decrement by 1 until hitting high watermark
    for (int i = monitor->messages() - 1; i > messageHighWatermark; --i) {
        ASSERT_EQ_D(i,
                    monitor->updateMessages(-1),
                    mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
        ASSERT_EQ_D(i, i, monitor->messages());
        ASSERT_EQ(monitor->messageState(),
                  mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    }

    // messages = messageHighWatermark + 1
    ASSERT_EQ(monitor->updateMessages(-1),
              mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
    ASSERT_EQ(messageHighWatermark, monitor->messages());
    ASSERT_EQ(monitor->messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);

    // Decrement by 1 until hitting low watermark
    for (int i = monitor->messages() - 1; i > messageLowWatermark; --i) {
        ASSERT_EQ_D(i,
                    monitor->updateMessages(-1),
                    mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
        ASSERT_EQ_D(i, i, monitor->messages());
        ASSERT_EQ(monitor->messageState(),
                  mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);
    }

    // messages = messageLowWatermark + 1
    ASSERT_EQ(monitor->updateMessages(-1),
              mqbu::ResourceUsageMonitorStateTransition::e_LOW_WATERMARK);
    ASSERT_EQ(messageLowWatermark, monitor->messages());
    ASSERT_EQ(monitor->messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // Decrement one more and observe no change
    ASSERT_EQ(monitor->updateMessages(-1),
              mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
    ASSERT_EQ(messageLowWatermark - 1, monitor->messages());
    ASSERT_EQ(monitor->messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // Restore messages value to low watermark point
    monitor->updateMessages(1);
    ASSERT_EQ(messageLowWatermark, monitor->messages());
}

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
// ------------------------------------------------------------------------
// BREATHING TEST
//
// Concerns:
//   Exercise the basic functionality of the component.
//
// Plan:
//   1 Check underlying assumption in the implementation of 'state()'
//     that the ResourceUsageMonitorState Enum is in increasing order of
//     limit reached.
//   2 Check underlying assumption in the implementation of 'update()'
//     that the ResourceUsageMonitorStateTransition Enum is in increasing
//     order of limit reached (after 'e_NO_CHANGE' as it comes first).
//   3 Create a mqbu::ResourceUsageMonitor object with default watermark
//     ratios
//   4 Increment bytes and messages from 'NO_CHANGE' to 'HIGH_WATERMARK'
//     and then from 'HIGH_WATERMARK' to 'FULL'
//   5 Reset object
//   6 Increment bytes and messages from 'NO_CHANGE' to 'HIGH_WATERMARK'
//     and then from 'HIGH_WATERMARK' to 'FULL'
//   7 Reset object with new limits and watermark ratios
//   8 Increment bytes and messages from 'NO_CHANGE' to 'HIGH_WATERMARK'
//     and then from 'HIGH_WATERMARK' to 'FULL'
//   9 Decrement bytes and messages from 'FULL' to 'LOW_WATERMARK'
//  10 Reconfigure object by ratio such that it is still in 'LOW_WATERMARK'
//     for bytes but in 'HIGH_WATERMARK' for messages
//  11 Reconfigure object by concrete value such that it is in
//     'HIGH_WATERMARK' for bytes but in 'LOW_WATERMARK' for messages
//
// Testing:
//   Basic functionality
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    // 1
    ASSERT_LT(mqbu::ResourceUsageMonitorState::e_STATE_NORMAL,
              mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);

    ASSERT_LT(mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK,
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    // 2
    ASSERT_LT(mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE,
              mqbu::ResourceUsageMonitorStateTransition::e_LOW_WATERMARK);

    ASSERT_LT(mqbu::ResourceUsageMonitorStateTransition::e_LOW_WATERMARK,
              mqbu::ResourceUsageMonitorStateTransition::e_HIGH_WATERMARK);

    ASSERT_LT(mqbu::ResourceUsageMonitorStateTransition::e_HIGH_WATERMARK,
              mqbu::ResourceUsageMonitorStateTransition::e_FULL);

    // 3 Create a mqbu::ResourceUsageMonitor object with default watermark
    // ratios
    bsls::Types::Int64 k_BYTE_CAPACITY                = 20;
    bsls::Types::Int64 k_MESSAGE_CAPACITY             = 40;
    double             k_BYTE_LOW_WATERMARK_RATIO     = 0.4;
    double             k_BYTE_HIGH_WATERMARK_RATIO    = 0.8;
    double             k_MESSAGE_LOW_WATERMARK_RATIO  = 0.4;
    double             k_MESSAGE_HIGH_WATERMARK_RATIO = 0.8;

    mqbu::ResourceUsageMonitor monitor(k_BYTE_CAPACITY, k_MESSAGE_CAPACITY);

    // Initial state: BYTES
    ASSERT_EQ(0, monitor.bytes());
    ASSERT_EQ(k_BYTE_CAPACITY, monitor.byteCapacity());
    ASSERT_EQF(k_BYTE_LOW_WATERMARK_RATIO, monitor.byteLowWatermarkRatio());
    ASSERT_EQF(k_BYTE_HIGH_WATERMARK_RATIO, monitor.byteHighWatermarkRatio());
    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // Initial state: MESSAGES
    ASSERT_EQ(0, monitor.messages());
    ASSERT_EQ(k_MESSAGE_CAPACITY, monitor.messageCapacity());
    ASSERT_EQF(k_MESSAGE_LOW_WATERMARK_RATIO,
               monitor.messageLowWatermarkRatio());
    ASSERT_EQF(k_MESSAGE_HIGH_WATERMARK_RATIO,
               monitor.messageHighWatermarkRatio());
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // 4 Increment bytes and messages from 'NO_CHANGE' to 'HIGH_WATERMARK' and
    // then from 'HIGH_WATERMARK' to 'FULL'
    testIncrementFromZeroToFullCapacity(&monitor);

    // 5 Reset object, values set to 0 but limits remain unchanged
    monitor.reset();

    // Reset state: BYTES
    ASSERT_EQ(0, monitor.bytes());
    ASSERT_EQ(k_BYTE_CAPACITY, monitor.byteCapacity());
    ASSERT_EQF(k_BYTE_LOW_WATERMARK_RATIO, monitor.byteLowWatermarkRatio());
    ASSERT_EQF(k_BYTE_HIGH_WATERMARK_RATIO, monitor.byteHighWatermarkRatio());
    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // Reset state: MESSAGES
    ASSERT_EQ(0, monitor.messages());
    ASSERT_EQ(k_MESSAGE_CAPACITY, monitor.messageCapacity());
    ASSERT_EQF(k_MESSAGE_LOW_WATERMARK_RATIO,
               monitor.messageLowWatermarkRatio());
    ASSERT_EQF(k_MESSAGE_HIGH_WATERMARK_RATIO,
               monitor.messageHighWatermarkRatio());
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // 6 Increment bytes and messages from 'NO_CHANGE' to 'HIGH_WATERMARK' and
    // then from 'HIGH_WATERMARK' to 'FULL'
    testIncrementFromZeroToFullCapacity(&monitor);

    // 7 Reset object with new limits and watermark ratios (values set to 0)
    k_BYTE_CAPACITY                = 30;
    k_MESSAGE_CAPACITY             = 50;
    k_BYTE_LOW_WATERMARK_RATIO     = 0.3;
    k_BYTE_HIGH_WATERMARK_RATIO    = 0.5;
    k_MESSAGE_LOW_WATERMARK_RATIO  = 0.8;
    k_MESSAGE_HIGH_WATERMARK_RATIO = 0.9;

    monitor.resetByRatio(k_BYTE_CAPACITY,
                         k_MESSAGE_CAPACITY,
                         k_BYTE_LOW_WATERMARK_RATIO,
                         k_BYTE_HIGH_WATERMARK_RATIO,
                         k_MESSAGE_LOW_WATERMARK_RATIO,
                         k_MESSAGE_HIGH_WATERMARK_RATIO);

    // Reset state: BYTES
    ASSERT_EQ(0, monitor.bytes());
    ASSERT_EQ(k_BYTE_CAPACITY, monitor.byteCapacity());
    ASSERT_EQF(k_BYTE_LOW_WATERMARK_RATIO, monitor.byteLowWatermarkRatio());
    ASSERT_EQF(k_BYTE_HIGH_WATERMARK_RATIO, monitor.byteHighWatermarkRatio());
    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // Reset state: MESSAGES
    ASSERT_EQ(0, monitor.messages());
    ASSERT_EQ(k_MESSAGE_CAPACITY, monitor.messageCapacity());
    ASSERT_EQF(k_MESSAGE_LOW_WATERMARK_RATIO,
               monitor.messageLowWatermarkRatio());
    ASSERT_EQF(k_MESSAGE_HIGH_WATERMARK_RATIO,
               monitor.messageHighWatermarkRatio());
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // 8 Increment bytes and messages from 'NO_CHANGE' to 'HIGH_WATERMARK' and
    // then from 'HIGH_WATERMARK' to 'FULL'
    testIncrementFromZeroToFullCapacity(&monitor);

    // 9 Decrement bytes and messages from 'FULL' to 'LOW_WATERMARK'
    testDecrementFromFullCapacityToLowWatermark(&monitor);

    // 10 Reconfigure object by ratio such that it becomes in 'HIGH_WATERMARK'
    //    state for bytes but remains in 'LOW_WATERMARK' state for messages,
    //    and confirm that the values of bytes and messages have not changed.
    const bsls::Types::Int64 bytesBefore    = monitor.bytes();
    const bsls::Types::Int64 messagesBefore = monitor.messages();

    monitor.reconfigureByRatio(k_BYTE_CAPACITY,
                               k_MESSAGE_CAPACITY,
                               k_BYTE_LOW_WATERMARK_RATIO,
                               k_BYTE_LOW_WATERMARK_RATIO,
                               //    'HIGH_WATERMARK_RATIO'
                               // == 'LOW_WATERMARK_RATIO'
                               k_MESSAGE_LOW_WATERMARK_RATIO,
                               k_MESSAGE_HIGH_WATERMARK_RATIO);

    ASSERT_EQ(monitor.byteCapacity(), k_BYTE_CAPACITY);
    ASSERT_EQF(k_BYTE_LOW_WATERMARK_RATIO, monitor.byteLowWatermarkRatio());
    ASSERT_EQF(k_BYTE_LOW_WATERMARK_RATIO, monitor.byteHighWatermarkRatio());
    ASSERT_EQ(monitor.messageCapacity(), k_MESSAGE_CAPACITY);
    ASSERT_EQF(k_MESSAGE_LOW_WATERMARK_RATIO,
               monitor.messageLowWatermarkRatio());
    ASSERT_EQF(k_MESSAGE_HIGH_WATERMARK_RATIO,
               monitor.messageHighWatermarkRatio());

    ASSERT_EQ(monitor.bytes(), bytesBefore);
    ASSERT_EQ(monitor.messages(), messagesBefore);

    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // 11 Reconfigure object by concrete value instead of by ratio such that it
    //    becomes in 'HIGH_WATERMARK' state for bytes but remains in
    //    'LOW_WATERMARK' state for messages, and confirm that the values of
    //    bytes and messages have not changed.
    bsls::Types::Int64 k_BYTE_LOW_WATERMARK = k_BYTE_CAPACITY *
                                              k_BYTE_LOW_WATERMARK_RATIO;
    bsls::Types::Int64 k_BYTE_HIGH_WATERMARK = k_BYTE_CAPACITY *
                                               k_BYTE_HIGH_WATERMARK_RATIO;
    bsls::Types::Int64 k_MESSAGE_LOW_WATERMARK = k_MESSAGE_CAPACITY *
                                                 k_MESSAGE_LOW_WATERMARK_RATIO;

    monitor.reconfigure(k_BYTE_CAPACITY,
                        k_MESSAGE_CAPACITY,
                        k_BYTE_LOW_WATERMARK,
                        k_BYTE_HIGH_WATERMARK,
                        k_MESSAGE_LOW_WATERMARK,
                        k_MESSAGE_LOW_WATERMARK);
    // 'HIGH_WATERMARK' == 'LOW_WATERMARK'`

    ASSERT_EQ(monitor.byteCapacity(), k_BYTE_CAPACITY);
    ASSERT_EQF(k_BYTE_LOW_WATERMARK_RATIO, monitor.byteLowWatermarkRatio());
    ASSERT_EQF(k_BYTE_HIGH_WATERMARK_RATIO, monitor.byteHighWatermarkRatio());
    ASSERT_EQ(monitor.messageCapacity(), k_MESSAGE_CAPACITY);
    ASSERT_EQF(k_MESSAGE_LOW_WATERMARK_RATIO,
               monitor.messageLowWatermarkRatio());
    ASSERT_EQF(k_MESSAGE_LOW_WATERMARK_RATIO,
               monitor.messageHighWatermarkRatio());

    ASSERT_EQ(monitor.bytes(), bytesBefore);
    ASSERT_EQ(monitor.messages(), messagesBefore);

    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);
}

static void test2_zeroCapacity()
// ------------------------------------------------------------------------
// ZERO CAPACITY
//
// Concerns:
//   If the ResourceUsageMonitor is constructed or reconfigured with zero
//   capacity for bytes and/or messages, then the corresponding state(s)
//   are full.
//
// Plan:
//   1 Create an instance with zero messageCapacity and zero byteCapacity
//   2 Reconfigure instance to have zero messageCapacity and non-zero
//     byteCapacity
//   3 Reconfigure instance to have non-zero messageCapacity and zero
//     byteCapacity
//
// Testing:
//   Proper behavior in the special case where one or more of the
//   messageCapacity or byteCapacity is zero.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("UPDATE ONE RESOURCE");

    // 1 Create an instance with zero messageCapacity and zero byteCapacity
    mqbu::ResourceUsageMonitor obj(0, 0, 0, 0, 0, 0);
    ASSERT_EQ(obj.messageCapacity(), 0);
    ASSERT_EQF(obj.messageLowWatermarkRatio(), 0.0);
    ASSERT_EQF(obj.messageHighWatermarkRatio(), 0.0);
    ASSERT_EQ(obj.byteCapacity(), 0);
    ASSERT_EQF(obj.byteLowWatermarkRatio(), 0.0);
    ASSERT_EQF(obj.byteHighWatermarkRatio(), 0.0);

    ASSERT_EQ(obj.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    ASSERT_EQ(obj.byteState(), mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    ASSERT_EQ(obj.state(), mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    //   2 Reconfigure instance to have zero messageCapacity and non-zero
    //     byteCapacity
    obj.reconfigure(5, 0, 0, 0, 0, 0);
    ASSERT_EQ(obj.messageCapacity(), 0);
    ASSERT_EQF(obj.messageLowWatermarkRatio(), 0.0);
    ASSERT_EQF(obj.messageHighWatermarkRatio(), 0.0);
    ASSERT_EQ(obj.byteCapacity(), 5);
    ASSERT_EQF(obj.byteLowWatermarkRatio(), 0.0);
    ASSERT_EQF(obj.byteHighWatermarkRatio(), 0.0);

    ASSERT_EQ(obj.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    ASSERT_EQ(obj.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);
    ASSERT_EQ(obj.state(), mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    //   3 Reconfigure instance to have non-zero messageCapacity and zero
    //     byteCapacity
    obj.reconfigure(0, 5, 0, 0, 0, 0);
    ASSERT_EQ(obj.messageCapacity(), 5);
    ASSERT_EQF(obj.messageLowWatermarkRatio(), 0.0);
    ASSERT_EQF(obj.messageHighWatermarkRatio(), 0.0);
    ASSERT_EQ(obj.byteCapacity(), 0);
    ASSERT_EQF(obj.byteLowWatermarkRatio(), 0.0);
    ASSERT_EQF(obj.byteHighWatermarkRatio(), 0.0);

    ASSERT_EQ(obj.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);
    ASSERT_EQ(obj.byteState(), mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    ASSERT_EQ(obj.state(), mqbu::ResourceUsageMonitorState::e_STATE_FULL);
}

static void test3_updateOneResource()
// ------------------------------------------------------------------------
// UPDATE ONE RESOURCE
//
// Concerns:
//   Ensure proper behavior of the 'updateBytes()' and 'updateMessages()'
//   methods.
//
// Plan:
//   1 Test various combinations of monitor configuration and deltas for
//     bytes and messages (with a focus on limit boundaries).
//
// Testing:
//   Proper behavior of the following methods:
//     - updateBytes(bsls::Types::Int64 delta)
//     - updateMessages(bsls::Types::Int64 delta)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("UPDATE ONE RESOURCE");

    // TYPES
    typedef mqbu::ResourceUsageMonitorStateTransition RUMStateTransition;

    struct Test {
        int                      d_line;
        bsls::Types::Int64       d_byteLowWatermark;
        bsls::Types::Int64       d_byteHighWatermark;
        bsls::Types::Int64       d_byteCapacity;
        bsls::Types::Int64       d_messageLowWatermark;
        bsls::Types::Int64       d_messageHighWatermark;
        bsls::Types::Int64       d_messageCapacity;
        bsls::Types::Int64       d_bytes;
        bsls::Types::Int64       d_messages;
        RUMStateTransition::Enum d_expectedByteStateTransition;
        RUMStateTransition::Enum d_expectedMessageStateTransition;
    } k_DATA[] = {
        // Format:
        //..
        // { <lineNumber>
        // <byteLowWatermark>, <byteHighWatermark>, <byteCapacity>
        // <messageLowWatermark>, <messageHighWatermark>, <messageCapacity>
        // <bytes>, <messages>
        // <expectedByteStateTransition>
        // <expectedMessageStateTransition> }
        //..
        //
        // LOW_WATERMARK < HIGH_WATERMARK and HIGH_WATERMARK < CAPACITY
        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         0,
         0,
         RUMStateTransition::e_NO_CHANGE,   // Bytes    : 0 to 0
         RUMStateTransition::e_NO_CHANGE},  // Messages : 0 to 0

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         5,
         20,
         RUMStateTransition::e_NO_CHANGE,   // Bytes    : 0 to LOW_WATERMARK
         RUMStateTransition::e_NO_CHANGE},  // Messages : 0 to LOW_WATERMARK

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         12,
         36,
         RUMStateTransition::e_NO_CHANGE,   // Bytes    : 0 to HIGH_WATERMARK -
                                            // 1
         RUMStateTransition::e_NO_CHANGE},  // Messages : 0 to HIGH_WATERMARK -
                                            // 1

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         13,
         37,
         RUMStateTransition::e_HIGH_WATERMARK,   // Bytes    : 0 to
                                                 //            HIGH_WATERMARK
         RUMStateTransition::e_HIGH_WATERMARK},  // Messages : 0 to
                                                 //            HIGH_WATERMARK

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         14,
         39,
         RUMStateTransition::e_HIGH_WATERMARK,  // Bytes    : 0 to CAPACITY - 1
         RUMStateTransition::e_HIGH_WATERMARK},  // Messages : 0 to CAPACITY -
                                                 // 1

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         15,
         40,
         RUMStateTransition::e_FULL,   // Bytes    : 0 to CAPACITY
         RUMStateTransition::e_FULL},  // Messages : 0 to CAPACITY

        // LOW_WATERMARK < HIGH_WATERMARK and HIGH_WATERMARK == CAPACITY
        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         0,
         0,
         RUMStateTransition::e_NO_CHANGE,   // Bytes    : 0 to 0
         RUMStateTransition::e_NO_CHANGE},  // Messages : 0 to 0

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         5,
         20,
         RUMStateTransition::e_NO_CHANGE,   // Bytes    : 0 to LOW_WATERMARK
         RUMStateTransition::e_NO_CHANGE},  // Messages : 0 to LOW_WATERMARK

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         12,
         36,
         RUMStateTransition::e_NO_CHANGE,   // Bytes    : 0 to HIGH_WATERMARK -
                                            // 1
         RUMStateTransition::e_NO_CHANGE},  // Messages : 0 to HIGH_WATERMARK -
                                            // 1

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         13,
         37,
         RUMStateTransition::e_FULL,   // Bytes    : 0 to
                                       //            HIGH_WATERMARK == CAPACITY
         RUMStateTransition::e_FULL},  // Messages : 0 to
                                       //            HIGH_WATERMARK == CAPACITY

        // LOW_WATERMARK == HIGH_WATERMARK and HIGH_WATERMARK < CAPACITY
        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         0,
         0,
         RUMStateTransition::e_NO_CHANGE,   // Bytes    : 0 to 0
         RUMStateTransition::e_NO_CHANGE},  // Messages : 0 to 0

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         4,
         19,
         RUMStateTransition::e_NO_CHANGE,  // Bytes    : 0 to LOW_WATERMARK - 1
         RUMStateTransition::e_NO_CHANGE},  // Messages : 0 to LOW_WATERMARK -
                                            // 1

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         5,
         20,
         RUMStateTransition::e_HIGH_WATERMARK,  // Bytes    : 0 to
                                                //                LOW_WATERMARK
                                                //             ==
                                                //             HIGH_WATERMARK
         RUMStateTransition::
             e_HIGH_WATERMARK},  // Messages : 0 to
                                 //                LOW_WATERMARK
                                 //             == HIGH_WATERMARK

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         14,
         39,
         RUMStateTransition::e_HIGH_WATERMARK,  // Bytes    : 0 to CAPACITY - 1
         RUMStateTransition::e_HIGH_WATERMARK},  // Messages : 0 to CAPACITY -
                                                 // 1

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         15,
         40,
         RUMStateTransition::e_FULL,   // Bytes    : 0 to CAPACITY
         RUMStateTransition::e_FULL},  // Messages : 0 to CAPACITY

    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": checking 'monitor(" << test.d_byteLowWatermark
                        << ", " << test.d_byteHighWatermark << ", "
                        << test.d_byteCapacity << ", "
                        << test.d_messageLowWatermark << ", "
                        << test.d_messageHighWatermark << ", "
                        << test.d_messageCapacity << ")'\n"
                        << "updateBytes(" << test.d_bytes << ")\n"
                        << "updateMessages(" << test.d_messages << ")\n");

        mqbu::ResourceUsageMonitor monitor(
            test.d_byteCapacity,
            test.d_messageCapacity,
            test.d_byteLowWatermark * 1.0 / test.d_byteCapacity,
            test.d_byteHighWatermark * 1.0 / test.d_byteCapacity,
            test.d_messageLowWatermark * 1.0 / test.d_messageCapacity,
            test.d_messageHighWatermark * 1.0 / test.d_messageCapacity);

        // Update bytes
        ASSERT_EQ_D("line " << test.d_line,
                    monitor.updateBytes(test.d_bytes),
                    test.d_expectedByteStateTransition);

        ASSERT_EQ_D("line " << test.d_line, monitor.bytes(), test.d_bytes);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.updateBytes(0),
                    RUMStateTransition::e_NO_CHANGE);

        ASSERT_EQ_D("line " << test.d_line, monitor.bytes(), test.d_bytes);

        monitor.updateBytes(-test.d_bytes);
        ASSERT_EQ_D("line " << test.d_line, monitor.bytes(), 0);

        // Update messages
        ASSERT_EQ_D("line " << test.d_line,
                    monitor.updateMessages(test.d_messages),
                    test.d_expectedMessageStateTransition);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.messages(),
                    test.d_messages);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.updateMessages(0),
                    RUMStateTransition::e_NO_CHANGE);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.messages(),
                    test.d_messages);

        monitor.updateMessages(-test.d_messages);
        ASSERT_EQ_D("line " << test.d_line, monitor.messages(), 0);
    }
}

static void test4_updateBothResources()
// ------------------------------------------------------------------------
// UPDATE BOTH RESOURCES
//
// Concerns:
//   Ensure proper behavior of the 'update()' method. Specifically, we need
//   to ensure that calling 'update(bytes, messages)' updates the monitor's
//   bytes and messages by 'bytes' and 'messages', respectively, and emits
//   the appropriate state transition when a limit is reached.
//   Additionally, we need to ensure that the appropriate state transition
//   is emitted only once (for instance, if an update puts bytes but not
//   messages in 'HIGH_WATERMARK' and then another update puts messages in
//   'HIGH_WATERMARK', then a 'HIGH_WATERMARK' state transition is emitted
//   only for the first update.
//
// Plan:
//   1 Exercise 'update(bytesDelta, messagesDelta)' for various
//     combinations of monitor configuration and deltas for bytes and
//     messages (with a focus on limit boundaries) and verify that the
//     bytes, messages, and emitted state transition are as expected.
//   2 Verify that an update that takes the monitor past a limit emits the
//     appropriate state transition only once.
//
// Testing:
//   Proper behavior of the following method:
//     -  update(bsls::Types::Int64 deltaBytes,
//               bsls::Types::Int64 deltaMessages)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("UPDATE BOTH RESOURCES");

    // TYPES
    typedef mqbu::ResourceUsageMonitorStateTransition RUMStateTransition;

    // 2
    struct Test {
        int                      d_line;
        bsls::Types::Int64       d_byteLowWatermark;
        bsls::Types::Int64       d_byteHighWatermark;
        bsls::Types::Int64       d_byteCapacity;
        bsls::Types::Int64       d_messageLowWatermark;
        bsls::Types::Int64       d_messageHighWatermark;
        bsls::Types::Int64       d_messageCapacity;
        bsls::Types::Int64       d_bytes;
        bsls::Types::Int64       d_messages;
        RUMStateTransition::Enum d_expectedStateTransition;
    } k_DATA[] = {
        // Format:
        //..
        // { <lineNumber>
        // <byteLowWatermark>, <byteHighWatermark>, <byteCapacity>
        // <messageLowWatermark>, <messageHighWatermark>, <messageCapacity>
        // <bytes>, <messages>
        // <expectedStateTransition> }
        //..
        //
        // LOW_WATERMARK < HIGH_WATERMARK and HIGH_WATERMARK < CAPACITY
        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         0,
         0,
         RUMStateTransition::e_NO_CHANGE},  // 0 to 0 (both)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         5,
         20,
         RUMStateTransition::e_NO_CHANGE},  // 0 to LOW_WATERMARK (both)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         12,
         36,
         RUMStateTransition::e_NO_CHANGE},  // 0 to HIGH_WATERMARK - 1 (both)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         13,
         36,
         RUMStateTransition::e_HIGH_WATERMARK},  // 0 to HIGH_WATERMARK (bytes)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         12,
         37,
         RUMStateTransition::e_HIGH_WATERMARK},  // 0 to HIGH_WATERMARK (msgs)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         13,
         37,
         RUMStateTransition::e_HIGH_WATERMARK},  // 0 to HIGH_WATERMARK (both)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         14,
         39,
         RUMStateTransition::e_HIGH_WATERMARK},  // 0 to CAPACITY - 1 (both)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         15,
         39,
         RUMStateTransition::e_FULL},  // 0 to CAPACITY (bytes)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         14,
         40,
         RUMStateTransition::e_FULL},  // 0 to CAPACITY (msgs)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         15,
         40,
         RUMStateTransition::e_FULL},  // 0 to CAPACITY (both)

        // LOW_WATERMARK < HIGH_WATERMARK and HIGH_WATERMARK == CAPACITY
        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         0,
         0,
         RUMStateTransition::e_NO_CHANGE},  // 0 to 0 (both)

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         5,
         20,
         RUMStateTransition::e_NO_CHANGE},  // 0 to LOW_WATERMARK (both)

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         12,
         36,
         RUMStateTransition::e_NO_CHANGE},  // 0 to HIGH_WATERMARK - 1 (both)

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         13,
         36,
         RUMStateTransition::e_FULL},  // 0 to HIGH_WATERMARK == CAPACITY
                                       // (bytes)

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         12,
         37,
         RUMStateTransition::e_FULL},  // 0 to HIGH_WATERMARK == CAPACITY
                                       // (msgs)
        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         13,
         37,
         RUMStateTransition::e_FULL},  // 0 to HIGH_WATERMARK == CAPACITY
                                       // (both)

        // LOW_WATERMARK == HIGH_WATERMARK and HIGH_WATERMARK < CAPACITY
        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         0,
         0,
         RUMStateTransition::e_NO_CHANGE},  // 0 to 0 (both)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         4,
         19,
         RUMStateTransition::e_NO_CHANGE},  // 0 to LOW_WATERMARK - 1 (both)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         5,
         19,
         RUMStateTransition::e_HIGH_WATERMARK},  // 0 to LOW_WATERMARK
                                                 //   == HIGH_WATERMARK (bytes)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         4,
         20,
         RUMStateTransition::e_HIGH_WATERMARK},  // 0 to LOW_WATERMARK
                                                 //   == HIGH_WATERMARK (msgs)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         5,
         20,
         RUMStateTransition::e_HIGH_WATERMARK},  // 0 to LOW_WATERMARK
                                                 //   == HIGH_WATERMARK (both)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         14,
         39,
         RUMStateTransition::e_HIGH_WATERMARK},  // 0 to CAPACITY - 1 (both)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         15,
         39,
         RUMStateTransition::e_FULL},  // 0 to CAPACITY (bytes)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         14,
         40,
         RUMStateTransition::e_FULL},  // 0 to CAPACITY (msgs)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         15,
         40,
         RUMStateTransition::e_FULL},  // 0 to CAPACITY (both)
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": checking 'monitor(" << test.d_byteLowWatermark
                        << ", " << test.d_byteHighWatermark << ", "
                        << test.d_byteCapacity << ", "
                        << test.d_messageLowWatermark << ", "
                        << test.d_messageHighWatermark << ", "
                        << test.d_messageCapacity << ")'\n"
                        << "- update(" << test.d_bytes << ", "
                        << test.d_messages << ")"
                        << "\n");

        // Create a mqbu::ResourceUsageMonitor with test parameters
        mqbu::ResourceUsageMonitor monitor(
            test.d_byteCapacity,
            test.d_messageCapacity,
            test.d_byteLowWatermark * 1.0 / test.d_byteCapacity,
            test.d_byteHighWatermark * 1.0 / test.d_byteCapacity,
            test.d_messageLowWatermark * 1.0 / test.d_messageCapacity,
            test.d_messageHighWatermark * 1.0 / test.d_messageCapacity);

        // update(bytes, msgs)
        ASSERT_EQ_D("line " << test.d_line,
                    monitor.update(test.d_bytes, test.d_messages),
                    test.d_expectedStateTransition);

        ASSERT_EQ_D("line " << test.d_line, monitor.bytes(), test.d_bytes);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.messages(),
                    test.d_messages);

        // Test edge case where update value is 0 (This fails in the original
        // 'mqbu::ResourceUsageMonitor' if 'highWatermark == capacity' and the
        // value is exactly at 'highWatermark' when 'update(0)' is called; see
        // the 'else' block in that component's 'update()' method for more
        // info) but shouldn't fail here.
        // This can be verified by replacing in the .cpp file methods
        // 'update<Bytes/Messages>' the 'else if (delta<Bytes/Messages> < 0)'
        // with just 'else'.
        ASSERT_EQ_D("line " << test.d_line,
                    monitor.update(0, 0),
                    RUMStateTransition::e_NO_CHANGE);

        ASSERT_EQ_D("line " << test.d_line, monitor.bytes(), test.d_bytes);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.messages(),
                    test.d_messages);

        // update(-bytes, -msgs)
        monitor.update(-test.d_bytes, -test.d_messages);

        ASSERT_EQ_D("line " << test.d_line, monitor.bytes(), 0);

        ASSERT_EQ_D("line " << test.d_line, monitor.messages(), 0);
    }

    // 3 Verify that an update that takes the monitor past a limit emits the
    //   appropriate state transition only once.

    // Configuration parameters
    bsls::Types::Int64 k_BYTE_LOW_WATERMARK     = 80;
    bsls::Types::Int64 k_BYTE_HIGH_WATERMARK    = 100;
    bsls::Types::Int64 k_BYTE_CAPACITY          = 110;
    bsls::Types::Int64 k_MESSAGE_LOW_WATERMARK  = 4;
    bsls::Types::Int64 k_MESSAGE_HIGH_WATERMARK = 10;
    bsls::Types::Int64 k_MESSAGE_CAPACITY       = 11;

    // Create a mqbu::ResourceUsageMonitor object
    mqbu::ResourceUsageMonitor monitor(
        k_BYTE_CAPACITY,
        k_MESSAGE_CAPACITY,
        k_BYTE_LOW_WATERMARK * 1.0 / k_BYTE_CAPACITY,
        k_BYTE_HIGH_WATERMARK * 1.0 / k_BYTE_CAPACITY,
        k_MESSAGE_LOW_WATERMARK * 1.0 / k_MESSAGE_CAPACITY,
        k_MESSAGE_HIGH_WATERMARK * 1.0 / k_MESSAGE_CAPACITY);

    ASSERT_EQ(monitor.update(k_BYTE_HIGH_WATERMARK - 1,
                             k_MESSAGE_HIGH_WATERMARK - 1),
              RUMStateTransition::e_NO_CHANGE);

    // Reach STATE_HIGH_WATERMARK (bytes state becomes STATE_HIGH_WATERMARK)
    ASSERT_EQ(monitor.update(1, 0), RUMStateTransition::e_HIGH_WATERMARK);

    // Still in STATE_HIGH_WATERMARK, emit NO_CHANGE (messages state becomes
    // STATE_HIGH_WATERMARK, bytes state stays in STATE_HIGH_WATERMARK)
    ASSERT_EQ(monitor.update(-1, 1), RUMStateTransition::e_NO_CHANGE);

    // Still in STATE_HIGH_WATERMARK, emit NO_CHANGE
    ASSERT_EQ(monitor.update(1, 0), RUMStateTransition::e_NO_CHANGE);

    // Reset bytes and messages values
    monitor.resetByRatio(k_BYTE_CAPACITY,
                         k_MESSAGE_CAPACITY,
                         k_BYTE_LOW_WATERMARK * 1.0 / k_BYTE_CAPACITY,
                         k_BYTE_HIGH_WATERMARK * 1.0 / k_BYTE_CAPACITY,
                         k_MESSAGE_LOW_WATERMARK * 1.0 / k_MESSAGE_CAPACITY,
                         k_MESSAGE_HIGH_WATERMARK * 1.0 / k_MESSAGE_CAPACITY);

    ASSERT_EQ(monitor.update(k_BYTE_CAPACITY - 1, k_MESSAGE_CAPACITY - 1),
              RUMStateTransition::e_HIGH_WATERMARK);

    // Reach STATE_FULL, emit FULL (bytes state becomes STATE_FULL)
    ASSERT_EQ(monitor.update(1, 0), RUMStateTransition::e_FULL);

    // Still in STATE_FULL, emit NO_CHANGE (messages state becomes STATE_FULL,
    // bytes state stays STATE_FULL)
    ASSERT_EQ(monitor.update(-1, 1), RUMStateTransition::e_NO_CHANGE);

    // Still in STATE_FULL, emit NO_CHANGE
    ASSERT_EQ(monitor.update(1, 0), RUMStateTransition::e_NO_CHANGE);

    bsls::Types::Int64 bytesDiffFullToLow = k_BYTE_CAPACITY -
                                            k_BYTE_LOW_WATERMARK;
    bsls::Types::Int64 messagesDiffFullToLow = k_MESSAGE_CAPACITY -
                                               k_MESSAGE_LOW_WATERMARK;

    ASSERT_EQ(monitor.update(-bytesDiffFullToLow + 1,
                             -messagesDiffFullToLow + 1),
              RUMStateTransition::e_NO_CHANGE);

    // Still in STATE_FULL, emit NO_CHANGE (bytes state becomes STATE_NORMAL)
    ASSERT_EQ(monitor.update(-1, 0), RUMStateTransition::e_NO_CHANGE);

    // Reach LOW_WATERMARK (messages state becomes LOW_WATERMARK, bytes state
    // stays at LOW_WATERMARK)
    ASSERT_EQ(monitor.update(1, -1), RUMStateTransition::e_LOW_WATERMARK);

    // Still in LOW_WATERMARK
    ASSERT_EQ(monitor.update(-1, 0), RUMStateTransition::e_NO_CHANGE);

    // Decrementing further does not emit
    ASSERT_EQ(monitor.update(-k_BYTE_LOW_WATERMARK, -k_MESSAGE_LOW_WATERMARK),
              RUMStateTransition::e_NO_CHANGE);
}

static void test5_stateOneResource()
// ------------------------------------------------------------------------
// STATE ONE RESOURCE
//
// Concerns:
//   Ensure that the 'byteState()' and 'messageState()' accessors return
//   the proper state of the monitor with respect to bytes and messages,
//   respectively.
//
// Plan:
//   1 Use the input data from 'test4' (where we asserted that an expected
//     transition in the state of the monitor with respect to bytes and
//     messages took place after 'updateBytes()' and 'updateMessages()',
//     respectively) to ensure that the resulting state as reported by
//     'byteState()' and 'messageState()' is as expected per the
//     respective transitions observed.
//
// Testing:
//   Proper state as reported by the following methods:
//     - byteState()
//     - messageState()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("STATE ONE RESOURCE");

    // TYPES
    typedef mqbu::ResourceUsageMonitorState RUMState;

    struct Test {
        int                d_line;
        bsls::Types::Int64 d_byteLowWatermark;
        bsls::Types::Int64 d_byteHighWatermark;
        bsls::Types::Int64 d_byteCapacity;
        bsls::Types::Int64 d_messageLowWatermark;
        bsls::Types::Int64 d_messageHighWatermark;
        bsls::Types::Int64 d_messageCapacity;
        bsls::Types::Int64 d_bytes;
        bsls::Types::Int64 d_messages;
        RUMState::Enum     d_expectedByteState;
        RUMState::Enum     d_expectedMessageState;
    } k_DATA[] = {
        // Format:
        //..
        // { <lineNumber>
        // <byteLowWatermark>, <byteHighWatermark>, <byteCapacity>
        // <messageLowWatermark>, <messageHighWatermark>, <messageCapacity>
        // <bytes>, <messages>
        // <expectedByteState>
        // <expectedMessageState> }
        //..
        //
        // LOW_WATERMARK < HIGH_WATERMARK and HIGH_WATERMARK < CAPACITY
        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         0,
         0,
         RUMState::e_STATE_NORMAL,   // Bytes    : 0 to 0
         RUMState::e_STATE_NORMAL},  // Messages : 0 to 0

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         5,
         20,
         RUMState::e_STATE_NORMAL,   // Bytes    : 0 to LOW_WATERMARK
         RUMState::e_STATE_NORMAL},  // Messages : 0 to LOW_WATERMARK

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         12,
         36,
         RUMState::e_STATE_NORMAL,   // Bytes    : 0 to HIGH_WATERMARK - 1
         RUMState::e_STATE_NORMAL},  // Messages : 0 to HIGH_WATERMARK - 1

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         13,
         36,
         RUMState::e_STATE_HIGH_WATERMARK,  // Bytes    : 0 to HIGH_WATERMARK
         RUMState::e_STATE_NORMAL},  // Messages : 0 to HIGH_WATERMARK - 1

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         12,
         37,
         RUMState::e_STATE_NORMAL,           // Bytes    : 0 to
                                             //            HIGH_WATERMARK - 1
         RUMState::e_STATE_HIGH_WATERMARK},  // Messages : 0 to HIGH_WATERMARK

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         13,
         37,
         RUMState::e_STATE_HIGH_WATERMARK,   // Bytes    : 0 to HIGH_WATERMARK
         RUMState::e_STATE_HIGH_WATERMARK},  // Messages : 0 to HIGH_WATERMARK

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         14,
         39,
         RUMState::e_STATE_HIGH_WATERMARK,   // Bytes    : 0 to CAPACITY - 1
         RUMState::e_STATE_HIGH_WATERMARK},  // Messages : 0 to CAPACITY - 1

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         15,
         39,
         RUMState::e_STATE_FULL,             // Bytes    : 0 to CAPACITY
         RUMState::e_STATE_HIGH_WATERMARK},  // Messages : 0 to CAPACITY - 1

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         14,
         40,
         RUMState::e_STATE_HIGH_WATERMARK,  // Bytes    : 0 to CAPACITY - 1
         RUMState::e_STATE_FULL},           // Messages : 0 to CAPACITY

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         15,
         40,
         RUMState::e_STATE_FULL,   // Bytes    : 0 to CAPACITY
         RUMState::e_STATE_FULL},  // Messages : 0 to CAPACITY

        // LOW_WATERMARK < HIGH_WATERMARK and HIGH_WATERMARK == CAPACITY
        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         0,
         0,
         RUMState::e_STATE_NORMAL,   // Bytes    : 0 to 0
         RUMState::e_STATE_NORMAL},  // Messages : 0 to 0

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         5,
         20,
         RUMState::e_STATE_NORMAL,   // Bytes    : 0 to LOW_WATERMARK
         RUMState::e_STATE_NORMAL},  // Messages : 0 to LOW_WATERMARK

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         12,
         36,
         RUMState::e_STATE_NORMAL,   // Bytes    : 0 to HIGH_WATERMARK - 1
         RUMState::e_STATE_NORMAL},  // Messages : 0 to HIGH_WATERMARK - 1

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         13,
         36,
         RUMState::e_STATE_FULL,     // Bytes    : 0 to
                                     //            HIGH_WATERMARK == CAPACITY
         RUMState::e_STATE_NORMAL},  // Messages : 0 to HIGH_WATERMARK - 1

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         12,
         37,
         RUMState::e_STATE_NORMAL,  // Bytes    : 0 to HIGH_WATERMARK - 1
         RUMState::e_STATE_FULL},   // Messages : 0 to HIGH_WATERMARK ==
                                    // CAPACITY

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         13,
         37,
         RUMState::e_STATE_FULL,  // Bytes    : 0 to HIGH_WATERMARK == CAPACITY
         RUMState::e_STATE_FULL},  // Messages : 0 to HIGH_WATERMARK ==
                                   // CAPACITY

        // LOW_WATERMARK == HIGH_WATERMARK and HIGH_WATERMARK < CAPACITY
        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         0,
         0,
         RUMState::e_STATE_NORMAL,   // Bytes    : 0 to 0
         RUMState::e_STATE_NORMAL},  // Messages : 0 to 0

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         4,
         19,
         RUMState::e_STATE_NORMAL,   // Bytes    : 0 to LOW_WATERMARK - 1
         RUMState::e_STATE_NORMAL},  // Messages : 0 to LOW_WATERMARK - 1

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         5,
         19,
         RUMState::e_STATE_HIGH_WATERMARK,  // Bytes    : 0 to LOW_WATERMARK
                                            //              == HIGH_WATERMARK
         RUMState::e_STATE_NORMAL},  // Messages : 0 to LOW_WATERMARK - 1

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         4,
         20,
         RUMState::e_STATE_NORMAL,  // Bytes    : 0 to LOW_WATERMARK - 1
         RUMState::e_STATE_HIGH_WATERMARK},  // Messages : 0 to LOW_WATERMARK
                                             //              == HIGH_WATERMARK

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         5,
         20,
         RUMState::e_STATE_HIGH_WATERMARK,   // Bytes    : 0 to LOW_WATERMARK
                                             //              == HIGH_WATERMARK
         RUMState::e_STATE_HIGH_WATERMARK},  // Messages : 0 to LOW_WATERMARK
                                             //              == HIGH_WATERMARK

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         14,
         39,
         RUMState::e_STATE_HIGH_WATERMARK,   // Bytes    : 0 to CAPACITY - 1
         RUMState::e_STATE_HIGH_WATERMARK},  // Messages : 0 to CAPACITY - 1

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         15,
         39,
         RUMState::e_STATE_FULL,             // Bytes    : 0 to CAPACITY
         RUMState::e_STATE_HIGH_WATERMARK},  // Messages : 0 to CAPACITY - 1

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         14,
         40,
         RUMState::e_STATE_HIGH_WATERMARK,  // Bytes    : 0 to CAPACITY - 1
         RUMState::e_STATE_FULL},           // Messages : 0 to CAPACITY

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         15,
         40,
         RUMState::e_STATE_FULL,   // Bytes    : 0 to CAPACITY
         RUMState::e_STATE_FULL},  // Messages : 0 to CAPACITY
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": checking 'monitor(" << test.d_byteLowWatermark
                        << ", " << test.d_byteHighWatermark << ", "
                        << test.d_byteCapacity << ", "
                        << test.d_messageLowWatermark << ", "
                        << test.d_messageHighWatermark << ", "
                        << test.d_messageCapacity << ")'\n"
                        << "\t- byteState()\n"
                        << "\t- messageState()\n"
                        << "following\n"
                        << "\tupdate(" << test.d_bytes << ", "
                        << test.d_messages << ")"
                        << "\n");

        mqbu::ResourceUsageMonitor monitor(
            test.d_byteCapacity,
            test.d_messageCapacity,
            test.d_byteLowWatermark * 1.0 / test.d_byteCapacity,
            test.d_byteHighWatermark * 1.0 / test.d_byteCapacity,
            test.d_messageLowWatermark * 1.0 / test.d_messageCapacity,
            test.d_messageHighWatermark * 1.0 / test.d_messageCapacity);

        // Update and check state
        monitor.update(test.d_bytes, test.d_messages);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.byteState(),
                    test.d_expectedByteState);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.messageState(),
                    test.d_expectedMessageState);

        // Test edge case where update value is 0 (This fails in the original
        // 'mqbu::ResourceUsageMonitor' if 'highWatermark == capacity' and the
        // value is exactly at 'highWatermark' when 'update(0)' is called; see
        // the 'else' block in that component's 'update()' method for more
        // info) but shouldn't fail here.
        // This can be verified by replacing in the .cpp file methods
        // 'update<Bytes/Messages>' the 'else if (delta<Bytes/Messages> < 0)'
        // with just 'else'.
        monitor.update(0, 0);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.byteState(),
                    test.d_expectedByteState);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.messageState(),
                    test.d_expectedMessageState);

        // Reverse update and check state
        monitor.update(-test.d_bytes, -test.d_messages);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.byteState(),
                    RUMState::e_STATE_NORMAL);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.messageState(),
                    RUMState::e_STATE_NORMAL);
    }
}

static void test6_stateBothResources()
// ------------------------------------------------------------------------
// STATE MONITOR
//
// Concerns:
//   Ensure that the 'state()' accessor returns the proper current state of
//   the monitor (the highest limit reached for either bytes or messages).
//
// Plan:
//   1 Use the input data from 'test5' (where we asserted that the expected
//     state of the monitor with respect to bytes and messages,
//     respectively, was proper) to ensure that the resulting *monitor*
//     state as reported by 'state()' is as expected per the state of
//     the monitor with respect to bytes and messages, respectively.
//
// Testing:
//   Proper state of the monitor as reported by the following methods:
//     - state()
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("STATE BOTH RESOURCES");

    // TYPES
    typedef mqbu::ResourceUsageMonitorState RUMState;

    // 2
    struct Test {
        int                d_line;
        bsls::Types::Int64 d_byteLowWatermark;
        bsls::Types::Int64 d_byteHighWatermark;
        bsls::Types::Int64 d_byteCapacity;
        bsls::Types::Int64 d_messageLowWatermark;
        bsls::Types::Int64 d_messageHighWatermark;
        bsls::Types::Int64 d_messageCapacity;
        bsls::Types::Int64 d_bytes;
        bsls::Types::Int64 d_messages;
        RUMState::Enum     d_expectedMonitorState;
    } k_DATA[] = {
        // Format:
        //..
        // { <lineNumber>
        // <byteLowWatermark>, <byteHighWatermark>, <byteCapacity>
        // <messageLowWatermark>, <messageHighWatermark>, <messageCapacity>
        // <bytes>, <messages>
        // <expectedMonitorState> }
        //..
        //
        // LOW_WATERMARK < HIGH_WATERMARK and HIGH_WATERMARK < CAPACITY
        {L_, 5, 13, 15, 20, 37, 40, 0, 0, RUMState::e_STATE_NORMAL},  // 0 to 0
                                                                      // (both)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         5,
         20,
         RUMState::e_STATE_NORMAL},  // 0 to LOW_WATERMARK (both)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         12,
         36,
         RUMState::e_STATE_NORMAL},  // 0 to HIGH_WATERMARK - 1 (both)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         13,
         36,
         RUMState::e_STATE_HIGH_WATERMARK},  // 0 to HIGH_WATERMARK (bytes)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         12,
         37,
         RUMState::e_STATE_HIGH_WATERMARK},  // 0 to HIGH_WATERMARK (messages)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         13,
         37,
         RUMState::e_STATE_HIGH_WATERMARK},  // 0 to HIGH_WATERMARK (both)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         14,
         39,
         RUMState::e_STATE_HIGH_WATERMARK},  // 0 to CAPACITY - 1 (both)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         15,
         39,
         RUMState::e_STATE_FULL},  // 0 to CAPACITY (bytes)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         14,
         40,
         RUMState::e_STATE_FULL},  // 0 to CAPACITY (messages)

        {L_,
         5,
         13,
         15,
         20,
         37,
         40,
         15,
         40,
         RUMState::e_STATE_FULL},  // 0 to CAPACITY (both)

        // LOW_WATERMARK < HIGH_WATERMARK and HIGH_WATERMARK == CAPACITY
        {L_, 5, 13, 13, 20, 37, 37, 0, 0, RUMState::e_STATE_NORMAL},  // 0 to 0
                                                                      // (both)

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         5,
         20,
         RUMState::e_STATE_NORMAL},  // 0 to LOW_WATERMARK (both)

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         12,
         36,
         RUMState::e_STATE_NORMAL},  // 0 to HIGH_WATERMARK - 1 (both)

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         13,
         36,
         RUMState::e_STATE_FULL},  // 0 to HIGH_WATERMARK == CAPACITY (bytes)

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         12,
         37,
         RUMState::e_STATE_FULL},  // 0 to HIGH_WATERMARK == CAPACITY
                                   // (messages)

        {L_,
         5,
         13,
         13,
         20,
         37,
         37,
         13,
         37,
         RUMState::e_STATE_FULL},  // 0 to HIGH_WATERMARK == CAPACITY (both)

        // LOW_WATERMARK == HIGH_WATERMARK and HIGH_WATERMARK < CAPACITY
        {L_, 5, 5, 15, 20, 20, 40, 0, 0, RUMState::e_STATE_NORMAL},  // 0 to 0
                                                                     // (both)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         4,
         19,
         RUMState::e_STATE_NORMAL},  // 0 to LOW_WATERMARK - 1 (both)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         5,
         19,
         RUMState::e_STATE_HIGH_WATERMARK},  // 0 to LOW_WATERMARK
                                             //   == HIGH_WATERMARK  (bytes)
        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         4,
         20,
         RUMState::e_STATE_HIGH_WATERMARK},  // 0 to LOW_WATERMARK
                                             //   == HIGH_WATERMARK  (messages)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         5,
         20,
         RUMState::e_STATE_HIGH_WATERMARK},  // 0 to LOW_WATERMARK
                                             //   == HIGH_WATERMARK  (both)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         14,
         39,
         RUMState::e_STATE_HIGH_WATERMARK},  // 0 to CAPACITY - 1 (both)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         15,
         39,
         RUMState::e_STATE_FULL},  // 0 to CAPACITY (bytes)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         15,
         39,
         RUMState::e_STATE_FULL},  // 0 to CAPACITY (messages)

        {L_,
         5,
         5,
         15,
         20,
         20,
         40,
         15,
         40,
         RUMState::e_STATE_FULL},  // 0 to CAPACITY (both)
    };

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test& test = k_DATA[idx];

        PVV(test.d_line << ": checking 'monitor(" << test.d_byteLowWatermark
                        << ", " << test.d_byteHighWatermark << ", "
                        << test.d_byteCapacity << ", "
                        << test.d_messageLowWatermark << ", "
                        << test.d_messageHighWatermark << ", "
                        << test.d_messageCapacity << ")'\n"
                        << "\t- state()\n"
                        << "following\n"
                        << "\tupdate(" << test.d_bytes << ", "
                        << test.d_messages << ")"
                        << "\n");

        mqbu::ResourceUsageMonitor monitor(
            test.d_byteCapacity,
            test.d_messageCapacity,
            test.d_byteLowWatermark * 1.0 / test.d_byteCapacity,
            test.d_byteHighWatermark * 1.0 / test.d_byteCapacity,
            test.d_messageLowWatermark * 1.0 / test.d_messageCapacity,
            test.d_messageHighWatermark * 1.0 / test.d_messageCapacity);

        // Update and check state
        monitor.update(test.d_bytes, test.d_messages);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.state(),
                    test.d_expectedMonitorState);

        // Test edge case where update value is 0 (This fails in the original
        // 'mqbu::ResourceUsageMonitor' if 'highWatermark == capacity' and the
        // value is exactly at 'highWatermark' when 'update(0)' is called; see
        // the 'else' block in that component's 'update()' method for more
        // info) but shouldn't fail here.
        // This can be verified by replacing in the .cpp file methods
        // 'update<Bytes/Messages>' the 'else if (delta<Bytes/Messages> < 0)'
        // with just 'else'.
        monitor.update(0, 0);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.state(),
                    test.d_expectedMonitorState);

        // Reverse update and check state
        monitor.update(-test.d_bytes, -test.d_messages);

        ASSERT_EQ_D("line " << test.d_line,
                    monitor.state(),
                    RUMState::e_STATE_NORMAL);
    }
}

static void test7_jumpingStates()
{
    bmqtst::TestHelper::printTestName("JUMPING STATES");

    // LOW_WATERMARK < HIGH_WATERMARK and HIGH_WATERMARK < CAPACITY
    bsls::Types::Int64 k_BYTE_LOW_WATERMARK  = 5;
    bsls::Types::Int64 k_BYTE_HIGH_WATERMARK = 10;
    bsls::Types::Int64 k_BYTE_CAPACITY       = 15;

    bsls::Types::Int64 k_MESSAGE_LOW_WATERMARK  = 20;
    bsls::Types::Int64 k_MESSAGE_HIGH_WATERMARK = 25;
    bsls::Types::Int64 k_MESSAGE_CAPACITY       = 30;

    // Create a mqbu::ResourceUsageMonitor object
    mqbu::ResourceUsageMonitor monitor(
        k_BYTE_CAPACITY,
        k_MESSAGE_CAPACITY,
        k_BYTE_LOW_WATERMARK * 1.0 / k_BYTE_CAPACITY,
        k_BYTE_HIGH_WATERMARK * 1.0 / k_BYTE_CAPACITY,
        k_MESSAGE_LOW_WATERMARK * 1.0 / k_MESSAGE_CAPACITY,
        k_MESSAGE_HIGH_WATERMARK * 1.0 / k_MESSAGE_CAPACITY);

    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // HIGH_WATERMARK and back: (HIGH_WATERMARK, LOW_WATERMARK)
    ASSERT_EQ(monitor.updateBytes(k_BYTE_HIGH_WATERMARK),
              mqbu::ResourceUsageMonitorStateTransition::e_HIGH_WATERMARK);
    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);

    ASSERT_EQ(monitor.updateMessages(k_MESSAGE_HIGH_WATERMARK),
              mqbu::ResourceUsageMonitorStateTransition::e_HIGH_WATERMARK);
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_HIGH_WATERMARK);

    ASSERT_EQ(monitor.updateBytes(-k_BYTE_HIGH_WATERMARK),
              mqbu::ResourceUsageMonitorStateTransition::e_LOW_WATERMARK);
    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    ASSERT_EQ(monitor.updateMessages(-k_MESSAGE_HIGH_WATERMARK),
              mqbu::ResourceUsageMonitorStateTransition::e_LOW_WATERMARK);
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // CAPACITY and back: (FULL, LOW_WATERMARK)
    ASSERT_EQ(monitor.updateBytes(k_BYTE_CAPACITY),
              mqbu::ResourceUsageMonitorStateTransition::e_FULL);
    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    ASSERT_EQ(monitor.updateMessages(k_MESSAGE_CAPACITY),
              mqbu::ResourceUsageMonitorStateTransition::e_FULL);
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    ASSERT_EQ(monitor.updateBytes(-k_BYTE_CAPACITY),
              mqbu::ResourceUsageMonitorStateTransition::e_LOW_WATERMARK);
    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    ASSERT_EQ(monitor.updateMessages(-k_MESSAGE_CAPACITY),
              mqbu::ResourceUsageMonitorStateTransition::e_LOW_WATERMARK);
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // LOW_WATERMARK < HIGH_WATERMARK and HIGH_WATERMARK == CAPACITY
    k_BYTE_HIGH_WATERMARK    = k_BYTE_CAPACITY;
    k_MESSAGE_HIGH_WATERMARK = k_MESSAGE_CAPACITY;

    monitor.reset(k_BYTE_CAPACITY,
                  k_MESSAGE_CAPACITY,
                  k_BYTE_LOW_WATERMARK,
                  k_BYTE_HIGH_WATERMARK,  // == k_BYTE_CAPACITY
                  k_MESSAGE_LOW_WATERMARK,
                  k_MESSAGE_HIGH_WATERMARK);  // == k_MESSAGE_CAPACITY

    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    // When hitting high watermark, expect a transition to FULL
    // HIGH_WATERMARK and back: (FULL, LOW_WATERMARK)
    ASSERT_EQ(monitor.updateBytes(k_BYTE_HIGH_WATERMARK),
              mqbu::ResourceUsageMonitorStateTransition::e_FULL);
    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    ASSERT_EQ(monitor.updateMessages(k_MESSAGE_HIGH_WATERMARK),
              mqbu::ResourceUsageMonitorStateTransition::e_FULL);
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    ASSERT_EQ(monitor.updateBytes(-k_BYTE_HIGH_WATERMARK),
              mqbu::ResourceUsageMonitorStateTransition::e_LOW_WATERMARK);
    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);

    ASSERT_EQ(monitor.updateMessages(-k_MESSAGE_HIGH_WATERMARK),
              mqbu::ResourceUsageMonitorStateTransition::e_LOW_WATERMARK);
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_NORMAL);
}

static void test8_zeroWatermarksUpdate()
// ------------------------------------------------------------------------
// EQUAL WATERMARKS UPDATE
//
// Concerns:
//   When low, high, and capacity watermarks are all equal to 0 and the
//   value of the resource is larger than 0, an update bringing down the
//   value of the resource to *exactly* 0 (and hence low watermark) should
//   not cause a transition from FULL to LOW_WATERMARK.
//
// Plan:
//   1. Instantiate a ResourceUsageMonitor with equal low, high, and
//      capacity watermarks.
//   2. Increase messages above zero.
//   3. Decrease messages to 0 and verify that the state is still FULL.
//   4. Increase bytes above zero.
//   5. Decrease bytes to 0 and verify that the state is still FULL.
//
// Testing:
//   'update' with low, high, and capacity watermark values all equal to 0.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("EQUAL WATERMARKS UPDATE");

    // 1. Instantiate a ResourceUsageMonitor with equal low, high, and capacity
    //    watermarks.
    bsls::Types::Int64 k_BYTE_LOW_WATERMARK     = 0;
    bsls::Types::Int64 k_BYTE_HIGH_WATERMARK    = 0;
    bsls::Types::Int64 k_BYTE_CAPACITY          = 0;
    bsls::Types::Int64 k_MESSAGE_LOW_WATERMARK  = 0;
    bsls::Types::Int64 k_MESSAGE_HIGH_WATERMARK = 0;
    bsls::Types::Int64 k_MESSAGE_CAPACITY       = 0;

    // Careful! Do not divide by 0!
    mqbu::ResourceUsageMonitor monitor(k_BYTE_CAPACITY,
                                       k_MESSAGE_CAPACITY,
                                       k_BYTE_LOW_WATERMARK,
                                       k_BYTE_HIGH_WATERMARK,
                                       k_MESSAGE_LOW_WATERMARK,
                                       k_MESSAGE_HIGH_WATERMARK);

    ASSERT_EQ(monitor.state(), mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    // 2. Increase messages above zero
    ASSERT_EQ(monitor.update(0, 1),
              mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    ASSERT_EQ(monitor.state(), mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    // 3. Decrease messages to 0 and verify that the state is FULL
    ASSERT_EQ(monitor.update(0, -1),
              mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE)
    ASSERT_EQ(monitor.messageState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    ASSERT_EQ(monitor.state(), mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    // 4. Increase bytes above 0
    ASSERT_EQ(monitor.update(2, 0),
              mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    ASSERT_EQ(monitor.state(), mqbu::ResourceUsageMonitorState::e_STATE_FULL);

    // 5. Decrease bytes to 0 and verify that the state is still FULL
    ASSERT_EQ(monitor.update(-1, 0),
              mqbu::ResourceUsageMonitorStateTransition::e_NO_CHANGE);
    ASSERT_EQ(monitor.byteState(),
              mqbu::ResourceUsageMonitorState::e_STATE_FULL);
    ASSERT_EQ(monitor.state(), mqbu::ResourceUsageMonitorState::e_STATE_FULL);
}

static void test9_print()
// ------------------------------------------------------------------------
// PRINT
//
// Concerns:
//   Ensure that the 'toAscii' methods of 'RUMState' and
//   'RUMStateTransition' work as expected and that the 'print' methods of
//   of 'RUMState', 'RUMStateTransition', and 'RUM' work as expected .
//
// Plan:
//   1 Verify that the 'toAscii' method of 'RUMState' returns the expected
//     string representation of an 'RUMState::Enum' enumeration value.
//   2 Verify that the 'toAscii' method of 'RUMStateTransition' returns the
//     expected string representation of an 'RUMStateTransition::Enum'
//     enumeration value.
//   3 Ensure that the 'print' method of 'RUMState' prints the expected
//     formatted string representation of an 'RUMState::Enum' enumeration
//     value.
//   4 Ensure that the 'print' method of 'RUMStateTransition' prints the
//     expected formatted string representation of an
//     'RUMStateTransition::Enum' value enumeration.
//   5 Ensure that the 'print' method of 'RUM' prints the expected
//     formatted string representation of a 'ResourceUsageMonitor'.
//
// Testing:
//   - static const char *
//     ResourceUsageMonitorState::toAscii(
//                                  ResourceUsageMonitorState::Enum value);
//   - static const char *
//     ResourceUsageMonitorStateTransition::toAscii(
//                        ResourceUsageMonitorStateTransition::Enum value);
//   - static bsl::ostream&
//     ResourceUsageMonitorState::print(
//                     bsl::ostream&                   stream,
//                     ResourceUsageMonitorState::Enum value,
//                     int                             level = 0,
//                     int                             spacesPerLevel = 4);
//   - static bsl::ostream&
//     ResourceUsageMonitorStateTransition::print(
//           bsl::ostream&                             stream,
//           ResourceUsageMonitorStateTransition::Enum value,
//           int                                       level = 0,
//           int                                       spacesPerLevel = 4);
//   - bsl::ostream&
//     ResourceUsageMonitor::print(bsl::ostream& stream,
//                                 int           level = 0,
//                                 int           spacesPerLevel = 4) const;
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("PRINT");

    // 1
    {
        PV("TO ASCII - STATE");

        // TYPES
        typedef mqbu::ResourceUsageMonitorState RUMState;

        struct Test {
            int            d_line;
            RUMState::Enum d_value;
            const char*    d_expected;
        } k_DATA[] = {
            {L_, RUMState::e_STATE_NORMAL, "STATE_NORMAL"},
            {L_, RUMState::e_STATE_HIGH_WATERMARK, "STATE_HIGH_WATERMARK"},
            {L_, RUMState::e_STATE_FULL, "STATE_FULL"},
            {L_, static_cast<RUMState::Enum>(-1), "(* UNKNOWN *)"}};

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];
            bsl::string ascii(s_allocator_p);

            PVV(test.d_line << ": checking 'RUMState::toAscii("
                            << "RUMState::e_" << test.d_expected << ")';");

            ascii = RUMState::toAscii(test.d_value);

            ASSERT_EQ_D("line " << test.d_line, ascii, test.d_expected);

            PVV("    " << ascii);
        }
    }

    // 2
    {
        PV("TO ASCII - STATE TRANSITION");

        // TYPES
        typedef mqbu::ResourceUsageMonitorStateTransition RUMStateTransition;

        struct Test {
            int                      d_line;
            RUMStateTransition::Enum d_value;
            const char*              d_expected;
        } k_DATA[] = {
            {L_, RUMStateTransition::e_NO_CHANGE, "NO_CHANGE"},
            {L_, RUMStateTransition::e_LOW_WATERMARK, "LOW_WATERMARK"},
            {L_, RUMStateTransition::e_HIGH_WATERMARK, "HIGH_WATERMARK"},
            {L_, RUMStateTransition::e_FULL, "FULL"},
            {L_, static_cast<RUMStateTransition::Enum>(-1), "(* UNKNOWN *)"}};

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test& test = k_DATA[idx];
            bsl::string ascii(s_allocator_p);

            PVV(test.d_line << ": checking 'RUMStateTransition::toAscii("
                            << "RUMStateTransition::e_" << test.d_expected
                            << ")';");

            ascii = RUMStateTransition::toAscii(test.d_value);

            ASSERT_EQ_D("line " << test.d_line, ascii, test.d_expected);

            PVV("    " << ascii);
        }
    }

    // 3
    {
        PV("PRINT - STATE");

        // TYPES
        typedef mqbu::ResourceUsageMonitorState RUMState;

        struct Test {
            int            d_line;
            RUMState::Enum d_value;
            const char*    d_expected;
        } k_DATA[] = {
            {L_, RUMState::e_STATE_NORMAL, "STATE_NORMAL"},
            {L_, RUMState::e_STATE_HIGH_WATERMARK, "STATE_HIGH_WATERMARK"},
            {L_, RUMState::e_STATE_FULL, "STATE_FULL"}};

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&        test = k_DATA[idx];
            bsl::string        ascii(s_allocator_p);
            bmqu::MemOutStream ss(s_allocator_p);

            PVV(test.d_line << ": checking 'RUMState::print(ostream, "
                            << "RUMState::e_" << test.d_expected
                            << ", 0, -1)';");

            RUMState::print(ss, test.d_value, 0, -1);

            ASSERT_EQ_D("line " << test.d_line, ss.str(), test.d_expected);

            // operator <<
            PVV(test.d_line << ": checking 'operator<<(ostream, "
                            << "mqbu::RUMState::Enum)'");

            ss.reset();

            ss << test.d_value;

            ASSERT_EQ_D("line " << test.d_line, ss.str(), test.d_expected);

            // Set bad bit and ensure nothing was printed.
            ss.reset();
            ss.setstate(bsl::ios_base::badbit);
            RUMState::print(ss, test.d_value, 0, -1);
            ASSERT_EQ_D("line" << test.d_line, ss.str(), "");
        }
    }

    // 4
    {
        PV("PRINT - STATE TRANSITION");

        // TYPES
        typedef mqbu::ResourceUsageMonitorStateTransition RUMStateTransition;

        struct Test {
            int                      d_line;
            RUMStateTransition::Enum d_value;
            const char*              d_expected;
        } k_DATA[] = {
            {L_, RUMStateTransition::e_NO_CHANGE, "NO_CHANGE"},
            {L_, RUMStateTransition::e_LOW_WATERMARK, "LOW_WATERMARK"},
            {L_, RUMStateTransition::e_HIGH_WATERMARK, "HIGH_WATERMARK"},
            {L_, RUMStateTransition::e_FULL, "FULL"}};

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&        test = k_DATA[idx];
            bmqu::MemOutStream ss(s_allocator_p);

            PVV(test.d_line << ": checking 'RUMStateTransition::print(ostream"
                            << ", RUMStateTransition::e_" << test.d_expected
                            << ", 0, -1)';");

            RUMStateTransition::print(ss, test.d_value, 0, -1);

            ASSERT_EQ_D("line " << test.d_line, ss.str(), test.d_expected);

            PVV(test.d_line << ": checking 'operator<<(ostream, "
                            << "mqbu::RUMStateTransition::Enum)");

            ss.reset();

            ss << test.d_value;

            ASSERT_EQ_D("line " << test.d_line, ss.str(), test.d_expected);

            // Set bad bit and ensure nothing was printed.
            ss.reset();
            ss.setstate(bsl::ios_base::badbit);
            RUMStateTransition::print(ss, test.d_value, 0, -1);
            ASSERT_EQ_D("line" << test.d_line, ss.str(), "");
        }
    }

    // 5
    {
        PV("PRINT");

        struct Test {
            int                d_line;
            bsls::Types::Int64 d_byteLowWatermark;
            bsls::Types::Int64 d_byteHighWatermark;
            bsls::Types::Int64 d_byteCapacity;
            bsls::Types::Int64 d_messageLowWatermark;
            bsls::Types::Int64 d_messageHighWatermark;
            bsls::Types::Int64 d_messageCapacity;
            const char* d_expectedLowWatermark;   // string output by 'print()'
            const char* d_expectedHighWatermark;  // string output by 'print()'
            const char* d_expectedCapacity;       // string output by 'print()'
        } k_DATA[] = {
            // Format:
            //..
            // { <lineNumber>
            // <byteLowWatermark>, <byteHighWatermark>, <byteCapacity>
            // <messageLowWatermark>, <messageHighWatermark>, <messageCapacity>
            // <expectedLowWatermark>
            // <expectedHighWatermark>
            // <expectedCapacity> }
            //..
            //
            // LOW_WATERMARK < HIGH_WATERMARK and HIGH_WATERMARK < CAPACITY
            // Bytes (B)
            {L_,
             5,
             137,
             933,
             5,
             137,
             933,
             " STATE_NORMAL"
             " [Messages (STATE_NORMAL): 0 (5 - 137 - 933)"
             ", Bytes (STATE_NORMAL): 0  B (5  B - 137  B - 933  B)] ",
             " STATE_HIGH_WATERMARK"
             " [Messages (STATE_HIGH_WATERMARK): 137 (5 - 137 - 933)"
             ", Bytes (STATE_HIGH_WATERMARK): 137  B (5  B - 137  B - 933  "
             "B)] ",
             " STATE_FULL"
             " [Messages (STATE_FULL): 933 (5 - 137 - 933)"
             ", Bytes (STATE_FULL): 933  B (5  B - 137  B - 933  B)] "},
            // Kilobytes (KB)
            {L_,
             5000,
             137137,
             933933,
             5000,
             137137,
             933933,
             " STATE_NORMAL"
             " [Messages (STATE_NORMAL): 0 (5,000 - 137,137 - 933,933)"
             ", Bytes (STATE_NORMAL): 0  B"
             " (4.88 KB - 133.92 KB - 912.04 KB)] ",
             " STATE_HIGH_WATERMARK"
             " [Messages (STATE_HIGH_WATERMARK): 137,137"
             " (5,000 - 137,137 - 933,933)"
             ", Bytes (STATE_HIGH_WATERMARK): 133.92 KB"
             " (4.88 KB - 133.92 KB - 912.04 KB)] ",
             " STATE_FULL"
             " [Messages (STATE_FULL): 933,933 (5,000 - 137,137 - 933,933)"
             ", Bytes (STATE_FULL): 912.04 KB"
             " (4.88 KB - 133.92 KB - 912.04 KB)] "},
            // Megabyte (MB)
            {L_,
             5000000,
             137137137,
             933933933,
             5000000,
             137137137,
             933933933,
             " STATE_NORMAL"
             " [Messages (STATE_NORMAL): 0"
             " (5,000,000 - 137,137,137 - 933,933,933)"
             ", Bytes (STATE_NORMAL): 0  B"
             " (4.77 MB - 130.78 MB - 890.67 MB)] ",
             " STATE_HIGH_WATERMARK"
             " [Messages (STATE_HIGH_WATERMARK): 137,137,137"
             " (5,000,000 - 137,137,137 - 933,933,933)"
             ", Bytes (STATE_HIGH_WATERMARK): 130.78 MB"
             " (4.77 MB - 130.78 MB - 890.67 MB)] ",
             " STATE_FULL"
             " [Messages (STATE_FULL): 933,933,933"
             " (5,000,000 - 137,137,137 - 933,933,933)"
             ", Bytes (STATE_FULL): 890.67 MB"
             " (4.77 MB - 130.78 MB - 890.67 MB)] "}};

        const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

        for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
            const Test&        test = k_DATA[idx];
            bmqu::MemOutStream ss(s_allocator_p);

            PVV(test.d_line << ": checking 'monitor.print(ostream)'");

            mqbu::ResourceUsageMonitor monitor(
                test.d_byteCapacity,
                test.d_messageCapacity,
                test.d_byteLowWatermark * 1.0 / test.d_byteCapacity,
                test.d_byteHighWatermark * 1.0 / test.d_byteCapacity,
                test.d_messageLowWatermark * 1.0 / test.d_messageCapacity,
                test.d_messageHighWatermark * 1.0 / test.d_messageCapacity);

            // STATE_NORMAL
            monitor.print(ss, 0, -1);

            ASSERT_EQ_D("line " << test.d_line,
                        ss.str(),
                        test.d_expectedLowWatermark);

            ss.reset();

            ss << monitor;

            ASSERT_EQ_D("line " << test.d_line,
                        ss.str(),
                        test.d_expectedLowWatermark);

            // STATE_HIGH_WATERMARK
            ss.reset();

            monitor.update(test.d_byteHighWatermark,
                           test.d_messageHighWatermark);
            monitor.print(ss, 0, -1);

            ASSERT_EQ_D("line " << test.d_line,
                        ss.str(),
                        test.d_expectedHighWatermark);

            ss.reset();

            ss << monitor;

            ASSERT_EQ_D("line " << test.d_line,
                        ss.str(),
                        test.d_expectedHighWatermark);

            // STATE_FULL
            ss.reset();

            monitor.reset();
            monitor.update(test.d_byteCapacity, test.d_messageCapacity);
            monitor.print(ss, 0, -1);

            ASSERT_EQ_D("line " << test.d_line,
                        ss.str(),
                        test.d_expectedCapacity);

            ss.reset();

            ss << monitor;

            ASSERT_EQ_D("line " << test.d_line,
                        ss.str(),
                        test.d_expectedCapacity);
        }
    }
}

static void test10_usageExample()
// ------------------------------------------------------------------------
// USAGE EXAMPLE
//
// Concerns:
//   Ensure that the usage example compiles and works.
//
// Plan:
//   1 Run the example's code with appropriate values and verify that
//     the monitor's behavior and state are as expected.
//
// Testing:
//   Usage example
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("USAGE EXAMPLE");

    // First, create the monitor.

    // Configuration parameters
    bsls::Types::Int64 k_BYTE_LOW_WATERMARK     = 80;
    bsls::Types::Int64 k_BYTE_HIGH_WATERMARK    = 100;
    bsls::Types::Int64 k_BYTE_CAPACITY          = 110;
    bsls::Types::Int64 k_MESSAGE_LOW_WATERMARK  = 4;
    bsls::Types::Int64 k_MESSAGE_HIGH_WATERMARK = 10;
    bsls::Types::Int64 k_MESSAGE_CAPACITY       = 11;

    // Create a mqbu::ResourceUsageMonitor object
    mqbu::ResourceUsageMonitor monitor(
        k_BYTE_CAPACITY,
        k_MESSAGE_CAPACITY,
        k_BYTE_LOW_WATERMARK * 1.0 / k_BYTE_CAPACITY,
        k_BYTE_HIGH_WATERMARK * 1.0 / k_BYTE_CAPACITY,
        k_MESSAGE_LOW_WATERMARK * 1.0 / k_MESSAGE_CAPACITY,
        k_MESSAGE_HIGH_WATERMARK * 1.0 / k_MESSAGE_CAPACITY);

    // Then, we check to see if we've hit our high watermark or capacity before
    // sending a message (i.e. we're no longer in 'NORMAL' state).

    // TYPE
    typedef mqbu::ResourceUsageMonitorState RUMState;

    // Check resource usage state
    bool highWatermarkLimitReached = monitor.state() !=
                                     RUMState::e_STATE_NORMAL;

    ASSERT(!highWatermarkLimitReached);

    if (!highWatermarkLimitReached) {
        // Send message(s)

        // Update resource usage
        monitor.update(110, 11);
    }
    else {
        // Save it to send later
    }

    // After hitting high watermark or capacity, we decrease our resource usage
    // to a point that takes us below our low watermark (and back to the
    // 'NORMAL' state), so we check for that and resume sending messages if
    // that's the case.

    // TYPE
    typedef mqbu::ResourceUsageMonitorStateTransition RUMStateTransition;

    // Update resource usage state (e.g. confirmed message from client)
    RUMStateTransition::Enum change = monitor.update(-30, -7);

    ASSERT_EQ(change, RUMStateTransition::e_LOW_WATERMARK);

    // Check if we're back to below our low watermark
    if (change == RUMStateTransition::e_LOW_WATERMARK) {
        // Resume sending messages
    }
}

}  // close unnamed namespace

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 10: test10_usageExample(); break;
    case 9: test9_print(); break;
    case 8: test8_zeroWatermarksUpdate(); break;
    case 7: test7_jumpingStates(); break;
    case 6: test6_stateBothResources(); break;
    case 5: test5_stateOneResource(); break;
    case 4: test4_updateBothResources(); break;
    case 3: test3_updateOneResource(); break;
    case 2: test2_zeroCapacity(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
