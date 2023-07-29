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

// mwctsk_alarmlog.h                                                  -*-C++-*-
#ifndef INCLUDED_MWCTSK_ALARMLOG
#define INCLUDED_MWCTSK_ALARMLOG

//@PURPOSE: Provide a BALL observer for ALARMS with rate-controlled output.
//
//@CLASSES:
//  mwctsk::AlarmLog: BALL observer emitting alarms to stderr
//
//@MACROS:
//  MWCTSK_ALARMLOG_ALARM:     Macro for emitting a throttled regular alarm
//  MWCTSK_ALARMLOG_PANIC:     Macro for emitting a throttled 'PANIC' alarm
//  MWCTSK_ALARMLOG_RAW_ALARM: Macro for emitting a non-throttled regular alarm
//  MWCTSK_ALARMLOG_RAW_PANIC: Macro for emitting a non-throttled 'PANIC' alarm
//
//@DESCRIPTION: 'mwctsk::AlarmLog' implements the 'ball::ObserverAdapter'
// protocol to monitor ball logs.  If a log record matches specific criteria,
// it will be dumped to 'stderr' so that some external monitoring systems can
// trigger system alerts about those events.  Two macros,
// 'MWCTSK_ALARMLOG_ALARM' and 'MWCTSK_ALARMLOG_PANIC' are provided in order to
// facilitate generation of rate-contolled (throttled) alarms.  The alarms are
// rate-controlled so that no more than *one* alarm per minute, for the same
// source, will be generated.  Additionally, two macros,
// 'MWCTSK_ALARMLOG_RAW_ALARM' and 'MWCTSK_ALARMLOG_RAW_PANIC' are provided in
// order to facilitate generation of non-throttled alarms.  These alarms are
// always immediately generated.  Finally, note that the 'mwctsk::AlarmLog'
// must be registered to the BALL logging infrastructure.
//
/// Thread-safety
///-------------
// This object is *thread* *enabled*, meaning that two threads can safely call
// any methods on the *same* *instance* without external synchronization.
//
/// Implementation Notes
///--------------------
// The implementation leverages the 'customFields' on a ball record: the two
// 'MWCTSK_ALARMLOG_ALARM' and 'MWCTSK_ALARMLOG_PANIC' macros are simply using
// the default 'BALL_LOG_ERROR_BLOCK' macro to create a 'ballRecord', and
// setting its first customField to the corresponding string.
//
// The alarms are rate-controlled, by authorizing at most one alarm per source,
// the source being identified by the file and line producing the log.
//
/// Usage Example
///-------------
// The following example illustrates how to typically instantiate and use a
// 'LogAlarm'.
//
// First, we create a 'LogAlarm' object:
//..
//  mwctsk::LogAlarm logAlarm(allocator);
//..
//
// Then, we need to register this Observer to the BALL logging infrastructure.
// Note that the following code assumed the 'ball::LoggerManager' singleton was
// initialized with a 'ball::MultiplexObserver'.
//
//..
//  ball::MultiplexObserver *ballObserver =
//      dynamic_cast<ball::MultiplexObserver*>(
//          ball::LoggerManager::singleton().observer());
//  if (!ballObserver) {
//      BALL_LOG_ERROR << "Unable to register logAlarm [reason: "
//                     << "'ball::LoggerManager::observer()' is NOT a "
//                     << "multiplexObserver]";
//  } else {
//      ballObserver->registerObserver(&logAlarm);
//  }
//..
//
// We can now use the macros to emit alarms:
//..
//  MWCTSK_LOGALARM_PANIC("MyErrorCategory")
//      << "Unable to start task [rc: " << rc << "]"
//      << MWCTSK_LOGALARM_END;
//..
//
// Finally, before destruction we must unregister the observer.
//..
//  ball::MultiplexObserver *ballObserver =
//      dynamic_cast<ball::MultiplexObserver*>(
//          ball::LoggerManager::singleton().observer());
//  if (ballObserver) {
//      ballObserver->deregisterObserver(&logAlarm);
//  }
//..
//

// MWC

// BDE
#include <ball_log.h>
#include <ball_observeradapter.h>
#include <ball_record.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_mutex.h>
#include <bsls_types.h>

// ==================
// MACROS DEFINITIONS
// ==================

// The following two macros provide a simple wrapper on top of the
// 'BALL_LOG_ERROR_BLOCK' macro, setting up the alarm identification field.
// The specified 'CATEGORY' corresponds to the category of that alarm (for
// standardizing on the representation).  Those macros behave, and are to be
// used, exactly the same way as the scope and 'BALL_LOG_OUTPUT_STREAM' in
// 'BALL_LOG_ERROR_BLOCK', meaning that:
//:  o log message is streamed into the macro
//:  o the call must be ended by a terminal 'MWCTSK_ALARMLOG_END'
#define MWCTSK_ALARMLOG_ALARM(CATEGORY)                                       \
    BALL_LOG_ERROR_BLOCK                                                      \
    {                                                                         \
        BALL_LOG_OUTPUT_STREAM << "ALARM [" << CATEGORY << "] ";              \
        BALL_LOG_RECORD->customFields().appendString("ALARM");                \
        BALL_LOG_OUTPUT_STREAM
#define MWCTSK_ALARMLOG_PANIC(CATEGORY)                                       \
    BALL_LOG_ERROR_BLOCK                                                      \
    {                                                                         \
        BALL_LOG_OUTPUT_STREAM << "PANIC [" << CATEGORY << "] ";              \
        BALL_LOG_RECORD->customFields().appendString("PANIC");                \
        BALL_LOG_OUTPUT_STREAM
#define MWCTSK_ALARMLOG_END                                                   \
    "";                                                                       \
    }

// The following two macros provide a simple wrapper on top of the
// 'BALL_LOG_ERROR_BLOCK' macro as above, setting up the alarm identification
// field, only doing so in such a way that alarm generation is not throttled.
#define MWCTSK_ALARMLOG_RAW_ALARM(CATEGORY)                                   \
    BALL_LOG_ERROR_BLOCK                                                      \
    {                                                                         \
        BALL_LOG_OUTPUT_STREAM << "ALARM [" << CATEGORY << "] ";              \
        BALL_LOG_RECORD->customFields().appendString("RAW_ALARM");            \
        BALL_LOG_OUTPUT_STREAM
#define MWCTSK_ALARMLOG_RAW_PANIC(CATEGORY)                                   \
    BALL_LOG_ERROR_BLOCK                                                      \
    {                                                                         \
        BALL_LOG_OUTPUT_STREAM << "PANIC [" << CATEGORY << "] ";              \
        BALL_LOG_RECORD->customFields().appendString("RAW_PANIC");            \
        BALL_LOG_OUTPUT_STREAM

namespace BloombergLP {
namespace mwctsk {

// ==============
// class AlarmLog
// ==============

/// BALL observer emitting rate-controlled alarms to stderr
class AlarmLog : public ball::ObserverAdapter {
  private:
    // PRIVATE TYPES

    /// Map of log source identity to last alarm emitting time.
    typedef bsl::unordered_map<bsl::string, bsls::Types::Int64>
        CategoryTimeMap;

    // DATA
    bslma::Allocator* d_allocator_p;
    // Allocator to use, held not owned.

    CategoryTimeMap d_lastAlarmTime;
    // Map of last emit time of all encountered alarms.

    bslmt::Mutex d_mutex;
    // Mutex for thread-safety of this component.

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    AlarmLog(const AlarmLog&);             // = delete;
    AlarmLog& operator=(const AlarmLog&);  // = delete;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(AlarmLog, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object, using the specified `allocator`.
    explicit AlarmLog(bslma::Allocator* allocator);

    /// Destroy this object.
    ~AlarmLog() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    //  (virtual: ball::ObserverAdapter)

    /// Process the specified log `record` having the specified publishing
    /// `context`.  If `record` correspond to an alarm record, its message
    /// will be dumped to stderr with respects to rate-controlled logic.
    void publish(const ball::Record&  record,
                 const ball::Context& context) BSLS_KEYWORD_OVERRIDE;
};

}  // close package namespace
}  // close enterprise namespace

#endif
