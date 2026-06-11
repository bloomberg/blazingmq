// Copyright 2026 Bloomberg Finance L.P.
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

#ifndef INCLUDED_BMQTSK_ALARMLOG
#define INCLUDED_BMQTSK_ALARMLOG

//@PURPOSE: Provide macros for emitting ALARM log records.
//
//@MACROS:
//  BMQTSK_ALARMLOG_ALARM:     Macro for emitting a throttled regular alarm
//  BMQTSK_ALARMLOG_PANIC:     Macro for emitting a throttled 'PANIC' alarm
//  BMQTSK_ALARMLOG_RAW_ALARM: Macro for emitting a non-throttled regular alarm
//  BMQTSK_ALARMLOG_RAW_PANIC: Macro for emitting a non-throttled 'PANIC' alarm
//  BMQTSK_ALARMLOG_END:       Terminal macro to close an alarm block
//
//@SEE_ALSO: bmqtsk_alarmlogobserver
//
//@DESCRIPTION: This component provides macros for generating alarm log
// records.  Two macros, 'BMQTSK_ALARMLOG_ALARM' and 'BMQTSK_ALARMLOG_PANIC'
// are provided in order to facilitate generation of rate-controlled
// (throttled) alarms.  The alarms are rate-controlled so that no more than
// *one* alarm per minute, for the same source, will be generated.
// Additionally, two macros, 'BMQTSK_ALARMLOG_RAW_ALARM' and
// 'BMQTSK_ALARMLOG_RAW_PANIC' are provided in order to facilitate generation
// of non-throttled alarms.  These alarms are always immediately generated. The
// alarm records produced by these macros are consumed by
// 'bmqtsk::AlarmLogObserver' (see 'bmqtsk_alarmlogobserver').
//
/// Usage Example
///-------------
// The following example illustrates how to use the alarm macros:
//..
//  BMQTSK_ALARMLOG_PANIC("MyErrorCategory")
//      << "Unable to start task [rc: " << rc << "]"
//      << BMQTSK_ALARMLOG_END;
//..

// BDE
#include <ball_log.h>

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
//:  o the call must be ended by a terminal 'BMQTSK_ALARMLOG_END'
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMQTSK_ALARMLOG_ALARM(CATEGORY)                                       \
    BALL_LOG_ERROR_BLOCK                                                      \
    {                                                                         \
        BALL_LOG_OUTPUT_STREAM                                                \
            << "ALARM [" << CATEGORY                                          \
            << "] "; /* NOLINT(bugprone-macro-parentheses) */                 \
        BALL_LOG_RECORD->customFields().appendString("ALARM");                \
        BALL_LOG_OUTPUT_STREAM

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMQTSK_ALARMLOG_PANIC(CATEGORY)                                       \
    BALL_LOG_ERROR_BLOCK                                                      \
    {                                                                         \
        BALL_LOG_OUTPUT_STREAM                                                \
            << "PANIC [" << CATEGORY                                          \
            << "] "; /* NOLINT(bugprone-macro-parentheses) */                 \
        BALL_LOG_RECORD->customFields().appendString("PANIC");                \
        BALL_LOG_OUTPUT_STREAM

// The following two macros provide a simple wrapper on top of the
// 'BALL_LOG_ERROR_BLOCK' macro as above, setting up the alarm identification
// field, only doing so in such a way that alarm generation is not throttled.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMQTSK_ALARMLOG_RAW_ALARM(CATEGORY)                                   \
    BALL_LOG_ERROR_BLOCK                                                      \
    {                                                                         \
        BALL_LOG_OUTPUT_STREAM                                                \
            << "ALARM [" << CATEGORY                                          \
            << "] "; /* NOLINT(bugprone-macro-parentheses) */                 \
        BALL_LOG_RECORD->customFields().appendString("RAW_ALARM");            \
        BALL_LOG_OUTPUT_STREAM

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMQTSK_ALARMLOG_RAW_PANIC(CATEGORY)                                   \
    BALL_LOG_ERROR_BLOCK                                                      \
    {                                                                         \
        BALL_LOG_OUTPUT_STREAM                                                \
            << "PANIC [" << CATEGORY                                          \
            << "] "; /* NOLINT(bugprone-macro-parentheses) */                 \
        BALL_LOG_RECORD->customFields().appendString("RAW_PANIC");            \
        BALL_LOG_OUTPUT_STREAM

// Terminal macro to close the alarm block opened by any of the above macros.
#define BMQTSK_ALARMLOG_END                                                   \
    "";                                                                       \
    }

#endif
