// Copyright 2018-2024 Bloomberg Finance L.P.
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

// mqbcfg_messages.cpp            *DO NOT EDIT*            @generated -*-C++-*-

#include <mqbcfg_messages.h>

#include <bdlat_formattingmode.h>
#include <bdlat_valuetypefunctions.h>
#include <bdlb_print.h>
#include <bdlb_printmethods.h>
#include <bdlb_string.h>

#include <bdlb_nullablevalue.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslim_printer.h>
#include <bsls_assert.h>
#include <bsls_types.h>

#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_limits.h>
#include <bsl_ostream.h>
#include <bsl_utility.h>

namespace BloombergLP {
namespace mqbcfg {

// -------------------
// class AllocatorType
// -------------------

// CONSTANTS

const char AllocatorType::CLASS_NAME[] = "AllocatorType";

const bdlat_EnumeratorInfo AllocatorType::ENUMERATOR_INFO_ARRAY[] = {
    {AllocatorType::NEWDELETE, "NEWDELETE", sizeof("NEWDELETE") - 1, ""},
    {AllocatorType::COUNTING, "COUNTING", sizeof("COUNTING") - 1, ""},
    {AllocatorType::STACKTRACETEST,
     "STACKTRACETEST",
     sizeof("STACKTRACETEST") - 1,
     ""}};

// CLASS METHODS

int AllocatorType::fromInt(AllocatorType::Value* result, int number)
{
    switch (number) {
    case AllocatorType::NEWDELETE:
    case AllocatorType::COUNTING:
    case AllocatorType::STACKTRACETEST:
        *result = static_cast<AllocatorType::Value>(number);
        return 0;
    default: return -1;
    }
}

int AllocatorType::fromString(AllocatorType::Value* result,
                              const char*           string,
                              int                   stringLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            AllocatorType::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<AllocatorType::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* AllocatorType::toString(AllocatorType::Value value)
{
    switch (value) {
    case NEWDELETE: {
        return "NEWDELETE";
    }
    case COUNTING: {
        return "COUNTING";
    }
    case STACKTRACETEST: {
        return "STACKTRACETEST";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -------------------
// class BmqconfConfig
// -------------------

// CONSTANTS

const char BmqconfConfig::CLASS_NAME[] = "BmqconfConfig";

const bdlat_AttributeInfo BmqconfConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CACHE_T_T_L_SECONDS,
     "cacheTTLSeconds",
     sizeof("cacheTTLSeconds") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* BmqconfConfig::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            BmqconfConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* BmqconfConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CACHE_T_T_L_SECONDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CACHE_T_T_L_SECONDS];
    default: return 0;
    }
}

// CREATORS

BmqconfConfig::BmqconfConfig()
: d_cacheTTLSeconds()
{
}

// MANIPULATORS

void BmqconfConfig::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_cacheTTLSeconds);
}

// ACCESSORS

bsl::ostream&
BmqconfConfig::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("cacheTTLSeconds", this->cacheTTLSeconds());
    printer.end();
    return stream;
}

// -----------------------
// class ClusterAttributes
// -----------------------

// CONSTANTS

const char ClusterAttributes::CLASS_NAME[] = "ClusterAttributes";

const bool ClusterAttributes::DEFAULT_INITIALIZER_IS_C_S_L_MODE_ENABLED =
    false;

const bool ClusterAttributes::DEFAULT_INITIALIZER_IS_F_S_M_WORKFLOW = false;

const bdlat_AttributeInfo ClusterAttributes::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_IS_C_S_L_MODE_ENABLED,
     "isCSLModeEnabled",
     sizeof("isCSLModeEnabled") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_IS_F_S_M_WORKFLOW,
     "isFSMWorkflow",
     sizeof("isFSMWorkflow") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterAttributes::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterAttributes::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterAttributes::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_IS_C_S_L_MODE_ENABLED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_C_S_L_MODE_ENABLED];
    case ATTRIBUTE_ID_IS_F_S_M_WORKFLOW:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_F_S_M_WORKFLOW];
    default: return 0;
    }
}

// CREATORS

ClusterAttributes::ClusterAttributes()
: d_isCSLModeEnabled(DEFAULT_INITIALIZER_IS_C_S_L_MODE_ENABLED)
, d_isFSMWorkflow(DEFAULT_INITIALIZER_IS_F_S_M_WORKFLOW)
{
}

// MANIPULATORS

void ClusterAttributes::reset()
{
    d_isCSLModeEnabled = DEFAULT_INITIALIZER_IS_C_S_L_MODE_ENABLED;
    d_isFSMWorkflow    = DEFAULT_INITIALIZER_IS_F_S_M_WORKFLOW;
}

// ACCESSORS

bsl::ostream& ClusterAttributes::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("isCSLModeEnabled", this->isCSLModeEnabled());
    printer.printAttribute("isFSMWorkflow", this->isFSMWorkflow());
    printer.end();
    return stream;
}

// --------------------------
// class ClusterMonitorConfig
// --------------------------

// CONSTANTS

const char ClusterMonitorConfig::CLASS_NAME[] = "ClusterMonitorConfig";

const int ClusterMonitorConfig::DEFAULT_INITIALIZER_MAX_TIME_LEADER = 60;

const int ClusterMonitorConfig::DEFAULT_INITIALIZER_MAX_TIME_MASTER = 120;

const int ClusterMonitorConfig::DEFAULT_INITIALIZER_MAX_TIME_NODE = 120;

const int ClusterMonitorConfig::DEFAULT_INITIALIZER_MAX_TIME_FAILOVER = 600;

const int ClusterMonitorConfig::DEFAULT_INITIALIZER_THRESHOLD_LEADER = 30;

const int ClusterMonitorConfig::DEFAULT_INITIALIZER_THRESHOLD_MASTER = 60;

const int ClusterMonitorConfig::DEFAULT_INITIALIZER_THRESHOLD_NODE = 60;

const int ClusterMonitorConfig::DEFAULT_INITIALIZER_THRESHOLD_FAILOVER = 300;

const bdlat_AttributeInfo ClusterMonitorConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_MAX_TIME_LEADER,
     "maxTimeLeader",
     sizeof("maxTimeLeader") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_TIME_MASTER,
     "maxTimeMaster",
     sizeof("maxTimeMaster") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_TIME_NODE,
     "maxTimeNode",
     sizeof("maxTimeNode") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_TIME_FAILOVER,
     "maxTimeFailover",
     sizeof("maxTimeFailover") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_THRESHOLD_LEADER,
     "thresholdLeader",
     sizeof("thresholdLeader") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_THRESHOLD_MASTER,
     "thresholdMaster",
     sizeof("thresholdMaster") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_THRESHOLD_NODE,
     "thresholdNode",
     sizeof("thresholdNode") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_THRESHOLD_FAILOVER,
     "thresholdFailover",
     sizeof("thresholdFailover") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterMonitorConfig::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 8; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterMonitorConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterMonitorConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_MAX_TIME_LEADER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_LEADER];
    case ATTRIBUTE_ID_MAX_TIME_MASTER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_MASTER];
    case ATTRIBUTE_ID_MAX_TIME_NODE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_NODE];
    case ATTRIBUTE_ID_MAX_TIME_FAILOVER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_TIME_FAILOVER];
    case ATTRIBUTE_ID_THRESHOLD_LEADER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_LEADER];
    case ATTRIBUTE_ID_THRESHOLD_MASTER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_MASTER];
    case ATTRIBUTE_ID_THRESHOLD_NODE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_NODE];
    case ATTRIBUTE_ID_THRESHOLD_FAILOVER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THRESHOLD_FAILOVER];
    default: return 0;
    }
}

// CREATORS

ClusterMonitorConfig::ClusterMonitorConfig()
: d_maxTimeLeader(DEFAULT_INITIALIZER_MAX_TIME_LEADER)
, d_maxTimeMaster(DEFAULT_INITIALIZER_MAX_TIME_MASTER)
, d_maxTimeNode(DEFAULT_INITIALIZER_MAX_TIME_NODE)
, d_maxTimeFailover(DEFAULT_INITIALIZER_MAX_TIME_FAILOVER)
, d_thresholdLeader(DEFAULT_INITIALIZER_THRESHOLD_LEADER)
, d_thresholdMaster(DEFAULT_INITIALIZER_THRESHOLD_MASTER)
, d_thresholdNode(DEFAULT_INITIALIZER_THRESHOLD_NODE)
, d_thresholdFailover(DEFAULT_INITIALIZER_THRESHOLD_FAILOVER)
{
}

// MANIPULATORS

void ClusterMonitorConfig::reset()
{
    d_maxTimeLeader     = DEFAULT_INITIALIZER_MAX_TIME_LEADER;
    d_maxTimeMaster     = DEFAULT_INITIALIZER_MAX_TIME_MASTER;
    d_maxTimeNode       = DEFAULT_INITIALIZER_MAX_TIME_NODE;
    d_maxTimeFailover   = DEFAULT_INITIALIZER_MAX_TIME_FAILOVER;
    d_thresholdLeader   = DEFAULT_INITIALIZER_THRESHOLD_LEADER;
    d_thresholdMaster   = DEFAULT_INITIALIZER_THRESHOLD_MASTER;
    d_thresholdNode     = DEFAULT_INITIALIZER_THRESHOLD_NODE;
    d_thresholdFailover = DEFAULT_INITIALIZER_THRESHOLD_FAILOVER;
}

// ACCESSORS

bsl::ostream& ClusterMonitorConfig::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("maxTimeLeader", this->maxTimeLeader());
    printer.printAttribute("maxTimeMaster", this->maxTimeMaster());
    printer.printAttribute("maxTimeNode", this->maxTimeNode());
    printer.printAttribute("maxTimeFailover", this->maxTimeFailover());
    printer.printAttribute("thresholdLeader", this->thresholdLeader());
    printer.printAttribute("thresholdMaster", this->thresholdMaster());
    printer.printAttribute("thresholdNode", this->thresholdNode());
    printer.printAttribute("thresholdFailover", this->thresholdFailover());
    printer.end();
    return stream;
}

// -----------------------------------
// class DispatcherProcessorParameters
// -----------------------------------

// CONSTANTS

const char DispatcherProcessorParameters::CLASS_NAME[] =
    "DispatcherProcessorParameters";

const bdlat_AttributeInfo
    DispatcherProcessorParameters::ATTRIBUTE_INFO_ARRAY[] = {
        {ATTRIBUTE_ID_QUEUE_SIZE,
         "queueSize",
         sizeof("queueSize") - 1,
         "",
         bdlat_FormattingMode::e_DEC},
        {ATTRIBUTE_ID_QUEUE_SIZE_LOW_WATERMARK,
         "queueSizeLowWatermark",
         sizeof("queueSizeLowWatermark") - 1,
         "",
         bdlat_FormattingMode::e_DEC},
        {ATTRIBUTE_ID_QUEUE_SIZE_HIGH_WATERMARK,
         "queueSizeHighWatermark",
         sizeof("queueSizeHighWatermark") - 1,
         "",
         bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
DispatcherProcessorParameters::lookupAttributeInfo(const char* name,
                                                   int         nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DispatcherProcessorParameters::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
DispatcherProcessorParameters::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_QUEUE_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE];
    case ATTRIBUTE_ID_QUEUE_SIZE_LOW_WATERMARK:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE_LOW_WATERMARK];
    case ATTRIBUTE_ID_QUEUE_SIZE_HIGH_WATERMARK:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_QUEUE_SIZE_HIGH_WATERMARK];
    default: return 0;
    }
}

// CREATORS

DispatcherProcessorParameters::DispatcherProcessorParameters()
: d_queueSize()
, d_queueSizeLowWatermark()
, d_queueSizeHighWatermark()
{
}

// MANIPULATORS

void DispatcherProcessorParameters::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_queueSize);
    bdlat_ValueTypeFunctions::reset(&d_queueSizeLowWatermark);
    bdlat_ValueTypeFunctions::reset(&d_queueSizeHighWatermark);
}

// ACCESSORS

bsl::ostream& DispatcherProcessorParameters::print(bsl::ostream& stream,
                                                   int           level,
                                                   int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("queueSize", this->queueSize());
    printer.printAttribute("queueSizeLowWatermark",
                           this->queueSizeLowWatermark());
    printer.printAttribute("queueSizeHighWatermark",
                           this->queueSizeHighWatermark());
    printer.end();
    return stream;
}

// -------------------
// class ElectorConfig
// -------------------

// CONSTANTS

const char ElectorConfig::CLASS_NAME[] = "ElectorConfig";

const int ElectorConfig::DEFAULT_INITIALIZER_INITIAL_WAIT_TIMEOUT_MS = 8000;

const int ElectorConfig::DEFAULT_INITIALIZER_MAX_RANDOM_WAIT_TIMEOUT_MS = 3000;

const int ElectorConfig::DEFAULT_INITIALIZER_SCOUTING_RESULT_TIMEOUT_MS = 4000;

const int ElectorConfig::DEFAULT_INITIALIZER_ELECTION_RESULT_TIMEOUT_MS = 4000;

const int ElectorConfig::DEFAULT_INITIALIZER_HEARTBEAT_BROADCAST_PERIOD_MS =
    2000;

const int ElectorConfig::DEFAULT_INITIALIZER_HEARTBEAT_CHECK_PERIOD_MS = 1000;

const int ElectorConfig::DEFAULT_INITIALIZER_HEARTBEAT_MISS_COUNT = 10;

const int ElectorConfig::DEFAULT_INITIALIZER_QUORUM = 0;

const int ElectorConfig::DEFAULT_INITIALIZER_LEADER_SYNC_DELAY_MS = 80000;

const bdlat_AttributeInfo ElectorConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_INITIAL_WAIT_TIMEOUT_MS,
     "initialWaitTimeoutMs",
     sizeof("initialWaitTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_RANDOM_WAIT_TIMEOUT_MS,
     "maxRandomWaitTimeoutMs",
     sizeof("maxRandomWaitTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SCOUTING_RESULT_TIMEOUT_MS,
     "scoutingResultTimeoutMs",
     sizeof("scoutingResultTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_ELECTION_RESULT_TIMEOUT_MS,
     "electionResultTimeoutMs",
     sizeof("electionResultTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_HEARTBEAT_BROADCAST_PERIOD_MS,
     "heartbeatBroadcastPeriodMs",
     sizeof("heartbeatBroadcastPeriodMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_HEARTBEAT_CHECK_PERIOD_MS,
     "heartbeatCheckPeriodMs",
     sizeof("heartbeatCheckPeriodMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_HEARTBEAT_MISS_COUNT,
     "heartbeatMissCount",
     sizeof("heartbeatMissCount") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_QUORUM,
     "quorum",
     sizeof("quorum") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_LEADER_SYNC_DELAY_MS,
     "leaderSyncDelayMs",
     sizeof("leaderSyncDelayMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* ElectorConfig::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 9; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ElectorConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ElectorConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_INITIAL_WAIT_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_INITIAL_WAIT_TIMEOUT_MS];
    case ATTRIBUTE_ID_MAX_RANDOM_WAIT_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_MAX_RANDOM_WAIT_TIMEOUT_MS];
    case ATTRIBUTE_ID_SCOUTING_RESULT_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_SCOUTING_RESULT_TIMEOUT_MS];
    case ATTRIBUTE_ID_ELECTION_RESULT_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_ELECTION_RESULT_TIMEOUT_MS];
    case ATTRIBUTE_ID_HEARTBEAT_BROADCAST_PERIOD_MS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_HEARTBEAT_BROADCAST_PERIOD_MS];
    case ATTRIBUTE_ID_HEARTBEAT_CHECK_PERIOD_MS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_HEARTBEAT_CHECK_PERIOD_MS];
    case ATTRIBUTE_ID_HEARTBEAT_MISS_COUNT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_MISS_COUNT];
    case ATTRIBUTE_ID_QUORUM:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUORUM];
    case ATTRIBUTE_ID_LEADER_SYNC_DELAY_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LEADER_SYNC_DELAY_MS];
    default: return 0;
    }
}

// CREATORS

ElectorConfig::ElectorConfig()
: d_initialWaitTimeoutMs(DEFAULT_INITIALIZER_INITIAL_WAIT_TIMEOUT_MS)
, d_maxRandomWaitTimeoutMs(DEFAULT_INITIALIZER_MAX_RANDOM_WAIT_TIMEOUT_MS)
, d_scoutingResultTimeoutMs(DEFAULT_INITIALIZER_SCOUTING_RESULT_TIMEOUT_MS)
, d_electionResultTimeoutMs(DEFAULT_INITIALIZER_ELECTION_RESULT_TIMEOUT_MS)
, d_heartbeatBroadcastPeriodMs(
      DEFAULT_INITIALIZER_HEARTBEAT_BROADCAST_PERIOD_MS)
, d_heartbeatCheckPeriodMs(DEFAULT_INITIALIZER_HEARTBEAT_CHECK_PERIOD_MS)
, d_heartbeatMissCount(DEFAULT_INITIALIZER_HEARTBEAT_MISS_COUNT)
, d_quorum(DEFAULT_INITIALIZER_QUORUM)
, d_leaderSyncDelayMs(DEFAULT_INITIALIZER_LEADER_SYNC_DELAY_MS)
{
}

// MANIPULATORS

void ElectorConfig::reset()
{
    d_initialWaitTimeoutMs    = DEFAULT_INITIALIZER_INITIAL_WAIT_TIMEOUT_MS;
    d_maxRandomWaitTimeoutMs  = DEFAULT_INITIALIZER_MAX_RANDOM_WAIT_TIMEOUT_MS;
    d_scoutingResultTimeoutMs = DEFAULT_INITIALIZER_SCOUTING_RESULT_TIMEOUT_MS;
    d_electionResultTimeoutMs = DEFAULT_INITIALIZER_ELECTION_RESULT_TIMEOUT_MS;
    d_heartbeatBroadcastPeriodMs =
        DEFAULT_INITIALIZER_HEARTBEAT_BROADCAST_PERIOD_MS;
    d_heartbeatCheckPeriodMs = DEFAULT_INITIALIZER_HEARTBEAT_CHECK_PERIOD_MS;
    d_heartbeatMissCount     = DEFAULT_INITIALIZER_HEARTBEAT_MISS_COUNT;
    d_quorum                 = DEFAULT_INITIALIZER_QUORUM;
    d_leaderSyncDelayMs      = DEFAULT_INITIALIZER_LEADER_SYNC_DELAY_MS;
}

// ACCESSORS

bsl::ostream&
ElectorConfig::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("initialWaitTimeoutMs",
                           this->initialWaitTimeoutMs());
    printer.printAttribute("maxRandomWaitTimeoutMs",
                           this->maxRandomWaitTimeoutMs());
    printer.printAttribute("scoutingResultTimeoutMs",
                           this->scoutingResultTimeoutMs());
    printer.printAttribute("electionResultTimeoutMs",
                           this->electionResultTimeoutMs());
    printer.printAttribute("heartbeatBroadcastPeriodMs",
                           this->heartbeatBroadcastPeriodMs());
    printer.printAttribute("heartbeatCheckPeriodMs",
                           this->heartbeatCheckPeriodMs());
    printer.printAttribute("heartbeatMissCount", this->heartbeatMissCount());
    printer.printAttribute("quorum", this->quorum());
    printer.printAttribute("leaderSyncDelayMs", this->leaderSyncDelayMs());
    printer.end();
    return stream;
}

// ----------------
// class ExportMode
// ----------------

// CONSTANTS

const char ExportMode::CLASS_NAME[] = "ExportMode";

const bdlat_EnumeratorInfo ExportMode::ENUMERATOR_INFO_ARRAY[] = {
    {ExportMode::E_PUSH, "E_PUSH", sizeof("E_PUSH") - 1, ""},
    {ExportMode::E_PULL, "E_PULL", sizeof("E_PULL") - 1, ""}};

// CLASS METHODS

int ExportMode::fromInt(ExportMode::Value* result, int number)
{
    switch (number) {
    case ExportMode::E_PUSH:
    case ExportMode::E_PULL:
        *result = static_cast<ExportMode::Value>(number);
        return 0;
    default: return -1;
    }
}

int ExportMode::fromString(ExportMode::Value* result,
                           const char*        string,
                           int                stringLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            ExportMode::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<ExportMode::Value>(enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* ExportMode::toString(ExportMode::Value value)
{
    switch (value) {
    case E_PUSH: {
        return "E_PUSH";
    }
    case E_PULL: {
        return "E_PULL";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ---------------
// class Heartbeat
// ---------------

// CONSTANTS

const char Heartbeat::CLASS_NAME[] = "Heartbeat";

const int Heartbeat::DEFAULT_INITIALIZER_CLIENT = 0;

const int Heartbeat::DEFAULT_INITIALIZER_DOWNSTREAM_BROKER = 0;

const int Heartbeat::DEFAULT_INITIALIZER_UPSTREAM_BROKER = 0;

const int Heartbeat::DEFAULT_INITIALIZER_CLUSTER_PEER = 0;

const bdlat_AttributeInfo Heartbeat::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CLIENT,
     "client",
     sizeof("client") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_DOWNSTREAM_BROKER,
     "downstreamBroker",
     sizeof("downstreamBroker") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_UPSTREAM_BROKER,
     "upstreamBroker",
     sizeof("upstreamBroker") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CLUSTER_PEER,
     "clusterPeer",
     sizeof("clusterPeer") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo* Heartbeat::lookupAttributeInfo(const char* name,
                                                          int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Heartbeat::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Heartbeat::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CLIENT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLIENT];
    case ATTRIBUTE_ID_DOWNSTREAM_BROKER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DOWNSTREAM_BROKER];
    case ATTRIBUTE_ID_UPSTREAM_BROKER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_UPSTREAM_BROKER];
    case ATTRIBUTE_ID_CLUSTER_PEER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_PEER];
    default: return 0;
    }
}

// CREATORS

Heartbeat::Heartbeat()
: d_client(DEFAULT_INITIALIZER_CLIENT)
, d_downstreamBroker(DEFAULT_INITIALIZER_DOWNSTREAM_BROKER)
, d_upstreamBroker(DEFAULT_INITIALIZER_UPSTREAM_BROKER)
, d_clusterPeer(DEFAULT_INITIALIZER_CLUSTER_PEER)
{
}

// MANIPULATORS

void Heartbeat::reset()
{
    d_client           = DEFAULT_INITIALIZER_CLIENT;
    d_downstreamBroker = DEFAULT_INITIALIZER_DOWNSTREAM_BROKER;
    d_upstreamBroker   = DEFAULT_INITIALIZER_UPSTREAM_BROKER;
    d_clusterPeer      = DEFAULT_INITIALIZER_CLUSTER_PEER;
}

// ACCESSORS

bsl::ostream&
Heartbeat::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("client", this->client());
    printer.printAttribute("downstreamBroker", this->downstreamBroker());
    printer.printAttribute("upstreamBroker", this->upstreamBroker());
    printer.printAttribute("clusterPeer", this->clusterPeer());
    printer.end();
    return stream;
}

// -------------------------------
// class MasterAssignmentAlgorithm
// -------------------------------

// CONSTANTS

const char MasterAssignmentAlgorithm::CLASS_NAME[] =
    "MasterAssignmentAlgorithm";

const bdlat_EnumeratorInfo MasterAssignmentAlgorithm::ENUMERATOR_INFO_ARRAY[] =
    {{MasterAssignmentAlgorithm::E_LEADER_IS_MASTER_ALL,
      "E_LEADER_IS_MASTER_ALL",
      sizeof("E_LEADER_IS_MASTER_ALL") - 1,
      ""},
     {MasterAssignmentAlgorithm::E_LEAST_ASSIGNED,
      "E_LEAST_ASSIGNED",
      sizeof("E_LEAST_ASSIGNED") - 1,
      ""}};

// CLASS METHODS

int MasterAssignmentAlgorithm::fromInt(
    MasterAssignmentAlgorithm::Value* result,
    int                               number)
{
    switch (number) {
    case MasterAssignmentAlgorithm::E_LEADER_IS_MASTER_ALL:
    case MasterAssignmentAlgorithm::E_LEAST_ASSIGNED:
        *result = static_cast<MasterAssignmentAlgorithm::Value>(number);
        return 0;
    default: return -1;
    }
}

int MasterAssignmentAlgorithm::fromString(
    MasterAssignmentAlgorithm::Value* result,
    const char*                       string,
    int                               stringLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            MasterAssignmentAlgorithm::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<MasterAssignmentAlgorithm::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char*
MasterAssignmentAlgorithm::toString(MasterAssignmentAlgorithm::Value value)
{
    switch (value) {
    case E_LEADER_IS_MASTER_ALL: {
        return "E_LEADER_IS_MASTER_ALL";
    }
    case E_LEAST_ASSIGNED: {
        return "E_LEAST_ASSIGNED";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -------------------------
// class MessagePropertiesV2
// -------------------------

// CONSTANTS

const char MessagePropertiesV2::CLASS_NAME[] = "MessagePropertiesV2";

const bool MessagePropertiesV2::DEFAULT_INITIALIZER_ADVERTISE_V2_SUPPORT =
    true;

const int MessagePropertiesV2::DEFAULT_INITIALIZER_MIN_CPP_SDK_VERSION = 11207;

const int MessagePropertiesV2::DEFAULT_INITIALIZER_MIN_JAVA_SDK_VERSION = 10;

const bdlat_AttributeInfo MessagePropertiesV2::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ADVERTISE_V2_SUPPORT,
     "advertiseV2Support",
     sizeof("advertiseV2Support") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_MIN_CPP_SDK_VERSION,
     "minCppSdkVersion",
     sizeof("minCppSdkVersion") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MIN_JAVA_SDK_VERSION,
     "minJavaSdkVersion",
     sizeof("minJavaSdkVersion") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
MessagePropertiesV2::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            MessagePropertiesV2::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* MessagePropertiesV2::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ADVERTISE_V2_SUPPORT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADVERTISE_V2_SUPPORT];
    case ATTRIBUTE_ID_MIN_CPP_SDK_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_CPP_SDK_VERSION];
    case ATTRIBUTE_ID_MIN_JAVA_SDK_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MIN_JAVA_SDK_VERSION];
    default: return 0;
    }
}

// CREATORS

MessagePropertiesV2::MessagePropertiesV2()
: d_minCppSdkVersion(DEFAULT_INITIALIZER_MIN_CPP_SDK_VERSION)
, d_minJavaSdkVersion(DEFAULT_INITIALIZER_MIN_JAVA_SDK_VERSION)
, d_advertiseV2Support(DEFAULT_INITIALIZER_ADVERTISE_V2_SUPPORT)
{
}

// MANIPULATORS

void MessagePropertiesV2::reset()
{
    d_advertiseV2Support = DEFAULT_INITIALIZER_ADVERTISE_V2_SUPPORT;
    d_minCppSdkVersion   = DEFAULT_INITIALIZER_MIN_CPP_SDK_VERSION;
    d_minJavaSdkVersion  = DEFAULT_INITIALIZER_MIN_JAVA_SDK_VERSION;
}

// ACCESSORS

bsl::ostream& MessagePropertiesV2::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("advertiseV2Support", this->advertiseV2Support());
    printer.printAttribute("minCppSdkVersion", this->minCppSdkVersion());
    printer.printAttribute("minJavaSdkVersion", this->minJavaSdkVersion());
    printer.end();
    return stream;
}

// ---------------------------
// class MessageThrottleConfig
// ---------------------------

// CONSTANTS

const char MessageThrottleConfig::CLASS_NAME[] = "MessageThrottleConfig";

const unsigned int MessageThrottleConfig::DEFAULT_INITIALIZER_LOW_THRESHOLD =
    2;

const unsigned int MessageThrottleConfig::DEFAULT_INITIALIZER_HIGH_THRESHOLD =
    4;

const unsigned int MessageThrottleConfig::DEFAULT_INITIALIZER_LOW_INTERVAL =
    1000;

const unsigned int MessageThrottleConfig::DEFAULT_INITIALIZER_HIGH_INTERVAL =
    3000;

const bdlat_AttributeInfo MessageThrottleConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_LOW_THRESHOLD,
     "lowThreshold",
     sizeof("lowThreshold") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_HIGH_THRESHOLD,
     "highThreshold",
     sizeof("highThreshold") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_LOW_INTERVAL,
     "lowInterval",
     sizeof("lowInterval") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_HIGH_INTERVAL,
     "highInterval",
     sizeof("highInterval") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
MessageThrottleConfig::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            MessageThrottleConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* MessageThrottleConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_LOW_THRESHOLD:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_THRESHOLD];
    case ATTRIBUTE_ID_HIGH_THRESHOLD:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_THRESHOLD];
    case ATTRIBUTE_ID_LOW_INTERVAL:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_INTERVAL];
    case ATTRIBUTE_ID_HIGH_INTERVAL:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_INTERVAL];
    default: return 0;
    }
}

// CREATORS

MessageThrottleConfig::MessageThrottleConfig()
: d_lowThreshold(DEFAULT_INITIALIZER_LOW_THRESHOLD)
, d_highThreshold(DEFAULT_INITIALIZER_HIGH_THRESHOLD)
, d_lowInterval(DEFAULT_INITIALIZER_LOW_INTERVAL)
, d_highInterval(DEFAULT_INITIALIZER_HIGH_INTERVAL)
{
}

// MANIPULATORS

void MessageThrottleConfig::reset()
{
    d_lowThreshold  = DEFAULT_INITIALIZER_LOW_THRESHOLD;
    d_highThreshold = DEFAULT_INITIALIZER_HIGH_THRESHOLD;
    d_lowInterval   = DEFAULT_INITIALIZER_LOW_INTERVAL;
    d_highInterval  = DEFAULT_INITIALIZER_HIGH_INTERVAL;
}

// ACCESSORS

bsl::ostream& MessageThrottleConfig::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("lowThreshold", this->lowThreshold());
    printer.printAttribute("highThreshold", this->highThreshold());
    printer.printAttribute("lowInterval", this->lowInterval());
    printer.printAttribute("highInterval", this->highInterval());
    printer.end();
    return stream;
}

// -------------
// class Plugins
// -------------

// CONSTANTS

const char Plugins::CLASS_NAME[] = "Plugins";

const bdlat_AttributeInfo Plugins::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_LIBRARIES,
     "libraries",
     sizeof("libraries") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_ENABLED,
     "enabled",
     sizeof("enabled") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* Plugins::lookupAttributeInfo(const char* name,
                                                        int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Plugins::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Plugins::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_LIBRARIES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LIBRARIES];
    case ATTRIBUTE_ID_ENABLED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENABLED];
    default: return 0;
    }
}

// CREATORS

Plugins::Plugins(bslma::Allocator* basicAllocator)
: d_libraries(basicAllocator)
, d_enabled(basicAllocator)
{
}

Plugins::Plugins(const Plugins& original, bslma::Allocator* basicAllocator)
: d_libraries(original.d_libraries, basicAllocator)
, d_enabled(original.d_enabled, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Plugins::Plugins(Plugins&& original) noexcept
: d_libraries(bsl::move(original.d_libraries)),
  d_enabled(bsl::move(original.d_enabled))
{
}

Plugins::Plugins(Plugins&& original, bslma::Allocator* basicAllocator)
: d_libraries(bsl::move(original.d_libraries), basicAllocator)
, d_enabled(bsl::move(original.d_enabled), basicAllocator)
{
}
#endif

Plugins::~Plugins()
{
}

// MANIPULATORS

Plugins& Plugins::operator=(const Plugins& rhs)
{
    if (this != &rhs) {
        d_libraries = rhs.d_libraries;
        d_enabled   = rhs.d_enabled;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Plugins& Plugins::operator=(Plugins&& rhs)
{
    if (this != &rhs) {
        d_libraries = bsl::move(rhs.d_libraries);
        d_enabled   = bsl::move(rhs.d_enabled);
    }

    return *this;
}
#endif

void Plugins::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_libraries);
    bdlat_ValueTypeFunctions::reset(&d_enabled);
}

// ACCESSORS

bsl::ostream&
Plugins::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("libraries", this->libraries());
    printer.printAttribute("enabled", this->enabled());
    printer.end();
    return stream;
}

// ---------------------------
// class QueueOperationsConfig
// ---------------------------

// CONSTANTS

const char QueueOperationsConfig::CLASS_NAME[] = "QueueOperationsConfig";

const int QueueOperationsConfig::DEFAULT_INITIALIZER_OPEN_TIMEOUT_MS = 300000;

const int QueueOperationsConfig::DEFAULT_INITIALIZER_CONFIGURE_TIMEOUT_MS =
    300000;

const int QueueOperationsConfig::DEFAULT_INITIALIZER_CLOSE_TIMEOUT_MS = 300000;

const int QueueOperationsConfig::DEFAULT_INITIALIZER_REOPEN_TIMEOUT_MS =
    43200000;

const int QueueOperationsConfig::DEFAULT_INITIALIZER_REOPEN_RETRY_INTERVAL_MS =
    5000;

const int QueueOperationsConfig::DEFAULT_INITIALIZER_REOPEN_MAX_ATTEMPTS = 10;

const int QueueOperationsConfig::DEFAULT_INITIALIZER_ASSIGNMENT_TIMEOUT_MS =
    15000;

const int QueueOperationsConfig::DEFAULT_INITIALIZER_KEEPALIVE_DURATION_MS =
    1800000;

const int
    QueueOperationsConfig::DEFAULT_INITIALIZER_CONSUMPTION_MONITOR_PERIOD_MS =
        30000;

const int QueueOperationsConfig::DEFAULT_INITIALIZER_STOP_TIMEOUT_MS = 10000;

const int QueueOperationsConfig::DEFAULT_INITIALIZER_SHUTDOWN_TIMEOUT_MS =
    20000;

const int QueueOperationsConfig::DEFAULT_INITIALIZER_ACK_WINDOW_SIZE = 500;

const bdlat_AttributeInfo QueueOperationsConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_OPEN_TIMEOUT_MS,
     "openTimeoutMs",
     sizeof("openTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CONFIGURE_TIMEOUT_MS,
     "configureTimeoutMs",
     sizeof("configureTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CLOSE_TIMEOUT_MS,
     "closeTimeoutMs",
     sizeof("closeTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_REOPEN_TIMEOUT_MS,
     "reopenTimeoutMs",
     sizeof("reopenTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_REOPEN_RETRY_INTERVAL_MS,
     "reopenRetryIntervalMs",
     sizeof("reopenRetryIntervalMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_REOPEN_MAX_ATTEMPTS,
     "reopenMaxAttempts",
     sizeof("reopenMaxAttempts") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_ASSIGNMENT_TIMEOUT_MS,
     "assignmentTimeoutMs",
     sizeof("assignmentTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_KEEPALIVE_DURATION_MS,
     "keepaliveDurationMs",
     sizeof("keepaliveDurationMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CONSUMPTION_MONITOR_PERIOD_MS,
     "consumptionMonitorPeriodMs",
     sizeof("consumptionMonitorPeriodMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STOP_TIMEOUT_MS,
     "stopTimeoutMs",
     sizeof("stopTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SHUTDOWN_TIMEOUT_MS,
     "shutdownTimeoutMs",
     sizeof("shutdownTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_ACK_WINDOW_SIZE,
     "ackWindowSize",
     sizeof("ackWindowSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
QueueOperationsConfig::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 12; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            QueueOperationsConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* QueueOperationsConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_OPEN_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_OPEN_TIMEOUT_MS];
    case ATTRIBUTE_ID_CONFIGURE_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURE_TIMEOUT_MS];
    case ATTRIBUTE_ID_CLOSE_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLOSE_TIMEOUT_MS];
    case ATTRIBUTE_ID_REOPEN_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_TIMEOUT_MS];
    case ATTRIBUTE_ID_REOPEN_RETRY_INTERVAL_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_RETRY_INTERVAL_MS];
    case ATTRIBUTE_ID_REOPEN_MAX_ATTEMPTS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_REOPEN_MAX_ATTEMPTS];
    case ATTRIBUTE_ID_ASSIGNMENT_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASSIGNMENT_TIMEOUT_MS];
    case ATTRIBUTE_ID_KEEPALIVE_DURATION_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEEPALIVE_DURATION_MS];
    case ATTRIBUTE_ID_CONSUMPTION_MONITOR_PERIOD_MS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_CONSUMPTION_MONITOR_PERIOD_MS];
    case ATTRIBUTE_ID_STOP_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STOP_TIMEOUT_MS];
    case ATTRIBUTE_ID_SHUTDOWN_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SHUTDOWN_TIMEOUT_MS];
    case ATTRIBUTE_ID_ACK_WINDOW_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ACK_WINDOW_SIZE];
    default: return 0;
    }
}

// CREATORS

QueueOperationsConfig::QueueOperationsConfig()
: d_openTimeoutMs(DEFAULT_INITIALIZER_OPEN_TIMEOUT_MS)
, d_configureTimeoutMs(DEFAULT_INITIALIZER_CONFIGURE_TIMEOUT_MS)
, d_closeTimeoutMs(DEFAULT_INITIALIZER_CLOSE_TIMEOUT_MS)
, d_reopenTimeoutMs(DEFAULT_INITIALIZER_REOPEN_TIMEOUT_MS)
, d_reopenRetryIntervalMs(DEFAULT_INITIALIZER_REOPEN_RETRY_INTERVAL_MS)
, d_reopenMaxAttempts(DEFAULT_INITIALIZER_REOPEN_MAX_ATTEMPTS)
, d_assignmentTimeoutMs(DEFAULT_INITIALIZER_ASSIGNMENT_TIMEOUT_MS)
, d_keepaliveDurationMs(DEFAULT_INITIALIZER_KEEPALIVE_DURATION_MS)
, d_consumptionMonitorPeriodMs(
      DEFAULT_INITIALIZER_CONSUMPTION_MONITOR_PERIOD_MS)
, d_stopTimeoutMs(DEFAULT_INITIALIZER_STOP_TIMEOUT_MS)
, d_shutdownTimeoutMs(DEFAULT_INITIALIZER_SHUTDOWN_TIMEOUT_MS)
, d_ackWindowSize(DEFAULT_INITIALIZER_ACK_WINDOW_SIZE)
{
}

// MANIPULATORS

void QueueOperationsConfig::reset()
{
    d_openTimeoutMs         = DEFAULT_INITIALIZER_OPEN_TIMEOUT_MS;
    d_configureTimeoutMs    = DEFAULT_INITIALIZER_CONFIGURE_TIMEOUT_MS;
    d_closeTimeoutMs        = DEFAULT_INITIALIZER_CLOSE_TIMEOUT_MS;
    d_reopenTimeoutMs       = DEFAULT_INITIALIZER_REOPEN_TIMEOUT_MS;
    d_reopenRetryIntervalMs = DEFAULT_INITIALIZER_REOPEN_RETRY_INTERVAL_MS;
    d_reopenMaxAttempts     = DEFAULT_INITIALIZER_REOPEN_MAX_ATTEMPTS;
    d_assignmentTimeoutMs   = DEFAULT_INITIALIZER_ASSIGNMENT_TIMEOUT_MS;
    d_keepaliveDurationMs   = DEFAULT_INITIALIZER_KEEPALIVE_DURATION_MS;
    d_consumptionMonitorPeriodMs =
        DEFAULT_INITIALIZER_CONSUMPTION_MONITOR_PERIOD_MS;
    d_stopTimeoutMs     = DEFAULT_INITIALIZER_STOP_TIMEOUT_MS;
    d_shutdownTimeoutMs = DEFAULT_INITIALIZER_SHUTDOWN_TIMEOUT_MS;
    d_ackWindowSize     = DEFAULT_INITIALIZER_ACK_WINDOW_SIZE;
}

// ACCESSORS

bsl::ostream& QueueOperationsConfig::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("openTimeoutMs", this->openTimeoutMs());
    printer.printAttribute("configureTimeoutMs", this->configureTimeoutMs());
    printer.printAttribute("closeTimeoutMs", this->closeTimeoutMs());
    printer.printAttribute("reopenTimeoutMs", this->reopenTimeoutMs());
    printer.printAttribute("reopenRetryIntervalMs",
                           this->reopenRetryIntervalMs());
    printer.printAttribute("reopenMaxAttempts", this->reopenMaxAttempts());
    printer.printAttribute("assignmentTimeoutMs", this->assignmentTimeoutMs());
    printer.printAttribute("keepaliveDurationMs", this->keepaliveDurationMs());
    printer.printAttribute("consumptionMonitorPeriodMs",
                           this->consumptionMonitorPeriodMs());
    printer.printAttribute("stopTimeoutMs", this->stopTimeoutMs());
    printer.printAttribute("shutdownTimeoutMs", this->shutdownTimeoutMs());
    printer.printAttribute("ackWindowSize", this->ackWindowSize());
    printer.end();
    return stream;
}

// --------------------
// class ResolvedDomain
// --------------------

// CONSTANTS

const char ResolvedDomain::CLASS_NAME[] = "ResolvedDomain";

const bdlat_AttributeInfo ResolvedDomain::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_RESOLVED_NAME,
     "resolvedName",
     sizeof("resolvedName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CLUSTER_NAME,
     "clusterName",
     sizeof("clusterName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ResolvedDomain::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ResolvedDomain::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ResolvedDomain::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_RESOLVED_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_RESOLVED_NAME];
    case ATTRIBUTE_ID_CLUSTER_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_NAME];
    default: return 0;
    }
}

// CREATORS

ResolvedDomain::ResolvedDomain(bslma::Allocator* basicAllocator)
: d_resolvedName(basicAllocator)
, d_clusterName(basicAllocator)
{
}

ResolvedDomain::ResolvedDomain(const ResolvedDomain& original,
                               bslma::Allocator*     basicAllocator)
: d_resolvedName(original.d_resolvedName, basicAllocator)
, d_clusterName(original.d_clusterName, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ResolvedDomain::ResolvedDomain(ResolvedDomain&& original) noexcept
: d_resolvedName(bsl::move(original.d_resolvedName)),
  d_clusterName(bsl::move(original.d_clusterName))
{
}

ResolvedDomain::ResolvedDomain(ResolvedDomain&&  original,
                               bslma::Allocator* basicAllocator)
: d_resolvedName(bsl::move(original.d_resolvedName), basicAllocator)
, d_clusterName(bsl::move(original.d_clusterName), basicAllocator)
{
}
#endif

ResolvedDomain::~ResolvedDomain()
{
}

// MANIPULATORS

ResolvedDomain& ResolvedDomain::operator=(const ResolvedDomain& rhs)
{
    if (this != &rhs) {
        d_resolvedName = rhs.d_resolvedName;
        d_clusterName  = rhs.d_clusterName;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ResolvedDomain& ResolvedDomain::operator=(ResolvedDomain&& rhs)
{
    if (this != &rhs) {
        d_resolvedName = bsl::move(rhs.d_resolvedName);
        d_clusterName  = bsl::move(rhs.d_clusterName);
    }

    return *this;
}
#endif

void ResolvedDomain::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_resolvedName);
    bdlat_ValueTypeFunctions::reset(&d_clusterName);
}

// ACCESSORS

bsl::ostream& ResolvedDomain::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("resolvedName", this->resolvedName());
    printer.printAttribute("clusterName", this->clusterName());
    printer.end();
    return stream;
}

// ------------------------
// class StatsPrinterConfig
// ------------------------

// CONSTANTS

const char StatsPrinterConfig::CLASS_NAME[] = "StatsPrinterConfig";

const int StatsPrinterConfig::DEFAULT_INITIALIZER_PRINT_INTERVAL = 60;

const int StatsPrinterConfig::DEFAULT_INITIALIZER_ROTATE_BYTES = 268435456;

const int StatsPrinterConfig::DEFAULT_INITIALIZER_ROTATE_DAYS = 1;

const bdlat_AttributeInfo StatsPrinterConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PRINT_INTERVAL,
     "printInterval",
     sizeof("printInterval") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_FILE,
     "file",
     sizeof("file") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_MAX_AGE_DAYS,
     "maxAgeDays",
     sizeof("maxAgeDays") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_ROTATE_BYTES,
     "rotateBytes",
     sizeof("rotateBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_ROTATE_DAYS,
     "rotateDays",
     sizeof("rotateDays") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
StatsPrinterConfig::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StatsPrinterConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StatsPrinterConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PRINT_INTERVAL:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRINT_INTERVAL];
    case ATTRIBUTE_ID_FILE: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE];
    case ATTRIBUTE_ID_MAX_AGE_DAYS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_AGE_DAYS];
    case ATTRIBUTE_ID_ROTATE_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATE_BYTES];
    case ATTRIBUTE_ID_ROTATE_DAYS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATE_DAYS];
    default: return 0;
    }
}

// CREATORS

StatsPrinterConfig::StatsPrinterConfig(bslma::Allocator* basicAllocator)
: d_file(basicAllocator)
, d_printInterval(DEFAULT_INITIALIZER_PRINT_INTERVAL)
, d_maxAgeDays()
, d_rotateBytes(DEFAULT_INITIALIZER_ROTATE_BYTES)
, d_rotateDays(DEFAULT_INITIALIZER_ROTATE_DAYS)
{
}

StatsPrinterConfig::StatsPrinterConfig(const StatsPrinterConfig& original,
                                       bslma::Allocator* basicAllocator)
: d_file(original.d_file, basicAllocator)
, d_printInterval(original.d_printInterval)
, d_maxAgeDays(original.d_maxAgeDays)
, d_rotateBytes(original.d_rotateBytes)
, d_rotateDays(original.d_rotateDays)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatsPrinterConfig::StatsPrinterConfig(StatsPrinterConfig&& original) noexcept
: d_file(bsl::move(original.d_file)),
  d_printInterval(bsl::move(original.d_printInterval)),
  d_maxAgeDays(bsl::move(original.d_maxAgeDays)),
  d_rotateBytes(bsl::move(original.d_rotateBytes)),
  d_rotateDays(bsl::move(original.d_rotateDays))
{
}

StatsPrinterConfig::StatsPrinterConfig(StatsPrinterConfig&& original,
                                       bslma::Allocator*    basicAllocator)
: d_file(bsl::move(original.d_file), basicAllocator)
, d_printInterval(bsl::move(original.d_printInterval))
, d_maxAgeDays(bsl::move(original.d_maxAgeDays))
, d_rotateBytes(bsl::move(original.d_rotateBytes))
, d_rotateDays(bsl::move(original.d_rotateDays))
{
}
#endif

StatsPrinterConfig::~StatsPrinterConfig()
{
}

// MANIPULATORS

StatsPrinterConfig&
StatsPrinterConfig::operator=(const StatsPrinterConfig& rhs)
{
    if (this != &rhs) {
        d_printInterval = rhs.d_printInterval;
        d_file          = rhs.d_file;
        d_maxAgeDays    = rhs.d_maxAgeDays;
        d_rotateBytes   = rhs.d_rotateBytes;
        d_rotateDays    = rhs.d_rotateDays;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatsPrinterConfig& StatsPrinterConfig::operator=(StatsPrinterConfig&& rhs)
{
    if (this != &rhs) {
        d_printInterval = bsl::move(rhs.d_printInterval);
        d_file          = bsl::move(rhs.d_file);
        d_maxAgeDays    = bsl::move(rhs.d_maxAgeDays);
        d_rotateBytes   = bsl::move(rhs.d_rotateBytes);
        d_rotateDays    = bsl::move(rhs.d_rotateDays);
    }

    return *this;
}
#endif

void StatsPrinterConfig::reset()
{
    d_printInterval = DEFAULT_INITIALIZER_PRINT_INTERVAL;
    bdlat_ValueTypeFunctions::reset(&d_file);
    bdlat_ValueTypeFunctions::reset(&d_maxAgeDays);
    d_rotateBytes = DEFAULT_INITIALIZER_ROTATE_BYTES;
    d_rotateDays  = DEFAULT_INITIALIZER_ROTATE_DAYS;
}

// ACCESSORS

bsl::ostream& StatsPrinterConfig::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("printInterval", this->printInterval());
    printer.printAttribute("file", this->file());
    printer.printAttribute("maxAgeDays", this->maxAgeDays());
    printer.printAttribute("rotateBytes", this->rotateBytes());
    printer.printAttribute("rotateDays", this->rotateDays());
    printer.end();
    return stream;
}

// -----------------------
// class StorageSyncConfig
// -----------------------

// CONSTANTS

const char StorageSyncConfig::CLASS_NAME[] = "StorageSyncConfig";

const int
    StorageSyncConfig::DEFAULT_INITIALIZER_STARTUP_RECOVERY_MAX_DURATION_MS =
        1200000;

const int StorageSyncConfig::DEFAULT_INITIALIZER_MAX_ATTEMPTS_STORAGE_SYNC = 3;

const int StorageSyncConfig::DEFAULT_INITIALIZER_STORAGE_SYNC_REQ_TIMEOUT_MS =
    300000;

const int StorageSyncConfig::DEFAULT_INITIALIZER_MASTER_SYNC_MAX_DURATION_MS =
    600000;

const int StorageSyncConfig::
    DEFAULT_INITIALIZER_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS = 120000;

const int
    StorageSyncConfig::DEFAULT_INITIALIZER_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS =
        120000;

const int StorageSyncConfig::DEFAULT_INITIALIZER_STARTUP_WAIT_DURATION_MS =
    60000;

const int StorageSyncConfig::DEFAULT_INITIALIZER_FILE_CHUNK_SIZE = 4194304;

const int StorageSyncConfig::DEFAULT_INITIALIZER_PARTITION_SYNC_EVENT_SIZE =
    4194304;

const bdlat_AttributeInfo StorageSyncConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_STARTUP_RECOVERY_MAX_DURATION_MS,
     "startupRecoveryMaxDurationMs",
     sizeof("startupRecoveryMaxDurationMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_ATTEMPTS_STORAGE_SYNC,
     "maxAttemptsStorageSync",
     sizeof("maxAttemptsStorageSync") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STORAGE_SYNC_REQ_TIMEOUT_MS,
     "storageSyncReqTimeoutMs",
     sizeof("storageSyncReqTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MASTER_SYNC_MAX_DURATION_MS,
     "masterSyncMaxDurationMs",
     sizeof("masterSyncMaxDurationMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS,
     "partitionSyncStateReqTimeoutMs",
     sizeof("partitionSyncStateReqTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS,
     "partitionSyncDataReqTimeoutMs",
     sizeof("partitionSyncDataReqTimeoutMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_STARTUP_WAIT_DURATION_MS,
     "startupWaitDurationMs",
     sizeof("startupWaitDurationMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_FILE_CHUNK_SIZE,
     "fileChunkSize",
     sizeof("fileChunkSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PARTITION_SYNC_EVENT_SIZE,
     "partitionSyncEventSize",
     sizeof("partitionSyncEventSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
StorageSyncConfig::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 9; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StorageSyncConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StorageSyncConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_STARTUP_RECOVERY_MAX_DURATION_MS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_STARTUP_RECOVERY_MAX_DURATION_MS];
    case ATTRIBUTE_ID_MAX_ATTEMPTS_STORAGE_SYNC:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_MAX_ATTEMPTS_STORAGE_SYNC];
    case ATTRIBUTE_ID_STORAGE_SYNC_REQ_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_STORAGE_SYNC_REQ_TIMEOUT_MS];
    case ATTRIBUTE_ID_MASTER_SYNC_MAX_DURATION_MS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_MASTER_SYNC_MAX_DURATION_MS];
    case ATTRIBUTE_ID_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS];
    case ATTRIBUTE_ID_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS];
    case ATTRIBUTE_ID_STARTUP_WAIT_DURATION_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STARTUP_WAIT_DURATION_MS];
    case ATTRIBUTE_ID_FILE_CHUNK_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_CHUNK_SIZE];
    case ATTRIBUTE_ID_PARTITION_SYNC_EVENT_SIZE:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_PARTITION_SYNC_EVENT_SIZE];
    default: return 0;
    }
}

// CREATORS

StorageSyncConfig::StorageSyncConfig()
: d_startupRecoveryMaxDurationMs(
      DEFAULT_INITIALIZER_STARTUP_RECOVERY_MAX_DURATION_MS)
, d_maxAttemptsStorageSync(DEFAULT_INITIALIZER_MAX_ATTEMPTS_STORAGE_SYNC)
, d_storageSyncReqTimeoutMs(DEFAULT_INITIALIZER_STORAGE_SYNC_REQ_TIMEOUT_MS)
, d_masterSyncMaxDurationMs(DEFAULT_INITIALIZER_MASTER_SYNC_MAX_DURATION_MS)
, d_partitionSyncStateReqTimeoutMs(
      DEFAULT_INITIALIZER_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS)
, d_partitionSyncDataReqTimeoutMs(
      DEFAULT_INITIALIZER_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS)
, d_startupWaitDurationMs(DEFAULT_INITIALIZER_STARTUP_WAIT_DURATION_MS)
, d_fileChunkSize(DEFAULT_INITIALIZER_FILE_CHUNK_SIZE)
, d_partitionSyncEventSize(DEFAULT_INITIALIZER_PARTITION_SYNC_EVENT_SIZE)
{
}

// MANIPULATORS

void StorageSyncConfig::reset()
{
    d_startupRecoveryMaxDurationMs =
        DEFAULT_INITIALIZER_STARTUP_RECOVERY_MAX_DURATION_MS;
    d_maxAttemptsStorageSync = DEFAULT_INITIALIZER_MAX_ATTEMPTS_STORAGE_SYNC;
    d_storageSyncReqTimeoutMs =
        DEFAULT_INITIALIZER_STORAGE_SYNC_REQ_TIMEOUT_MS;
    d_masterSyncMaxDurationMs =
        DEFAULT_INITIALIZER_MASTER_SYNC_MAX_DURATION_MS;
    d_partitionSyncStateReqTimeoutMs =
        DEFAULT_INITIALIZER_PARTITION_SYNC_STATE_REQ_TIMEOUT_MS;
    d_partitionSyncDataReqTimeoutMs =
        DEFAULT_INITIALIZER_PARTITION_SYNC_DATA_REQ_TIMEOUT_MS;
    d_startupWaitDurationMs  = DEFAULT_INITIALIZER_STARTUP_WAIT_DURATION_MS;
    d_fileChunkSize          = DEFAULT_INITIALIZER_FILE_CHUNK_SIZE;
    d_partitionSyncEventSize = DEFAULT_INITIALIZER_PARTITION_SYNC_EVENT_SIZE;
}

// ACCESSORS

bsl::ostream& StorageSyncConfig::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("startupRecoveryMaxDurationMs",
                           this->startupRecoveryMaxDurationMs());
    printer.printAttribute("maxAttemptsStorageSync",
                           this->maxAttemptsStorageSync());
    printer.printAttribute("storageSyncReqTimeoutMs",
                           this->storageSyncReqTimeoutMs());
    printer.printAttribute("masterSyncMaxDurationMs",
                           this->masterSyncMaxDurationMs());
    printer.printAttribute("partitionSyncStateReqTimeoutMs",
                           this->partitionSyncStateReqTimeoutMs());
    printer.printAttribute("partitionSyncDataReqTimeoutMs",
                           this->partitionSyncDataReqTimeoutMs());
    printer.printAttribute("startupWaitDurationMs",
                           this->startupWaitDurationMs());
    printer.printAttribute("fileChunkSize", this->fileChunkSize());
    printer.printAttribute("partitionSyncEventSize",
                           this->partitionSyncEventSize());
    printer.end();
    return stream;
}

// ------------------
// class SyslogConfig
// ------------------

// CONSTANTS

const char SyslogConfig::CLASS_NAME[] = "SyslogConfig";

const bool SyslogConfig::DEFAULT_INITIALIZER_ENABLED = false;

const bdlat_AttributeInfo SyslogConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ENABLED,
     "enabled",
     sizeof("enabled") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_APP_NAME,
     "appName",
     sizeof("appName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_LOG_FORMAT,
     "logFormat",
     sizeof("logFormat") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_VERBOSITY,
     "verbosity",
     sizeof("verbosity") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* SyslogConfig::lookupAttributeInfo(const char* name,
                                                             int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            SyslogConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* SyslogConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ENABLED:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENABLED];
    case ATTRIBUTE_ID_APP_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_NAME];
    case ATTRIBUTE_ID_LOG_FORMAT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_FORMAT];
    case ATTRIBUTE_ID_VERBOSITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERBOSITY];
    default: return 0;
    }
}

// CREATORS

SyslogConfig::SyslogConfig(bslma::Allocator* basicAllocator)
: d_appName(basicAllocator)
, d_logFormat(basicAllocator)
, d_verbosity(basicAllocator)
, d_enabled(DEFAULT_INITIALIZER_ENABLED)
{
}

SyslogConfig::SyslogConfig(const SyslogConfig& original,
                           bslma::Allocator*   basicAllocator)
: d_appName(original.d_appName, basicAllocator)
, d_logFormat(original.d_logFormat, basicAllocator)
, d_verbosity(original.d_verbosity, basicAllocator)
, d_enabled(original.d_enabled)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SyslogConfig::SyslogConfig(SyslogConfig&& original) noexcept
: d_appName(bsl::move(original.d_appName)),
  d_logFormat(bsl::move(original.d_logFormat)),
  d_verbosity(bsl::move(original.d_verbosity)),
  d_enabled(bsl::move(original.d_enabled))
{
}

SyslogConfig::SyslogConfig(SyslogConfig&&    original,
                           bslma::Allocator* basicAllocator)
: d_appName(bsl::move(original.d_appName), basicAllocator)
, d_logFormat(bsl::move(original.d_logFormat), basicAllocator)
, d_verbosity(bsl::move(original.d_verbosity), basicAllocator)
, d_enabled(bsl::move(original.d_enabled))
{
}
#endif

SyslogConfig::~SyslogConfig()
{
}

// MANIPULATORS

SyslogConfig& SyslogConfig::operator=(const SyslogConfig& rhs)
{
    if (this != &rhs) {
        d_enabled   = rhs.d_enabled;
        d_appName   = rhs.d_appName;
        d_logFormat = rhs.d_logFormat;
        d_verbosity = rhs.d_verbosity;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
SyslogConfig& SyslogConfig::operator=(SyslogConfig&& rhs)
{
    if (this != &rhs) {
        d_enabled   = bsl::move(rhs.d_enabled);
        d_appName   = bsl::move(rhs.d_appName);
        d_logFormat = bsl::move(rhs.d_logFormat);
        d_verbosity = bsl::move(rhs.d_verbosity);
    }

    return *this;
}
#endif

void SyslogConfig::reset()
{
    d_enabled = DEFAULT_INITIALIZER_ENABLED;
    bdlat_ValueTypeFunctions::reset(&d_appName);
    bdlat_ValueTypeFunctions::reset(&d_logFormat);
    bdlat_ValueTypeFunctions::reset(&d_verbosity);
}

// ACCESSORS

bsl::ostream&
SyslogConfig::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("enabled", this->enabled());
    printer.printAttribute("appName", this->appName());
    printer.printAttribute("logFormat", this->logFormat());
    printer.printAttribute("verbosity", this->verbosity());
    printer.end();
    return stream;
}

// ------------------------------
// class TcpClusterNodeConnection
// ------------------------------

// CONSTANTS

const char TcpClusterNodeConnection::CLASS_NAME[] = "TcpClusterNodeConnection";

const bdlat_AttributeInfo TcpClusterNodeConnection::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ENDPOINT,
     "endpoint",
     sizeof("endpoint") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
TcpClusterNodeConnection::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            TcpClusterNodeConnection::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
TcpClusterNodeConnection::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ENDPOINT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ENDPOINT];
    default: return 0;
    }
}

// CREATORS

TcpClusterNodeConnection::TcpClusterNodeConnection(
    bslma::Allocator* basicAllocator)
: d_endpoint(basicAllocator)
{
}

TcpClusterNodeConnection::TcpClusterNodeConnection(
    const TcpClusterNodeConnection& original,
    bslma::Allocator*               basicAllocator)
: d_endpoint(original.d_endpoint, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TcpClusterNodeConnection::TcpClusterNodeConnection(
    TcpClusterNodeConnection&& original) noexcept
: d_endpoint(bsl::move(original.d_endpoint))
{
}

TcpClusterNodeConnection::TcpClusterNodeConnection(
    TcpClusterNodeConnection&& original,
    bslma::Allocator*          basicAllocator)
: d_endpoint(bsl::move(original.d_endpoint), basicAllocator)
{
}
#endif

TcpClusterNodeConnection::~TcpClusterNodeConnection()
{
}

// MANIPULATORS

TcpClusterNodeConnection&
TcpClusterNodeConnection::operator=(const TcpClusterNodeConnection& rhs)
{
    if (this != &rhs) {
        d_endpoint = rhs.d_endpoint;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TcpClusterNodeConnection&
TcpClusterNodeConnection::operator=(TcpClusterNodeConnection&& rhs)
{
    if (this != &rhs) {
        d_endpoint = bsl::move(rhs.d_endpoint);
    }

    return *this;
}
#endif

void TcpClusterNodeConnection::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_endpoint);
}

// ACCESSORS

bsl::ostream& TcpClusterNodeConnection::print(bsl::ostream& stream,
                                              int           level,
                                              int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("endpoint", this->endpoint());
    printer.end();
    return stream;
}

// ------------------------
// class TcpInterfaceConfig
// ------------------------

// CONSTANTS

const char TcpInterfaceConfig::CLASS_NAME[] = "TcpInterfaceConfig";

const int TcpInterfaceConfig::DEFAULT_INITIALIZER_MAX_CONNECTIONS = 10000;

const bsls::Types::Int64
    TcpInterfaceConfig::DEFAULT_INITIALIZER_NODE_LOW_WATERMARK = 1024;

const bsls::Types::Int64
    TcpInterfaceConfig::DEFAULT_INITIALIZER_NODE_HIGH_WATERMARK = 2048;

const int TcpInterfaceConfig::DEFAULT_INITIALIZER_HEARTBEAT_INTERVAL_MS = 3000;

const bdlat_AttributeInfo TcpInterfaceConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PORT,
     "port",
     sizeof("port") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_IO_THREADS,
     "ioThreads",
     sizeof("ioThreads") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_CONNECTIONS,
     "maxConnections",
     sizeof("maxConnections") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_LOW_WATERMARK,
     "lowWatermark",
     sizeof("lowWatermark") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_HIGH_WATERMARK,
     "highWatermark",
     sizeof("highWatermark") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NODE_LOW_WATERMARK,
     "nodeLowWatermark",
     sizeof("nodeLowWatermark") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NODE_HIGH_WATERMARK,
     "nodeHighWatermark",
     sizeof("nodeHighWatermark") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_HEARTBEAT_INTERVAL_MS,
     "heartbeatIntervalMs",
     sizeof("heartbeatIntervalMs") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
TcpInterfaceConfig::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 9; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            TcpInterfaceConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* TcpInterfaceConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_PORT: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT];
    case ATTRIBUTE_ID_IO_THREADS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IO_THREADS];
    case ATTRIBUTE_ID_MAX_CONNECTIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_CONNECTIONS];
    case ATTRIBUTE_ID_LOW_WATERMARK:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOW_WATERMARK];
    case ATTRIBUTE_ID_HIGH_WATERMARK:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HIGH_WATERMARK];
    case ATTRIBUTE_ID_NODE_LOW_WATERMARK:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_LOW_WATERMARK];
    case ATTRIBUTE_ID_NODE_HIGH_WATERMARK:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODE_HIGH_WATERMARK];
    case ATTRIBUTE_ID_HEARTBEAT_INTERVAL_MS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEAT_INTERVAL_MS];
    default: return 0;
    }
}

// CREATORS

TcpInterfaceConfig::TcpInterfaceConfig(bslma::Allocator* basicAllocator)
: d_lowWatermark()
, d_highWatermark()
, d_nodeLowWatermark(DEFAULT_INITIALIZER_NODE_LOW_WATERMARK)
, d_nodeHighWatermark(DEFAULT_INITIALIZER_NODE_HIGH_WATERMARK)
, d_name(basicAllocator)
, d_port()
, d_ioThreads()
, d_maxConnections(DEFAULT_INITIALIZER_MAX_CONNECTIONS)
, d_heartbeatIntervalMs(DEFAULT_INITIALIZER_HEARTBEAT_INTERVAL_MS)
{
}

TcpInterfaceConfig::TcpInterfaceConfig(const TcpInterfaceConfig& original,
                                       bslma::Allocator* basicAllocator)
: d_lowWatermark(original.d_lowWatermark)
, d_highWatermark(original.d_highWatermark)
, d_nodeLowWatermark(original.d_nodeLowWatermark)
, d_nodeHighWatermark(original.d_nodeHighWatermark)
, d_name(original.d_name, basicAllocator)
, d_port(original.d_port)
, d_ioThreads(original.d_ioThreads)
, d_maxConnections(original.d_maxConnections)
, d_heartbeatIntervalMs(original.d_heartbeatIntervalMs)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TcpInterfaceConfig::TcpInterfaceConfig(TcpInterfaceConfig&& original) noexcept
: d_lowWatermark(bsl::move(original.d_lowWatermark)),
  d_highWatermark(bsl::move(original.d_highWatermark)),
  d_nodeLowWatermark(bsl::move(original.d_nodeLowWatermark)),
  d_nodeHighWatermark(bsl::move(original.d_nodeHighWatermark)),
  d_name(bsl::move(original.d_name)),
  d_port(bsl::move(original.d_port)),
  d_ioThreads(bsl::move(original.d_ioThreads)),
  d_maxConnections(bsl::move(original.d_maxConnections)),
  d_heartbeatIntervalMs(bsl::move(original.d_heartbeatIntervalMs))
{
}

TcpInterfaceConfig::TcpInterfaceConfig(TcpInterfaceConfig&& original,
                                       bslma::Allocator*    basicAllocator)
: d_lowWatermark(bsl::move(original.d_lowWatermark))
, d_highWatermark(bsl::move(original.d_highWatermark))
, d_nodeLowWatermark(bsl::move(original.d_nodeLowWatermark))
, d_nodeHighWatermark(bsl::move(original.d_nodeHighWatermark))
, d_name(bsl::move(original.d_name), basicAllocator)
, d_port(bsl::move(original.d_port))
, d_ioThreads(bsl::move(original.d_ioThreads))
, d_maxConnections(bsl::move(original.d_maxConnections))
, d_heartbeatIntervalMs(bsl::move(original.d_heartbeatIntervalMs))
{
}
#endif

TcpInterfaceConfig::~TcpInterfaceConfig()
{
}

// MANIPULATORS

TcpInterfaceConfig&
TcpInterfaceConfig::operator=(const TcpInterfaceConfig& rhs)
{
    if (this != &rhs) {
        d_name                = rhs.d_name;
        d_port                = rhs.d_port;
        d_ioThreads           = rhs.d_ioThreads;
        d_maxConnections      = rhs.d_maxConnections;
        d_lowWatermark        = rhs.d_lowWatermark;
        d_highWatermark       = rhs.d_highWatermark;
        d_nodeLowWatermark    = rhs.d_nodeLowWatermark;
        d_nodeHighWatermark   = rhs.d_nodeHighWatermark;
        d_heartbeatIntervalMs = rhs.d_heartbeatIntervalMs;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TcpInterfaceConfig& TcpInterfaceConfig::operator=(TcpInterfaceConfig&& rhs)
{
    if (this != &rhs) {
        d_name                = bsl::move(rhs.d_name);
        d_port                = bsl::move(rhs.d_port);
        d_ioThreads           = bsl::move(rhs.d_ioThreads);
        d_maxConnections      = bsl::move(rhs.d_maxConnections);
        d_lowWatermark        = bsl::move(rhs.d_lowWatermark);
        d_highWatermark       = bsl::move(rhs.d_highWatermark);
        d_nodeLowWatermark    = bsl::move(rhs.d_nodeLowWatermark);
        d_nodeHighWatermark   = bsl::move(rhs.d_nodeHighWatermark);
        d_heartbeatIntervalMs = bsl::move(rhs.d_heartbeatIntervalMs);
    }

    return *this;
}
#endif

void TcpInterfaceConfig::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_port);
    bdlat_ValueTypeFunctions::reset(&d_ioThreads);
    d_maxConnections = DEFAULT_INITIALIZER_MAX_CONNECTIONS;
    bdlat_ValueTypeFunctions::reset(&d_lowWatermark);
    bdlat_ValueTypeFunctions::reset(&d_highWatermark);
    d_nodeLowWatermark    = DEFAULT_INITIALIZER_NODE_LOW_WATERMARK;
    d_nodeHighWatermark   = DEFAULT_INITIALIZER_NODE_HIGH_WATERMARK;
    d_heartbeatIntervalMs = DEFAULT_INITIALIZER_HEARTBEAT_INTERVAL_MS;
}

// ACCESSORS

bsl::ostream& TcpInterfaceConfig::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("port", this->port());
    printer.printAttribute("ioThreads", this->ioThreads());
    printer.printAttribute("maxConnections", this->maxConnections());
    printer.printAttribute("lowWatermark", this->lowWatermark());
    printer.printAttribute("highWatermark", this->highWatermark());
    printer.printAttribute("nodeLowWatermark", this->nodeLowWatermark());
    printer.printAttribute("nodeHighWatermark", this->nodeHighWatermark());
    printer.printAttribute("heartbeatIntervalMs", this->heartbeatIntervalMs());
    printer.end();
    return stream;
}

// -------------------------------
// class VirtualClusterInformation
// -------------------------------

// CONSTANTS

const char VirtualClusterInformation::CLASS_NAME[] =
    "VirtualClusterInformation";

const bdlat_AttributeInfo VirtualClusterInformation::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_SELF_NODE_ID,
     "selfNodeId",
     sizeof("selfNodeId") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
VirtualClusterInformation::lookupAttributeInfo(const char* name,
                                               int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            VirtualClusterInformation::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
VirtualClusterInformation::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_SELF_NODE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SELF_NODE_ID];
    default: return 0;
    }
}

// CREATORS

VirtualClusterInformation::VirtualClusterInformation(
    bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_selfNodeId()
{
}

VirtualClusterInformation::VirtualClusterInformation(
    const VirtualClusterInformation& original,
    bslma::Allocator*                basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_selfNodeId(original.d_selfNodeId)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
VirtualClusterInformation::VirtualClusterInformation(
    VirtualClusterInformation&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_selfNodeId(bsl::move(original.d_selfNodeId))
{
}

VirtualClusterInformation::VirtualClusterInformation(
    VirtualClusterInformation&& original,
    bslma::Allocator*           basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_selfNodeId(bsl::move(original.d_selfNodeId))
{
}
#endif

VirtualClusterInformation::~VirtualClusterInformation()
{
}

// MANIPULATORS

VirtualClusterInformation&
VirtualClusterInformation::operator=(const VirtualClusterInformation& rhs)
{
    if (this != &rhs) {
        d_name       = rhs.d_name;
        d_selfNodeId = rhs.d_selfNodeId;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
VirtualClusterInformation&
VirtualClusterInformation::operator=(VirtualClusterInformation&& rhs)
{
    if (this != &rhs) {
        d_name       = bsl::move(rhs.d_name);
        d_selfNodeId = bsl::move(rhs.d_selfNodeId);
    }

    return *this;
}
#endif

void VirtualClusterInformation::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_selfNodeId);
}

// ACCESSORS

bsl::ostream& VirtualClusterInformation::print(bsl::ostream& stream,
                                               int           level,
                                               int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("selfNodeId", this->selfNodeId());
    printer.end();
    return stream;
}

// ---------------------------
// class ClusterNodeConnection
// ---------------------------

// CONSTANTS

const char ClusterNodeConnection::CLASS_NAME[] = "ClusterNodeConnection";

const bdlat_SelectionInfo ClusterNodeConnection::SELECTION_INFO_ARRAY[] = {
    {SELECTION_ID_TCP,
     "tcp",
     sizeof("tcp") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_SelectionInfo*
ClusterNodeConnection::lookupSelectionInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            ClusterNodeConnection::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo* ClusterNodeConnection::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_TCP: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_TCP];
    default: return 0;
    }
}

// CREATORS

ClusterNodeConnection::ClusterNodeConnection(
    const ClusterNodeConnection& original,
    bslma::Allocator*            basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_TCP: {
        new (d_tcp.buffer())
            TcpClusterNodeConnection(original.d_tcp.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterNodeConnection::ClusterNodeConnection(ClusterNodeConnection&& original)
    noexcept : d_selectionId(original.d_selectionId),
               d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_TCP: {
        new (d_tcp.buffer())
            TcpClusterNodeConnection(bsl::move(original.d_tcp.object()),
                                     d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

ClusterNodeConnection::ClusterNodeConnection(ClusterNodeConnection&& original,
                                             bslma::Allocator* basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_TCP: {
        new (d_tcp.buffer())
            TcpClusterNodeConnection(bsl::move(original.d_tcp.object()),
                                     d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

ClusterNodeConnection&
ClusterNodeConnection::operator=(const ClusterNodeConnection& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_TCP: {
            makeTcp(rhs.d_tcp.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterNodeConnection&
ClusterNodeConnection::operator=(ClusterNodeConnection&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_TCP: {
            makeTcp(bsl::move(rhs.d_tcp.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void ClusterNodeConnection::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_TCP: {
        d_tcp.object().~TcpClusterNodeConnection();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int ClusterNodeConnection::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_TCP: {
        makeTcp();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int ClusterNodeConnection::makeSelection(const char* name, int nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

TcpClusterNodeConnection& ClusterNodeConnection::makeTcp()
{
    if (SELECTION_ID_TCP == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_tcp.object());
    }
    else {
        reset();
        new (d_tcp.buffer()) TcpClusterNodeConnection(d_allocator_p);
        d_selectionId = SELECTION_ID_TCP;
    }

    return d_tcp.object();
}

TcpClusterNodeConnection&
ClusterNodeConnection::makeTcp(const TcpClusterNodeConnection& value)
{
    if (SELECTION_ID_TCP == d_selectionId) {
        d_tcp.object() = value;
    }
    else {
        reset();
        new (d_tcp.buffer()) TcpClusterNodeConnection(value, d_allocator_p);
        d_selectionId = SELECTION_ID_TCP;
    }

    return d_tcp.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TcpClusterNodeConnection&
ClusterNodeConnection::makeTcp(TcpClusterNodeConnection&& value)
{
    if (SELECTION_ID_TCP == d_selectionId) {
        d_tcp.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_tcp.buffer())
            TcpClusterNodeConnection(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_TCP;
    }

    return d_tcp.object();
}
#endif

// ACCESSORS

bsl::ostream& ClusterNodeConnection::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_TCP: {
        printer.printAttribute("tcp", d_tcp.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* ClusterNodeConnection::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_TCP:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_TCP].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -------------------------------
// class DispatcherProcessorConfig
// -------------------------------

// CONSTANTS

const char DispatcherProcessorConfig::CLASS_NAME[] =
    "DispatcherProcessorConfig";

const bdlat_AttributeInfo DispatcherProcessorConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NUM_PROCESSORS,
     "numProcessors",
     sizeof("numProcessors") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PROCESSOR_CONFIG,
     "processorConfig",
     sizeof("processorConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
DispatcherProcessorConfig::lookupAttributeInfo(const char* name,
                                               int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DispatcherProcessorConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
DispatcherProcessorConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NUM_PROCESSORS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PROCESSORS];
    case ATTRIBUTE_ID_PROCESSOR_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROCESSOR_CONFIG];
    default: return 0;
    }
}

// CREATORS

DispatcherProcessorConfig::DispatcherProcessorConfig()
: d_processorConfig()
, d_numProcessors()
{
}

// MANIPULATORS

void DispatcherProcessorConfig::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_numProcessors);
    bdlat_ValueTypeFunctions::reset(&d_processorConfig);
}

// ACCESSORS

bsl::ostream& DispatcherProcessorConfig::print(bsl::ostream& stream,
                                               int           level,
                                               int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("numProcessors", this->numProcessors());
    printer.printAttribute("processorConfig", this->processorConfig());
    printer.end();
    return stream;
}

// -------------------
// class LogController
// -------------------

// CONSTANTS

const char LogController::CLASS_NAME[] = "LogController";

const char LogController::DEFAULT_INITIALIZER_BSLS_LOG_SEVERITY_THRESHOLD[] =
    "ERROR";

const bdlat_AttributeInfo LogController::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_FILE_NAME,
     "fileName",
     sizeof("fileName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_FILE_MAX_AGE_DAYS,
     "fileMaxAgeDays",
     sizeof("fileMaxAgeDays") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_ROTATION_BYTES,
     "rotationBytes",
     sizeof("rotationBytes") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_LOGFILE_FORMAT,
     "logfileFormat",
     sizeof("logfileFormat") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CONSOLE_FORMAT,
     "consoleFormat",
     sizeof("consoleFormat") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_LOGGING_VERBOSITY,
     "loggingVerbosity",
     sizeof("loggingVerbosity") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_BSLS_LOG_SEVERITY_THRESHOLD,
     "bslsLogSeverityThreshold",
     sizeof("bslsLogSeverityThreshold") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CONSOLE_SEVERITY_THRESHOLD,
     "consoleSeverityThreshold",
     sizeof("consoleSeverityThreshold") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CATEGORIES,
     "categories",
     sizeof("categories") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_SYSLOG,
     "syslog",
     sizeof("syslog") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* LogController::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 10; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            LogController::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* LogController::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_FILE_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_NAME];
    case ATTRIBUTE_ID_FILE_MAX_AGE_DAYS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE_MAX_AGE_DAYS];
    case ATTRIBUTE_ID_ROTATION_BYTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ROTATION_BYTES];
    case ATTRIBUTE_ID_LOGFILE_FORMAT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGFILE_FORMAT];
    case ATTRIBUTE_ID_CONSOLE_FORMAT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSOLE_FORMAT];
    case ATTRIBUTE_ID_LOGGING_VERBOSITY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGGING_VERBOSITY];
    case ATTRIBUTE_ID_BSLS_LOG_SEVERITY_THRESHOLD:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_BSLS_LOG_SEVERITY_THRESHOLD];
    case ATTRIBUTE_ID_CONSOLE_SEVERITY_THRESHOLD:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_CONSOLE_SEVERITY_THRESHOLD];
    case ATTRIBUTE_ID_CATEGORIES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CATEGORIES];
    case ATTRIBUTE_ID_SYSLOG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SYSLOG];
    default: return 0;
    }
}

// CREATORS

LogController::LogController(bslma::Allocator* basicAllocator)
: d_categories(basicAllocator)
, d_fileName(basicAllocator)
, d_logfileFormat(basicAllocator)
, d_consoleFormat(basicAllocator)
, d_loggingVerbosity(basicAllocator)
, d_bslsLogSeverityThreshold(DEFAULT_INITIALIZER_BSLS_LOG_SEVERITY_THRESHOLD,
                             basicAllocator)
, d_consoleSeverityThreshold(basicAllocator)
, d_syslog(basicAllocator)
, d_fileMaxAgeDays()
, d_rotationBytes()
{
}

LogController::LogController(const LogController& original,
                             bslma::Allocator*    basicAllocator)
: d_categories(original.d_categories, basicAllocator)
, d_fileName(original.d_fileName, basicAllocator)
, d_logfileFormat(original.d_logfileFormat, basicAllocator)
, d_consoleFormat(original.d_consoleFormat, basicAllocator)
, d_loggingVerbosity(original.d_loggingVerbosity, basicAllocator)
, d_bslsLogSeverityThreshold(original.d_bslsLogSeverityThreshold,
                             basicAllocator)
, d_consoleSeverityThreshold(original.d_consoleSeverityThreshold,
                             basicAllocator)
, d_syslog(original.d_syslog, basicAllocator)
, d_fileMaxAgeDays(original.d_fileMaxAgeDays)
, d_rotationBytes(original.d_rotationBytes)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LogController::LogController(LogController&& original) noexcept
: d_categories(bsl::move(original.d_categories)),
  d_fileName(bsl::move(original.d_fileName)),
  d_logfileFormat(bsl::move(original.d_logfileFormat)),
  d_consoleFormat(bsl::move(original.d_consoleFormat)),
  d_loggingVerbosity(bsl::move(original.d_loggingVerbosity)),
  d_bslsLogSeverityThreshold(bsl::move(original.d_bslsLogSeverityThreshold)),
  d_consoleSeverityThreshold(bsl::move(original.d_consoleSeverityThreshold)),
  d_syslog(bsl::move(original.d_syslog)),
  d_fileMaxAgeDays(bsl::move(original.d_fileMaxAgeDays)),
  d_rotationBytes(bsl::move(original.d_rotationBytes))
{
}

LogController::LogController(LogController&&   original,
                             bslma::Allocator* basicAllocator)
: d_categories(bsl::move(original.d_categories), basicAllocator)
, d_fileName(bsl::move(original.d_fileName), basicAllocator)
, d_logfileFormat(bsl::move(original.d_logfileFormat), basicAllocator)
, d_consoleFormat(bsl::move(original.d_consoleFormat), basicAllocator)
, d_loggingVerbosity(bsl::move(original.d_loggingVerbosity), basicAllocator)
, d_bslsLogSeverityThreshold(bsl::move(original.d_bslsLogSeverityThreshold),
                             basicAllocator)
, d_consoleSeverityThreshold(bsl::move(original.d_consoleSeverityThreshold),
                             basicAllocator)
, d_syslog(bsl::move(original.d_syslog), basicAllocator)
, d_fileMaxAgeDays(bsl::move(original.d_fileMaxAgeDays))
, d_rotationBytes(bsl::move(original.d_rotationBytes))
{
}
#endif

LogController::~LogController()
{
}

// MANIPULATORS

LogController& LogController::operator=(const LogController& rhs)
{
    if (this != &rhs) {
        d_fileName                 = rhs.d_fileName;
        d_fileMaxAgeDays           = rhs.d_fileMaxAgeDays;
        d_rotationBytes            = rhs.d_rotationBytes;
        d_logfileFormat            = rhs.d_logfileFormat;
        d_consoleFormat            = rhs.d_consoleFormat;
        d_loggingVerbosity         = rhs.d_loggingVerbosity;
        d_bslsLogSeverityThreshold = rhs.d_bslsLogSeverityThreshold;
        d_consoleSeverityThreshold = rhs.d_consoleSeverityThreshold;
        d_categories               = rhs.d_categories;
        d_syslog                   = rhs.d_syslog;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
LogController& LogController::operator=(LogController&& rhs)
{
    if (this != &rhs) {
        d_fileName                 = bsl::move(rhs.d_fileName);
        d_fileMaxAgeDays           = bsl::move(rhs.d_fileMaxAgeDays);
        d_rotationBytes            = bsl::move(rhs.d_rotationBytes);
        d_logfileFormat            = bsl::move(rhs.d_logfileFormat);
        d_consoleFormat            = bsl::move(rhs.d_consoleFormat);
        d_loggingVerbosity         = bsl::move(rhs.d_loggingVerbosity);
        d_bslsLogSeverityThreshold = bsl::move(rhs.d_bslsLogSeverityThreshold);
        d_consoleSeverityThreshold = bsl::move(rhs.d_consoleSeverityThreshold);
        d_categories               = bsl::move(rhs.d_categories);
        d_syslog                   = bsl::move(rhs.d_syslog);
    }

    return *this;
}
#endif

void LogController::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_fileName);
    bdlat_ValueTypeFunctions::reset(&d_fileMaxAgeDays);
    bdlat_ValueTypeFunctions::reset(&d_rotationBytes);
    bdlat_ValueTypeFunctions::reset(&d_logfileFormat);
    bdlat_ValueTypeFunctions::reset(&d_consoleFormat);
    bdlat_ValueTypeFunctions::reset(&d_loggingVerbosity);
    d_bslsLogSeverityThreshold =
        DEFAULT_INITIALIZER_BSLS_LOG_SEVERITY_THRESHOLD;
    bdlat_ValueTypeFunctions::reset(&d_consoleSeverityThreshold);
    bdlat_ValueTypeFunctions::reset(&d_categories);
    bdlat_ValueTypeFunctions::reset(&d_syslog);
}

// ACCESSORS

bsl::ostream&
LogController::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("fileName", this->fileName());
    printer.printAttribute("fileMaxAgeDays", this->fileMaxAgeDays());
    printer.printAttribute("rotationBytes", this->rotationBytes());
    printer.printAttribute("logfileFormat", this->logfileFormat());
    printer.printAttribute("consoleFormat", this->consoleFormat());
    printer.printAttribute("loggingVerbosity", this->loggingVerbosity());
    printer.printAttribute("bslsLogSeverityThreshold",
                           this->bslsLogSeverityThreshold());
    printer.printAttribute("consoleSeverityThreshold",
                           this->consoleSeverityThreshold());
    printer.printAttribute("categories", this->categories());
    printer.printAttribute("syslog", this->syslog());
    printer.end();
    return stream;
}

// -----------------------
// class NetworkInterfaces
// -----------------------

// CONSTANTS

const char NetworkInterfaces::CLASS_NAME[] = "NetworkInterfaces";

const bdlat_AttributeInfo NetworkInterfaces::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_HEARTBEATS,
     "heartbeats",
     sizeof("heartbeats") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_TCP_INTERFACE,
     "tcpInterface",
     sizeof("tcpInterface") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
NetworkInterfaces::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            NetworkInterfaces::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* NetworkInterfaces::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_HEARTBEATS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HEARTBEATS];
    case ATTRIBUTE_ID_TCP_INTERFACE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TCP_INTERFACE];
    default: return 0;
    }
}

// CREATORS

NetworkInterfaces::NetworkInterfaces(bslma::Allocator* basicAllocator)
: d_tcpInterface(basicAllocator)
, d_heartbeats()
{
}

NetworkInterfaces::NetworkInterfaces(const NetworkInterfaces& original,
                                     bslma::Allocator*        basicAllocator)
: d_tcpInterface(original.d_tcpInterface, basicAllocator)
, d_heartbeats(original.d_heartbeats)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
NetworkInterfaces::NetworkInterfaces(NetworkInterfaces&& original) noexcept
: d_tcpInterface(bsl::move(original.d_tcpInterface)),
  d_heartbeats(bsl::move(original.d_heartbeats))
{
}

NetworkInterfaces::NetworkInterfaces(NetworkInterfaces&& original,
                                     bslma::Allocator*   basicAllocator)
: d_tcpInterface(bsl::move(original.d_tcpInterface), basicAllocator)
, d_heartbeats(bsl::move(original.d_heartbeats))
{
}
#endif

NetworkInterfaces::~NetworkInterfaces()
{
}

// MANIPULATORS

NetworkInterfaces& NetworkInterfaces::operator=(const NetworkInterfaces& rhs)
{
    if (this != &rhs) {
        d_heartbeats   = rhs.d_heartbeats;
        d_tcpInterface = rhs.d_tcpInterface;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
NetworkInterfaces& NetworkInterfaces::operator=(NetworkInterfaces&& rhs)
{
    if (this != &rhs) {
        d_heartbeats   = bsl::move(rhs.d_heartbeats);
        d_tcpInterface = bsl::move(rhs.d_tcpInterface);
    }

    return *this;
}
#endif

void NetworkInterfaces::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_heartbeats);
    bdlat_ValueTypeFunctions::reset(&d_tcpInterface);
}

// ACCESSORS

bsl::ostream& NetworkInterfaces::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("heartbeats", this->heartbeats());
    printer.printAttribute("tcpInterface", this->tcpInterface());
    printer.end();
    return stream;
}

// ---------------------
// class PartitionConfig
// ---------------------

// CONSTANTS

const char PartitionConfig::CLASS_NAME[] = "PartitionConfig";

const bool PartitionConfig::DEFAULT_INITIALIZER_PREALLOCATE = false;

const bool PartitionConfig::DEFAULT_INITIALIZER_PREFAULT_PAGES = false;

const bool PartitionConfig::DEFAULT_INITIALIZER_FLUSH_AT_SHUTDOWN = true;

const bdlat_AttributeInfo PartitionConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NUM_PARTITIONS,
     "numPartitions",
     sizeof("numPartitions") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_LOCATION,
     "location",
     sizeof("location") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_ARCHIVE_LOCATION,
     "archiveLocation",
     sizeof("archiveLocation") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_MAX_DATA_FILE_SIZE,
     "maxDataFileSize",
     sizeof("maxDataFileSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_JOURNAL_FILE_SIZE,
     "maxJournalFileSize",
     sizeof("maxJournalFileSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_MAX_QLIST_FILE_SIZE,
     "maxQlistFileSize",
     sizeof("maxQlistFileSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PREALLOCATE,
     "preallocate",
     sizeof("preallocate") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_MAX_ARCHIVED_FILE_SETS,
     "maxArchivedFileSets",
     sizeof("maxArchivedFileSets") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PREFAULT_PAGES,
     "prefaultPages",
     sizeof("prefaultPages") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_FLUSH_AT_SHUTDOWN,
     "flushAtShutdown",
     sizeof("flushAtShutdown") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_SYNC_CONFIG,
     "syncConfig",
     sizeof("syncConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
PartitionConfig::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 11; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            PartitionConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* PartitionConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NUM_PARTITIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NUM_PARTITIONS];
    case ATTRIBUTE_ID_LOCATION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOCATION];
    case ATTRIBUTE_ID_ARCHIVE_LOCATION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ARCHIVE_LOCATION];
    case ATTRIBUTE_ID_MAX_DATA_FILE_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_DATA_FILE_SIZE];
    case ATTRIBUTE_ID_MAX_JOURNAL_FILE_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_JOURNAL_FILE_SIZE];
    case ATTRIBUTE_ID_MAX_QLIST_FILE_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_QLIST_FILE_SIZE];
    case ATTRIBUTE_ID_PREALLOCATE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PREALLOCATE];
    case ATTRIBUTE_ID_MAX_ARCHIVED_FILE_SETS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_ARCHIVED_FILE_SETS];
    case ATTRIBUTE_ID_PREFAULT_PAGES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PREFAULT_PAGES];
    case ATTRIBUTE_ID_FLUSH_AT_SHUTDOWN:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLUSH_AT_SHUTDOWN];
    case ATTRIBUTE_ID_SYNC_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SYNC_CONFIG];
    default: return 0;
    }
}

// CREATORS

PartitionConfig::PartitionConfig(bslma::Allocator* basicAllocator)
: d_maxDataFileSize()
, d_maxJournalFileSize()
, d_maxQlistFileSize()
, d_location(basicAllocator)
, d_archiveLocation(basicAllocator)
, d_syncConfig()
, d_numPartitions()
, d_maxArchivedFileSets()
, d_preallocate(DEFAULT_INITIALIZER_PREALLOCATE)
, d_prefaultPages(DEFAULT_INITIALIZER_PREFAULT_PAGES)
, d_flushAtShutdown(DEFAULT_INITIALIZER_FLUSH_AT_SHUTDOWN)
{
}

PartitionConfig::PartitionConfig(const PartitionConfig& original,
                                 bslma::Allocator*      basicAllocator)
: d_maxDataFileSize(original.d_maxDataFileSize)
, d_maxJournalFileSize(original.d_maxJournalFileSize)
, d_maxQlistFileSize(original.d_maxQlistFileSize)
, d_location(original.d_location, basicAllocator)
, d_archiveLocation(original.d_archiveLocation, basicAllocator)
, d_syncConfig(original.d_syncConfig)
, d_numPartitions(original.d_numPartitions)
, d_maxArchivedFileSets(original.d_maxArchivedFileSets)
, d_preallocate(original.d_preallocate)
, d_prefaultPages(original.d_prefaultPages)
, d_flushAtShutdown(original.d_flushAtShutdown)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionConfig::PartitionConfig(PartitionConfig&& original) noexcept
: d_maxDataFileSize(bsl::move(original.d_maxDataFileSize)),
  d_maxJournalFileSize(bsl::move(original.d_maxJournalFileSize)),
  d_maxQlistFileSize(bsl::move(original.d_maxQlistFileSize)),
  d_location(bsl::move(original.d_location)),
  d_archiveLocation(bsl::move(original.d_archiveLocation)),
  d_syncConfig(bsl::move(original.d_syncConfig)),
  d_numPartitions(bsl::move(original.d_numPartitions)),
  d_maxArchivedFileSets(bsl::move(original.d_maxArchivedFileSets)),
  d_preallocate(bsl::move(original.d_preallocate)),
  d_prefaultPages(bsl::move(original.d_prefaultPages)),
  d_flushAtShutdown(bsl::move(original.d_flushAtShutdown))
{
}

PartitionConfig::PartitionConfig(PartitionConfig&& original,
                                 bslma::Allocator* basicAllocator)
: d_maxDataFileSize(bsl::move(original.d_maxDataFileSize))
, d_maxJournalFileSize(bsl::move(original.d_maxJournalFileSize))
, d_maxQlistFileSize(bsl::move(original.d_maxQlistFileSize))
, d_location(bsl::move(original.d_location), basicAllocator)
, d_archiveLocation(bsl::move(original.d_archiveLocation), basicAllocator)
, d_syncConfig(bsl::move(original.d_syncConfig))
, d_numPartitions(bsl::move(original.d_numPartitions))
, d_maxArchivedFileSets(bsl::move(original.d_maxArchivedFileSets))
, d_preallocate(bsl::move(original.d_preallocate))
, d_prefaultPages(bsl::move(original.d_prefaultPages))
, d_flushAtShutdown(bsl::move(original.d_flushAtShutdown))
{
}
#endif

PartitionConfig::~PartitionConfig()
{
}

// MANIPULATORS

PartitionConfig& PartitionConfig::operator=(const PartitionConfig& rhs)
{
    if (this != &rhs) {
        d_numPartitions       = rhs.d_numPartitions;
        d_location            = rhs.d_location;
        d_archiveLocation     = rhs.d_archiveLocation;
        d_maxDataFileSize     = rhs.d_maxDataFileSize;
        d_maxJournalFileSize  = rhs.d_maxJournalFileSize;
        d_maxQlistFileSize    = rhs.d_maxQlistFileSize;
        d_preallocate         = rhs.d_preallocate;
        d_maxArchivedFileSets = rhs.d_maxArchivedFileSets;
        d_prefaultPages       = rhs.d_prefaultPages;
        d_flushAtShutdown     = rhs.d_flushAtShutdown;
        d_syncConfig          = rhs.d_syncConfig;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
PartitionConfig& PartitionConfig::operator=(PartitionConfig&& rhs)
{
    if (this != &rhs) {
        d_numPartitions       = bsl::move(rhs.d_numPartitions);
        d_location            = bsl::move(rhs.d_location);
        d_archiveLocation     = bsl::move(rhs.d_archiveLocation);
        d_maxDataFileSize     = bsl::move(rhs.d_maxDataFileSize);
        d_maxJournalFileSize  = bsl::move(rhs.d_maxJournalFileSize);
        d_maxQlistFileSize    = bsl::move(rhs.d_maxQlistFileSize);
        d_preallocate         = bsl::move(rhs.d_preallocate);
        d_maxArchivedFileSets = bsl::move(rhs.d_maxArchivedFileSets);
        d_prefaultPages       = bsl::move(rhs.d_prefaultPages);
        d_flushAtShutdown     = bsl::move(rhs.d_flushAtShutdown);
        d_syncConfig          = bsl::move(rhs.d_syncConfig);
    }

    return *this;
}
#endif

void PartitionConfig::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_numPartitions);
    bdlat_ValueTypeFunctions::reset(&d_location);
    bdlat_ValueTypeFunctions::reset(&d_archiveLocation);
    bdlat_ValueTypeFunctions::reset(&d_maxDataFileSize);
    bdlat_ValueTypeFunctions::reset(&d_maxJournalFileSize);
    bdlat_ValueTypeFunctions::reset(&d_maxQlistFileSize);
    d_preallocate = DEFAULT_INITIALIZER_PREALLOCATE;
    bdlat_ValueTypeFunctions::reset(&d_maxArchivedFileSets);
    d_prefaultPages   = DEFAULT_INITIALIZER_PREFAULT_PAGES;
    d_flushAtShutdown = DEFAULT_INITIALIZER_FLUSH_AT_SHUTDOWN;
    bdlat_ValueTypeFunctions::reset(&d_syncConfig);
}

// ACCESSORS

bsl::ostream& PartitionConfig::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("numPartitions", this->numPartitions());
    printer.printAttribute("location", this->location());
    printer.printAttribute("archiveLocation", this->archiveLocation());
    printer.printAttribute("maxDataFileSize", this->maxDataFileSize());
    printer.printAttribute("maxJournalFileSize", this->maxJournalFileSize());
    printer.printAttribute("maxQlistFileSize", this->maxQlistFileSize());
    printer.printAttribute("preallocate", this->preallocate());
    printer.printAttribute("maxArchivedFileSets", this->maxArchivedFileSets());
    printer.printAttribute("prefaultPages", this->prefaultPages());
    printer.printAttribute("flushAtShutdown", this->flushAtShutdown());
    printer.printAttribute("syncConfig", this->syncConfig());
    printer.end();
    return stream;
}

// --------------------------------
// class StatPluginConfigPrometheus
// --------------------------------

// CONSTANTS

const char StatPluginConfigPrometheus::CLASS_NAME[] =
    "StatPluginConfigPrometheus";

const ExportMode::Value StatPluginConfigPrometheus::DEFAULT_INITIALIZER_MODE =
    ExportMode::E_PULL;

const char StatPluginConfigPrometheus::DEFAULT_INITIALIZER_HOST[] =
    "localhost";

const int StatPluginConfigPrometheus::DEFAULT_INITIALIZER_PORT = 8080;

const bdlat_AttributeInfo StatPluginConfigPrometheus::ATTRIBUTE_INFO_ARRAY[] =
    {{ATTRIBUTE_ID_MODE,
      "mode",
      sizeof("mode") - 1,
      "",
      bdlat_FormattingMode::e_DEFAULT},
     {ATTRIBUTE_ID_HOST,
      "host",
      sizeof("host") - 1,
      "",
      bdlat_FormattingMode::e_TEXT},
     {ATTRIBUTE_ID_PORT,
      "port",
      sizeof("port") - 1,
      "",
      bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
StatPluginConfigPrometheus::lookupAttributeInfo(const char* name,
                                                int         nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StatPluginConfigPrometheus::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
StatPluginConfigPrometheus::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_MODE: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE];
    case ATTRIBUTE_ID_HOST: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST];
    case ATTRIBUTE_ID_PORT: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PORT];
    default: return 0;
    }
}

// CREATORS

StatPluginConfigPrometheus::StatPluginConfigPrometheus(
    bslma::Allocator* basicAllocator)
: d_host(DEFAULT_INITIALIZER_HOST, basicAllocator)
, d_port(DEFAULT_INITIALIZER_PORT)
, d_mode(DEFAULT_INITIALIZER_MODE)
{
}

StatPluginConfigPrometheus::StatPluginConfigPrometheus(
    const StatPluginConfigPrometheus& original,
    bslma::Allocator*                 basicAllocator)
: d_host(original.d_host, basicAllocator)
, d_port(original.d_port)
, d_mode(original.d_mode)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatPluginConfigPrometheus::StatPluginConfigPrometheus(
    StatPluginConfigPrometheus&& original) noexcept
: d_host(bsl::move(original.d_host)),
  d_port(bsl::move(original.d_port)),
  d_mode(bsl::move(original.d_mode))
{
}

StatPluginConfigPrometheus::StatPluginConfigPrometheus(
    StatPluginConfigPrometheus&& original,
    bslma::Allocator*            basicAllocator)
: d_host(bsl::move(original.d_host), basicAllocator)
, d_port(bsl::move(original.d_port))
, d_mode(bsl::move(original.d_mode))
{
}
#endif

StatPluginConfigPrometheus::~StatPluginConfigPrometheus()
{
}

// MANIPULATORS

StatPluginConfigPrometheus&
StatPluginConfigPrometheus::operator=(const StatPluginConfigPrometheus& rhs)
{
    if (this != &rhs) {
        d_mode = rhs.d_mode;
        d_host = rhs.d_host;
        d_port = rhs.d_port;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatPluginConfigPrometheus&
StatPluginConfigPrometheus::operator=(StatPluginConfigPrometheus&& rhs)
{
    if (this != &rhs) {
        d_mode = bsl::move(rhs.d_mode);
        d_host = bsl::move(rhs.d_host);
        d_port = bsl::move(rhs.d_port);
    }

    return *this;
}
#endif

void StatPluginConfigPrometheus::reset()
{
    d_mode = DEFAULT_INITIALIZER_MODE;
    d_host = DEFAULT_INITIALIZER_HOST;
    d_port = DEFAULT_INITIALIZER_PORT;
}

// ACCESSORS

bsl::ostream& StatPluginConfigPrometheus::print(bsl::ostream& stream,
                                                int           level,
                                                int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("mode", this->mode());
    printer.printAttribute("host", this->host());
    printer.printAttribute("port", this->port());
    printer.end();
    return stream;
}

// -----------------
// class ClusterNode
// -----------------

// CONSTANTS

const char ClusterNode::CLASS_NAME[] = "ClusterNode";

const bdlat_AttributeInfo ClusterNode::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ID, "id", sizeof("id") - 1, "", bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_DATA_CENTER,
     "dataCenter",
     sizeof("dataCenter") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_TRANSPORT,
     "transport",
     sizeof("transport") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* ClusterNode::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterNode::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterNode::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID];
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_DATA_CENTER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_CENTER];
    case ATTRIBUTE_ID_TRANSPORT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TRANSPORT];
    default: return 0;
    }
}

// CREATORS

ClusterNode::ClusterNode(bslma::Allocator* basicAllocator)
: d_name(basicAllocator)
, d_dataCenter(basicAllocator)
, d_transport(basicAllocator)
, d_id()
{
}

ClusterNode::ClusterNode(const ClusterNode& original,
                         bslma::Allocator*  basicAllocator)
: d_name(original.d_name, basicAllocator)
, d_dataCenter(original.d_dataCenter, basicAllocator)
, d_transport(original.d_transport, basicAllocator)
, d_id(original.d_id)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterNode::ClusterNode(ClusterNode&& original) noexcept
: d_name(bsl::move(original.d_name)),
  d_dataCenter(bsl::move(original.d_dataCenter)),
  d_transport(bsl::move(original.d_transport)),
  d_id(bsl::move(original.d_id))
{
}

ClusterNode::ClusterNode(ClusterNode&&     original,
                         bslma::Allocator* basicAllocator)
: d_name(bsl::move(original.d_name), basicAllocator)
, d_dataCenter(bsl::move(original.d_dataCenter), basicAllocator)
, d_transport(bsl::move(original.d_transport), basicAllocator)
, d_id(bsl::move(original.d_id))
{
}
#endif

ClusterNode::~ClusterNode()
{
}

// MANIPULATORS

ClusterNode& ClusterNode::operator=(const ClusterNode& rhs)
{
    if (this != &rhs) {
        d_id         = rhs.d_id;
        d_name       = rhs.d_name;
        d_dataCenter = rhs.d_dataCenter;
        d_transport  = rhs.d_transport;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterNode& ClusterNode::operator=(ClusterNode&& rhs)
{
    if (this != &rhs) {
        d_id         = bsl::move(rhs.d_id);
        d_name       = bsl::move(rhs.d_name);
        d_dataCenter = bsl::move(rhs.d_dataCenter);
        d_transport  = bsl::move(rhs.d_transport);
    }

    return *this;
}
#endif

void ClusterNode::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_id);
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_dataCenter);
    bdlat_ValueTypeFunctions::reset(&d_transport);
}

// ACCESSORS

bsl::ostream&
ClusterNode::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("id", this->id());
    printer.printAttribute("name", this->name());
    printer.printAttribute("dataCenter", this->dataCenter());
    printer.printAttribute("transport", this->transport());
    printer.end();
    return stream;
}

// ----------------------
// class DispatcherConfig
// ----------------------

// CONSTANTS

const char DispatcherConfig::CLASS_NAME[] = "DispatcherConfig";

const bdlat_AttributeInfo DispatcherConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SESSIONS,
     "sessions",
     sizeof("sessions") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUES,
     "queues",
     sizeof("queues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_CLUSTERS,
     "clusters",
     sizeof("clusters") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
DispatcherConfig::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            DispatcherConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* DispatcherConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SESSIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SESSIONS];
    case ATTRIBUTE_ID_QUEUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUES];
    case ATTRIBUTE_ID_CLUSTERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTERS];
    default: return 0;
    }
}

// CREATORS

DispatcherConfig::DispatcherConfig()
: d_sessions()
, d_queues()
, d_clusters()
{
}

// MANIPULATORS

void DispatcherConfig::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_sessions);
    bdlat_ValueTypeFunctions::reset(&d_queues);
    bdlat_ValueTypeFunctions::reset(&d_clusters);
}

// ACCESSORS

bsl::ostream& DispatcherConfig::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("sessions", this->sessions());
    printer.printAttribute("queues", this->queues());
    printer.printAttribute("clusters", this->clusters());
    printer.end();
    return stream;
}

// -------------------------------
// class ReversedClusterConnection
// -------------------------------

// CONSTANTS

const char ReversedClusterConnection::CLASS_NAME[] =
    "ReversedClusterConnection";

const bdlat_AttributeInfo ReversedClusterConnection::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_CONNECTIONS,
     "connections",
     sizeof("connections") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ReversedClusterConnection::lookupAttributeInfo(const char* name,
                                               int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ReversedClusterConnection::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
ReversedClusterConnection::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_CONNECTIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONNECTIONS];
    default: return 0;
    }
}

// CREATORS

ReversedClusterConnection::ReversedClusterConnection(
    bslma::Allocator* basicAllocator)
: d_connections(basicAllocator)
, d_name(basicAllocator)
{
}

ReversedClusterConnection::ReversedClusterConnection(
    const ReversedClusterConnection& original,
    bslma::Allocator*                basicAllocator)
: d_connections(original.d_connections, basicAllocator)
, d_name(original.d_name, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReversedClusterConnection::ReversedClusterConnection(
    ReversedClusterConnection&& original) noexcept
: d_connections(bsl::move(original.d_connections)),
  d_name(bsl::move(original.d_name))
{
}

ReversedClusterConnection::ReversedClusterConnection(
    ReversedClusterConnection&& original,
    bslma::Allocator*           basicAllocator)
: d_connections(bsl::move(original.d_connections), basicAllocator)
, d_name(bsl::move(original.d_name), basicAllocator)
{
}
#endif

ReversedClusterConnection::~ReversedClusterConnection()
{
}

// MANIPULATORS

ReversedClusterConnection&
ReversedClusterConnection::operator=(const ReversedClusterConnection& rhs)
{
    if (this != &rhs) {
        d_name        = rhs.d_name;
        d_connections = rhs.d_connections;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ReversedClusterConnection&
ReversedClusterConnection::operator=(ReversedClusterConnection&& rhs)
{
    if (this != &rhs) {
        d_name        = bsl::move(rhs.d_name);
        d_connections = bsl::move(rhs.d_connections);
    }

    return *this;
}
#endif

void ReversedClusterConnection::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_connections);
}

// ACCESSORS

bsl::ostream& ReversedClusterConnection::print(bsl::ostream& stream,
                                               int           level,
                                               int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("connections", this->connections());
    printer.end();
    return stream;
}

// ----------------------
// class StatPluginConfig
// ----------------------

// CONSTANTS

const char StatPluginConfig::CLASS_NAME[] = "StatPluginConfig";

const char StatPluginConfig::DEFAULT_INITIALIZER_NAME[] = "";

const int StatPluginConfig::DEFAULT_INITIALIZER_QUEUE_SIZE = 10000;

const int StatPluginConfig::DEFAULT_INITIALIZER_QUEUE_HIGH_WATERMARK = 5000;

const int StatPluginConfig::DEFAULT_INITIALIZER_QUEUE_LOW_WATERMARK = 1000;

const int StatPluginConfig::DEFAULT_INITIALIZER_PUBLISH_INTERVAL = 30;

const char StatPluginConfig::DEFAULT_INITIALIZER_NAMESPACE_PREFIX[] = "";

const char StatPluginConfig::DEFAULT_INITIALIZER_INSTANCE_ID[] = "";

const bdlat_AttributeInfo StatPluginConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_QUEUE_SIZE,
     "queueSize",
     sizeof("queueSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_QUEUE_HIGH_WATERMARK,
     "queueHighWatermark",
     sizeof("queueHighWatermark") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_QUEUE_LOW_WATERMARK,
     "queueLowWatermark",
     sizeof("queueLowWatermark") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_PUBLISH_INTERVAL,
     "publishInterval",
     sizeof("publishInterval") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_NAMESPACE_PREFIX,
     "namespacePrefix",
     sizeof("namespacePrefix") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_HOSTS,
     "hosts",
     sizeof("hosts") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_INSTANCE_ID,
     "instanceId",
     sizeof("instanceId") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PROMETHEUS_SPECIFIC,
     "prometheusSpecific",
     sizeof("prometheusSpecific") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
StatPluginConfig::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 9; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StatPluginConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StatPluginConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_QUEUE_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_SIZE];
    case ATTRIBUTE_ID_QUEUE_HIGH_WATERMARK:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_HIGH_WATERMARK];
    case ATTRIBUTE_ID_QUEUE_LOW_WATERMARK:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_LOW_WATERMARK];
    case ATTRIBUTE_ID_PUBLISH_INTERVAL:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PUBLISH_INTERVAL];
    case ATTRIBUTE_ID_NAMESPACE_PREFIX:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAMESPACE_PREFIX];
    case ATTRIBUTE_ID_HOSTS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOSTS];
    case ATTRIBUTE_ID_INSTANCE_ID:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_INSTANCE_ID];
    case ATTRIBUTE_ID_PROMETHEUS_SPECIFIC:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROMETHEUS_SPECIFIC];
    default: return 0;
    }
}

// CREATORS

StatPluginConfig::StatPluginConfig(bslma::Allocator* basicAllocator)
: d_hosts(basicAllocator)
, d_name(DEFAULT_INITIALIZER_NAME, basicAllocator)
, d_namespacePrefix(DEFAULT_INITIALIZER_NAMESPACE_PREFIX, basicAllocator)
, d_instanceId(DEFAULT_INITIALIZER_INSTANCE_ID, basicAllocator)
, d_prometheusSpecific(basicAllocator)
, d_queueSize(DEFAULT_INITIALIZER_QUEUE_SIZE)
, d_queueHighWatermark(DEFAULT_INITIALIZER_QUEUE_HIGH_WATERMARK)
, d_queueLowWatermark(DEFAULT_INITIALIZER_QUEUE_LOW_WATERMARK)
, d_publishInterval(DEFAULT_INITIALIZER_PUBLISH_INTERVAL)
{
}

StatPluginConfig::StatPluginConfig(const StatPluginConfig& original,
                                   bslma::Allocator*       basicAllocator)
: d_hosts(original.d_hosts, basicAllocator)
, d_name(original.d_name, basicAllocator)
, d_namespacePrefix(original.d_namespacePrefix, basicAllocator)
, d_instanceId(original.d_instanceId, basicAllocator)
, d_prometheusSpecific(original.d_prometheusSpecific, basicAllocator)
, d_queueSize(original.d_queueSize)
, d_queueHighWatermark(original.d_queueHighWatermark)
, d_queueLowWatermark(original.d_queueLowWatermark)
, d_publishInterval(original.d_publishInterval)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatPluginConfig::StatPluginConfig(StatPluginConfig&& original) noexcept
: d_hosts(bsl::move(original.d_hosts)),
  d_name(bsl::move(original.d_name)),
  d_namespacePrefix(bsl::move(original.d_namespacePrefix)),
  d_instanceId(bsl::move(original.d_instanceId)),
  d_prometheusSpecific(bsl::move(original.d_prometheusSpecific)),
  d_queueSize(bsl::move(original.d_queueSize)),
  d_queueHighWatermark(bsl::move(original.d_queueHighWatermark)),
  d_queueLowWatermark(bsl::move(original.d_queueLowWatermark)),
  d_publishInterval(bsl::move(original.d_publishInterval))
{
}

StatPluginConfig::StatPluginConfig(StatPluginConfig&& original,
                                   bslma::Allocator*  basicAllocator)
: d_hosts(bsl::move(original.d_hosts), basicAllocator)
, d_name(bsl::move(original.d_name), basicAllocator)
, d_namespacePrefix(bsl::move(original.d_namespacePrefix), basicAllocator)
, d_instanceId(bsl::move(original.d_instanceId), basicAllocator)
, d_prometheusSpecific(bsl::move(original.d_prometheusSpecific),
                       basicAllocator)
, d_queueSize(bsl::move(original.d_queueSize))
, d_queueHighWatermark(bsl::move(original.d_queueHighWatermark))
, d_queueLowWatermark(bsl::move(original.d_queueLowWatermark))
, d_publishInterval(bsl::move(original.d_publishInterval))
{
}
#endif

StatPluginConfig::~StatPluginConfig()
{
}

// MANIPULATORS

StatPluginConfig& StatPluginConfig::operator=(const StatPluginConfig& rhs)
{
    if (this != &rhs) {
        d_name               = rhs.d_name;
        d_queueSize          = rhs.d_queueSize;
        d_queueHighWatermark = rhs.d_queueHighWatermark;
        d_queueLowWatermark  = rhs.d_queueLowWatermark;
        d_publishInterval    = rhs.d_publishInterval;
        d_namespacePrefix    = rhs.d_namespacePrefix;
        d_hosts              = rhs.d_hosts;
        d_instanceId         = rhs.d_instanceId;
        d_prometheusSpecific = rhs.d_prometheusSpecific;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatPluginConfig& StatPluginConfig::operator=(StatPluginConfig&& rhs)
{
    if (this != &rhs) {
        d_name               = bsl::move(rhs.d_name);
        d_queueSize          = bsl::move(rhs.d_queueSize);
        d_queueHighWatermark = bsl::move(rhs.d_queueHighWatermark);
        d_queueLowWatermark  = bsl::move(rhs.d_queueLowWatermark);
        d_publishInterval    = bsl::move(rhs.d_publishInterval);
        d_namespacePrefix    = bsl::move(rhs.d_namespacePrefix);
        d_hosts              = bsl::move(rhs.d_hosts);
        d_instanceId         = bsl::move(rhs.d_instanceId);
        d_prometheusSpecific = bsl::move(rhs.d_prometheusSpecific);
    }

    return *this;
}
#endif

void StatPluginConfig::reset()
{
    d_name               = DEFAULT_INITIALIZER_NAME;
    d_queueSize          = DEFAULT_INITIALIZER_QUEUE_SIZE;
    d_queueHighWatermark = DEFAULT_INITIALIZER_QUEUE_HIGH_WATERMARK;
    d_queueLowWatermark  = DEFAULT_INITIALIZER_QUEUE_LOW_WATERMARK;
    d_publishInterval    = DEFAULT_INITIALIZER_PUBLISH_INTERVAL;
    d_namespacePrefix    = DEFAULT_INITIALIZER_NAMESPACE_PREFIX;
    bdlat_ValueTypeFunctions::reset(&d_hosts);
    d_instanceId = DEFAULT_INITIALIZER_INSTANCE_ID;
    bdlat_ValueTypeFunctions::reset(&d_prometheusSpecific);
}

// ACCESSORS

bsl::ostream& StatPluginConfig::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("queueSize", this->queueSize());
    printer.printAttribute("queueHighWatermark", this->queueHighWatermark());
    printer.printAttribute("queueLowWatermark", this->queueLowWatermark());
    printer.printAttribute("publishInterval", this->publishInterval());
    printer.printAttribute("namespacePrefix", this->namespacePrefix());
    printer.printAttribute("hosts", this->hosts());
    printer.printAttribute("instanceId", this->instanceId());
    printer.printAttribute("prometheusSpecific", this->prometheusSpecific());
    printer.end();
    return stream;
}

// ----------------
// class TaskConfig
// ----------------

// CONSTANTS

const char TaskConfig::CLASS_NAME[] = "TaskConfig";

const bdlat_AttributeInfo TaskConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ALLOCATOR_TYPE,
     "allocatorType",
     sizeof("allocatorType") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_ALLOCATION_LIMIT,
     "allocationLimit",
     sizeof("allocationLimit") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_LOG_CONTROLLER,
     "logController",
     sizeof("logController") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* TaskConfig::lookupAttributeInfo(const char* name,
                                                           int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            TaskConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* TaskConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ALLOCATOR_TYPE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALLOCATOR_TYPE];
    case ATTRIBUTE_ID_ALLOCATION_LIMIT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ALLOCATION_LIMIT];
    case ATTRIBUTE_ID_LOG_CONTROLLER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_CONTROLLER];
    default: return 0;
    }
}

// CREATORS

TaskConfig::TaskConfig(bslma::Allocator* basicAllocator)
: d_allocationLimit()
, d_logController(basicAllocator)
, d_allocatorType(static_cast<AllocatorType::Value>(0))
{
}

TaskConfig::TaskConfig(const TaskConfig& original,
                       bslma::Allocator* basicAllocator)
: d_allocationLimit(original.d_allocationLimit)
, d_logController(original.d_logController, basicAllocator)
, d_allocatorType(original.d_allocatorType)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TaskConfig::TaskConfig(TaskConfig&& original) noexcept
: d_allocationLimit(bsl::move(original.d_allocationLimit)),
  d_logController(bsl::move(original.d_logController)),
  d_allocatorType(bsl::move(original.d_allocatorType))
{
}

TaskConfig::TaskConfig(TaskConfig&& original, bslma::Allocator* basicAllocator)
: d_allocationLimit(bsl::move(original.d_allocationLimit))
, d_logController(bsl::move(original.d_logController), basicAllocator)
, d_allocatorType(bsl::move(original.d_allocatorType))
{
}
#endif

TaskConfig::~TaskConfig()
{
}

// MANIPULATORS

TaskConfig& TaskConfig::operator=(const TaskConfig& rhs)
{
    if (this != &rhs) {
        d_allocatorType   = rhs.d_allocatorType;
        d_allocationLimit = rhs.d_allocationLimit;
        d_logController   = rhs.d_logController;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
TaskConfig& TaskConfig::operator=(TaskConfig&& rhs)
{
    if (this != &rhs) {
        d_allocatorType   = bsl::move(rhs.d_allocatorType);
        d_allocationLimit = bsl::move(rhs.d_allocationLimit);
        d_logController   = bsl::move(rhs.d_logController);
    }

    return *this;
}
#endif

void TaskConfig::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_allocatorType);
    bdlat_ValueTypeFunctions::reset(&d_allocationLimit);
    bdlat_ValueTypeFunctions::reset(&d_logController);
}

// ACCESSORS

bsl::ostream&
TaskConfig::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("allocatorType", this->allocatorType());
    printer.printAttribute("allocationLimit", this->allocationLimit());
    printer.printAttribute("logController", this->logController());
    printer.end();
    return stream;
}

// -----------------------
// class ClusterDefinition
// -----------------------

// CONSTANTS

const char ClusterDefinition::CLASS_NAME[] = "ClusterDefinition";

const bdlat_AttributeInfo ClusterDefinition::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NODES,
     "nodes",
     sizeof("nodes") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PARTITION_CONFIG,
     "partitionConfig",
     sizeof("partitionConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_MASTER_ASSIGNMENT,
     "masterAssignment",
     sizeof("masterAssignment") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_ELECTOR,
     "elector",
     sizeof("elector") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUE_OPERATIONS,
     "queueOperations",
     sizeof("queueOperations") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_CLUSTER_ATTRIBUTES,
     "clusterAttributes",
     sizeof("clusterAttributes") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_CLUSTER_MONITOR_CONFIG,
     "clusterMonitorConfig",
     sizeof("clusterMonitorConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_MESSAGE_THROTTLE_CONFIG,
     "messageThrottleConfig",
     sizeof("messageThrottleConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterDefinition::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 9; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterDefinition::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterDefinition::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_NODES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES];
    case ATTRIBUTE_ID_PARTITION_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PARTITION_CONFIG];
    case ATTRIBUTE_ID_MASTER_ASSIGNMENT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MASTER_ASSIGNMENT];
    case ATTRIBUTE_ID_ELECTOR:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ELECTOR];
    case ATTRIBUTE_ID_QUEUE_OPERATIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_OPERATIONS];
    case ATTRIBUTE_ID_CLUSTER_ATTRIBUTES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_ATTRIBUTES];
    case ATTRIBUTE_ID_CLUSTER_MONITOR_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG];
    case ATTRIBUTE_ID_MESSAGE_THROTTLE_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG];
    default: return 0;
    }
}

// CREATORS

ClusterDefinition::ClusterDefinition(bslma::Allocator* basicAllocator)
: d_nodes(basicAllocator)
, d_name(basicAllocator)
, d_queueOperations()
, d_partitionConfig(basicAllocator)
, d_messageThrottleConfig()
, d_elector()
, d_clusterMonitorConfig()
, d_masterAssignment(static_cast<MasterAssignmentAlgorithm::Value>(0))
, d_clusterAttributes()
{
}

ClusterDefinition::ClusterDefinition(const ClusterDefinition& original,
                                     bslma::Allocator*        basicAllocator)
: d_nodes(original.d_nodes, basicAllocator)
, d_name(original.d_name, basicAllocator)
, d_queueOperations(original.d_queueOperations)
, d_partitionConfig(original.d_partitionConfig, basicAllocator)
, d_messageThrottleConfig(original.d_messageThrottleConfig)
, d_elector(original.d_elector)
, d_clusterMonitorConfig(original.d_clusterMonitorConfig)
, d_masterAssignment(original.d_masterAssignment)
, d_clusterAttributes(original.d_clusterAttributes)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterDefinition::ClusterDefinition(ClusterDefinition&& original) noexcept
: d_nodes(bsl::move(original.d_nodes)),
  d_name(bsl::move(original.d_name)),
  d_queueOperations(bsl::move(original.d_queueOperations)),
  d_partitionConfig(bsl::move(original.d_partitionConfig)),
  d_messageThrottleConfig(bsl::move(original.d_messageThrottleConfig)),
  d_elector(bsl::move(original.d_elector)),
  d_clusterMonitorConfig(bsl::move(original.d_clusterMonitorConfig)),
  d_masterAssignment(bsl::move(original.d_masterAssignment)),
  d_clusterAttributes(bsl::move(original.d_clusterAttributes))
{
}

ClusterDefinition::ClusterDefinition(ClusterDefinition&& original,
                                     bslma::Allocator*   basicAllocator)
: d_nodes(bsl::move(original.d_nodes), basicAllocator)
, d_name(bsl::move(original.d_name), basicAllocator)
, d_queueOperations(bsl::move(original.d_queueOperations))
, d_partitionConfig(bsl::move(original.d_partitionConfig), basicAllocator)
, d_messageThrottleConfig(bsl::move(original.d_messageThrottleConfig))
, d_elector(bsl::move(original.d_elector))
, d_clusterMonitorConfig(bsl::move(original.d_clusterMonitorConfig))
, d_masterAssignment(bsl::move(original.d_masterAssignment))
, d_clusterAttributes(bsl::move(original.d_clusterAttributes))
{
}
#endif

ClusterDefinition::~ClusterDefinition()
{
}

// MANIPULATORS

ClusterDefinition& ClusterDefinition::operator=(const ClusterDefinition& rhs)
{
    if (this != &rhs) {
        d_name                  = rhs.d_name;
        d_nodes                 = rhs.d_nodes;
        d_partitionConfig       = rhs.d_partitionConfig;
        d_masterAssignment      = rhs.d_masterAssignment;
        d_elector               = rhs.d_elector;
        d_queueOperations       = rhs.d_queueOperations;
        d_clusterAttributes     = rhs.d_clusterAttributes;
        d_clusterMonitorConfig  = rhs.d_clusterMonitorConfig;
        d_messageThrottleConfig = rhs.d_messageThrottleConfig;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterDefinition& ClusterDefinition::operator=(ClusterDefinition&& rhs)
{
    if (this != &rhs) {
        d_name                  = bsl::move(rhs.d_name);
        d_nodes                 = bsl::move(rhs.d_nodes);
        d_partitionConfig       = bsl::move(rhs.d_partitionConfig);
        d_masterAssignment      = bsl::move(rhs.d_masterAssignment);
        d_elector               = bsl::move(rhs.d_elector);
        d_queueOperations       = bsl::move(rhs.d_queueOperations);
        d_clusterAttributes     = bsl::move(rhs.d_clusterAttributes);
        d_clusterMonitorConfig  = bsl::move(rhs.d_clusterMonitorConfig);
        d_messageThrottleConfig = bsl::move(rhs.d_messageThrottleConfig);
    }

    return *this;
}
#endif

void ClusterDefinition::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_nodes);
    bdlat_ValueTypeFunctions::reset(&d_partitionConfig);
    bdlat_ValueTypeFunctions::reset(&d_masterAssignment);
    bdlat_ValueTypeFunctions::reset(&d_elector);
    bdlat_ValueTypeFunctions::reset(&d_queueOperations);
    bdlat_ValueTypeFunctions::reset(&d_clusterAttributes);
    bdlat_ValueTypeFunctions::reset(&d_clusterMonitorConfig);
    bdlat_ValueTypeFunctions::reset(&d_messageThrottleConfig);
}

// ACCESSORS

bsl::ostream& ClusterDefinition::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("nodes", this->nodes());
    printer.printAttribute("partitionConfig", this->partitionConfig());
    printer.printAttribute("masterAssignment", this->masterAssignment());
    printer.printAttribute("elector", this->elector());
    printer.printAttribute("queueOperations", this->queueOperations());
    printer.printAttribute("clusterAttributes", this->clusterAttributes());
    printer.printAttribute("clusterMonitorConfig",
                           this->clusterMonitorConfig());
    printer.printAttribute("messageThrottleConfig",
                           this->messageThrottleConfig());
    printer.end();
    return stream;
}

// ----------------------------
// class ClusterProxyDefinition
// ----------------------------

// CONSTANTS

const char ClusterProxyDefinition::CLASS_NAME[] = "ClusterProxyDefinition";

const bdlat_AttributeInfo ClusterProxyDefinition::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_NODES,
     "nodes",
     sizeof("nodes") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_QUEUE_OPERATIONS,
     "queueOperations",
     sizeof("queueOperations") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_CLUSTER_MONITOR_CONFIG,
     "clusterMonitorConfig",
     sizeof("clusterMonitorConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_MESSAGE_THROTTLE_CONFIG,
     "messageThrottleConfig",
     sizeof("messageThrottleConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClusterProxyDefinition::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClusterProxyDefinition::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClusterProxyDefinition::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_NODES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NODES];
    case ATTRIBUTE_ID_QUEUE_OPERATIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_OPERATIONS];
    case ATTRIBUTE_ID_CLUSTER_MONITOR_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CLUSTER_MONITOR_CONFIG];
    case ATTRIBUTE_ID_MESSAGE_THROTTLE_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_THROTTLE_CONFIG];
    default: return 0;
    }
}

// CREATORS

ClusterProxyDefinition::ClusterProxyDefinition(
    bslma::Allocator* basicAllocator)
: d_nodes(basicAllocator)
, d_name(basicAllocator)
, d_queueOperations()
, d_messageThrottleConfig()
, d_clusterMonitorConfig()
{
}

ClusterProxyDefinition::ClusterProxyDefinition(
    const ClusterProxyDefinition& original,
    bslma::Allocator*             basicAllocator)
: d_nodes(original.d_nodes, basicAllocator)
, d_name(original.d_name, basicAllocator)
, d_queueOperations(original.d_queueOperations)
, d_messageThrottleConfig(original.d_messageThrottleConfig)
, d_clusterMonitorConfig(original.d_clusterMonitorConfig)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterProxyDefinition::ClusterProxyDefinition(
    ClusterProxyDefinition&& original) noexcept
: d_nodes(bsl::move(original.d_nodes)),
  d_name(bsl::move(original.d_name)),
  d_queueOperations(bsl::move(original.d_queueOperations)),
  d_messageThrottleConfig(bsl::move(original.d_messageThrottleConfig)),
  d_clusterMonitorConfig(bsl::move(original.d_clusterMonitorConfig))
{
}

ClusterProxyDefinition::ClusterProxyDefinition(
    ClusterProxyDefinition&& original,
    bslma::Allocator*        basicAllocator)
: d_nodes(bsl::move(original.d_nodes), basicAllocator)
, d_name(bsl::move(original.d_name), basicAllocator)
, d_queueOperations(bsl::move(original.d_queueOperations))
, d_messageThrottleConfig(bsl::move(original.d_messageThrottleConfig))
, d_clusterMonitorConfig(bsl::move(original.d_clusterMonitorConfig))
{
}
#endif

ClusterProxyDefinition::~ClusterProxyDefinition()
{
}

// MANIPULATORS

ClusterProxyDefinition&
ClusterProxyDefinition::operator=(const ClusterProxyDefinition& rhs)
{
    if (this != &rhs) {
        d_name                  = rhs.d_name;
        d_nodes                 = rhs.d_nodes;
        d_queueOperations       = rhs.d_queueOperations;
        d_clusterMonitorConfig  = rhs.d_clusterMonitorConfig;
        d_messageThrottleConfig = rhs.d_messageThrottleConfig;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClusterProxyDefinition&
ClusterProxyDefinition::operator=(ClusterProxyDefinition&& rhs)
{
    if (this != &rhs) {
        d_name                  = bsl::move(rhs.d_name);
        d_nodes                 = bsl::move(rhs.d_nodes);
        d_queueOperations       = bsl::move(rhs.d_queueOperations);
        d_clusterMonitorConfig  = bsl::move(rhs.d_clusterMonitorConfig);
        d_messageThrottleConfig = bsl::move(rhs.d_messageThrottleConfig);
    }

    return *this;
}
#endif

void ClusterProxyDefinition::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_nodes);
    bdlat_ValueTypeFunctions::reset(&d_queueOperations);
    bdlat_ValueTypeFunctions::reset(&d_clusterMonitorConfig);
    bdlat_ValueTypeFunctions::reset(&d_messageThrottleConfig);
}

// ACCESSORS

bsl::ostream& ClusterProxyDefinition::print(bsl::ostream& stream,
                                            int           level,
                                            int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("nodes", this->nodes());
    printer.printAttribute("queueOperations", this->queueOperations());
    printer.printAttribute("clusterMonitorConfig",
                           this->clusterMonitorConfig());
    printer.printAttribute("messageThrottleConfig",
                           this->messageThrottleConfig());
    printer.end();
    return stream;
}

// -----------------
// class StatsConfig
// -----------------

// CONSTANTS

const char StatsConfig::CLASS_NAME[] = "StatsConfig";

const int StatsConfig::DEFAULT_INITIALIZER_SNAPSHOT_INTERVAL = 1;

const bdlat_AttributeInfo StatsConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_SNAPSHOT_INTERVAL,
     "snapshotInterval",
     sizeof("snapshotInterval") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_APP_ID_TAG_DOMAINS,
     "appIdTagDomains",
     sizeof("appIdTagDomains") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_PLUGINS,
     "plugins",
     sizeof("plugins") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PRINTER,
     "printer",
     sizeof("printer") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* StatsConfig::lookupAttributeInfo(const char* name,
                                                            int nameLength)
{
    for (int i = 0; i < 4; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StatsConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StatsConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_SNAPSHOT_INTERVAL:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SNAPSHOT_INTERVAL];
    case ATTRIBUTE_ID_APP_ID_TAG_DOMAINS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_ID_TAG_DOMAINS];
    case ATTRIBUTE_ID_PLUGINS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PLUGINS];
    case ATTRIBUTE_ID_PRINTER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PRINTER];
    default: return 0;
    }
}

// CREATORS

StatsConfig::StatsConfig(bslma::Allocator* basicAllocator)
: d_appIdTagDomains(basicAllocator)
, d_plugins(basicAllocator)
, d_printer(basicAllocator)
, d_snapshotInterval(DEFAULT_INITIALIZER_SNAPSHOT_INTERVAL)
{
}

StatsConfig::StatsConfig(const StatsConfig& original,
                         bslma::Allocator*  basicAllocator)
: d_appIdTagDomains(original.d_appIdTagDomains, basicAllocator)
, d_plugins(original.d_plugins, basicAllocator)
, d_printer(original.d_printer, basicAllocator)
, d_snapshotInterval(original.d_snapshotInterval)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatsConfig::StatsConfig(StatsConfig&& original) noexcept
: d_appIdTagDomains(bsl::move(original.d_appIdTagDomains)),
  d_plugins(bsl::move(original.d_plugins)),
  d_printer(bsl::move(original.d_printer)),
  d_snapshotInterval(bsl::move(original.d_snapshotInterval))
{
}

StatsConfig::StatsConfig(StatsConfig&&     original,
                         bslma::Allocator* basicAllocator)
: d_appIdTagDomains(bsl::move(original.d_appIdTagDomains), basicAllocator)
, d_plugins(bsl::move(original.d_plugins), basicAllocator)
, d_printer(bsl::move(original.d_printer), basicAllocator)
, d_snapshotInterval(bsl::move(original.d_snapshotInterval))
{
}
#endif

StatsConfig::~StatsConfig()
{
}

// MANIPULATORS

StatsConfig& StatsConfig::operator=(const StatsConfig& rhs)
{
    if (this != &rhs) {
        d_snapshotInterval = rhs.d_snapshotInterval;
        d_appIdTagDomains  = rhs.d_appIdTagDomains;
        d_plugins          = rhs.d_plugins;
        d_printer          = rhs.d_printer;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatsConfig& StatsConfig::operator=(StatsConfig&& rhs)
{
    if (this != &rhs) {
        d_snapshotInterval = bsl::move(rhs.d_snapshotInterval);
        d_appIdTagDomains  = bsl::move(rhs.d_appIdTagDomains);
        d_plugins          = bsl::move(rhs.d_plugins);
        d_printer          = bsl::move(rhs.d_printer);
    }

    return *this;
}
#endif

void StatsConfig::reset()
{
    d_snapshotInterval = DEFAULT_INITIALIZER_SNAPSHOT_INTERVAL;
    bdlat_ValueTypeFunctions::reset(&d_appIdTagDomains);
    bdlat_ValueTypeFunctions::reset(&d_plugins);
    bdlat_ValueTypeFunctions::reset(&d_printer);
}

// ACCESSORS

bsl::ostream&
StatsConfig::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("snapshotInterval", this->snapshotInterval());
    printer.printAttribute("appIdTagDomains", this->appIdTagDomains());
    printer.printAttribute("plugins", this->plugins());
    printer.printAttribute("printer", this->printer());
    printer.end();
    return stream;
}

// ---------------
// class AppConfig
// ---------------

// CONSTANTS

const char AppConfig::CLASS_NAME[] = "AppConfig";

const char AppConfig::DEFAULT_INITIALIZER_LATENCY_MONITOR_DOMAIN[] =
    "bmq.sys.latemon.latency";

const bool AppConfig::DEFAULT_INITIALIZER_CONFIGURE_STREAM = false;

const bool AppConfig::DEFAULT_INITIALIZER_ADVERTISE_SUBSCRIPTIONS = false;

const bdlat_AttributeInfo AppConfig::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_BROKER_INSTANCE_NAME,
     "brokerInstanceName",
     sizeof("brokerInstanceName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_BROKER_VERSION,
     "brokerVersion",
     sizeof("brokerVersion") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CONFIG_VERSION,
     "configVersion",
     sizeof("configVersion") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_ETC_DIR,
     "etcDir",
     sizeof("etcDir") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_HOST_NAME,
     "hostName",
     sizeof("hostName") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_HOST_TAGS,
     "hostTags",
     sizeof("hostTags") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_HOST_DATA_CENTER,
     "hostDataCenter",
     sizeof("hostDataCenter") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_IS_RUNNING_ON_DEV,
     "isRunningOnDev",
     sizeof("isRunningOnDev") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_LOGS_OBSERVER_MAX_SIZE,
     "logsObserverMaxSize",
     sizeof("logsObserverMaxSize") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_LATENCY_MONITOR_DOMAIN,
     "latencyMonitorDomain",
     sizeof("latencyMonitorDomain") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_DISPATCHER_CONFIG,
     "dispatcherConfig",
     sizeof("dispatcherConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_STATS,
     "stats",
     sizeof("stats") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_NETWORK_INTERFACES,
     "networkInterfaces",
     sizeof("networkInterfaces") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_BMQCONF_CONFIG,
     "bmqconfConfig",
     sizeof("bmqconfConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PLUGINS,
     "plugins",
     sizeof("plugins") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_MESSAGE_PROPERTIES_V2,
     "messagePropertiesV2",
     sizeof("messagePropertiesV2") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_CONFIGURE_STREAM,
     "configureStream",
     sizeof("configureStream") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_ADVERTISE_SUBSCRIPTIONS,
     "advertiseSubscriptions",
     sizeof("advertiseSubscriptions") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo* AppConfig::lookupAttributeInfo(const char* name,
                                                          int nameLength)
{
    for (int i = 0; i < 18; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            AppConfig::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* AppConfig::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_BROKER_INSTANCE_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_INSTANCE_NAME];
    case ATTRIBUTE_ID_BROKER_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER_VERSION];
    case ATTRIBUTE_ID_CONFIG_VERSION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIG_VERSION];
    case ATTRIBUTE_ID_ETC_DIR:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ETC_DIR];
    case ATTRIBUTE_ID_HOST_NAME:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_NAME];
    case ATTRIBUTE_ID_HOST_TAGS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_TAGS];
    case ATTRIBUTE_ID_HOST_DATA_CENTER:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HOST_DATA_CENTER];
    case ATTRIBUTE_ID_IS_RUNNING_ON_DEV:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_IS_RUNNING_ON_DEV];
    case ATTRIBUTE_ID_LOGS_OBSERVER_MAX_SIZE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOGS_OBSERVER_MAX_SIZE];
    case ATTRIBUTE_ID_LATENCY_MONITOR_DOMAIN:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY_MONITOR_DOMAIN];
    case ATTRIBUTE_ID_DISPATCHER_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DISPATCHER_CONFIG];
    case ATTRIBUTE_ID_STATS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STATS];
    case ATTRIBUTE_ID_NETWORK_INTERFACES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NETWORK_INTERFACES];
    case ATTRIBUTE_ID_BMQCONF_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BMQCONF_CONFIG];
    case ATTRIBUTE_ID_PLUGINS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PLUGINS];
    case ATTRIBUTE_ID_MESSAGE_PROPERTIES_V2:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES_V2];
    case ATTRIBUTE_ID_CONFIGURE_STREAM:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURE_STREAM];
    case ATTRIBUTE_ID_ADVERTISE_SUBSCRIPTIONS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ADVERTISE_SUBSCRIPTIONS];
    default: return 0;
    }
}

// CREATORS

AppConfig::AppConfig(bslma::Allocator* basicAllocator)
: d_brokerInstanceName(basicAllocator)
, d_etcDir(basicAllocator)
, d_hostName(basicAllocator)
, d_hostTags(basicAllocator)
, d_hostDataCenter(basicAllocator)
, d_latencyMonitorDomain(DEFAULT_INITIALIZER_LATENCY_MONITOR_DOMAIN,
                         basicAllocator)
, d_stats(basicAllocator)
, d_plugins(basicAllocator)
, d_networkInterfaces(basicAllocator)
, d_messagePropertiesV2()
, d_dispatcherConfig()
, d_bmqconfConfig()
, d_brokerVersion()
, d_configVersion()
, d_logsObserverMaxSize()
, d_isRunningOnDev()
, d_configureStream(DEFAULT_INITIALIZER_CONFIGURE_STREAM)
, d_advertiseSubscriptions(DEFAULT_INITIALIZER_ADVERTISE_SUBSCRIPTIONS)
{
}

AppConfig::AppConfig(const AppConfig&  original,
                     bslma::Allocator* basicAllocator)
: d_brokerInstanceName(original.d_brokerInstanceName, basicAllocator)
, d_etcDir(original.d_etcDir, basicAllocator)
, d_hostName(original.d_hostName, basicAllocator)
, d_hostTags(original.d_hostTags, basicAllocator)
, d_hostDataCenter(original.d_hostDataCenter, basicAllocator)
, d_latencyMonitorDomain(original.d_latencyMonitorDomain, basicAllocator)
, d_stats(original.d_stats, basicAllocator)
, d_plugins(original.d_plugins, basicAllocator)
, d_networkInterfaces(original.d_networkInterfaces, basicAllocator)
, d_messagePropertiesV2(original.d_messagePropertiesV2)
, d_dispatcherConfig(original.d_dispatcherConfig)
, d_bmqconfConfig(original.d_bmqconfConfig)
, d_brokerVersion(original.d_brokerVersion)
, d_configVersion(original.d_configVersion)
, d_logsObserverMaxSize(original.d_logsObserverMaxSize)
, d_isRunningOnDev(original.d_isRunningOnDev)
, d_configureStream(original.d_configureStream)
, d_advertiseSubscriptions(original.d_advertiseSubscriptions)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AppConfig::AppConfig(AppConfig&& original) noexcept
: d_brokerInstanceName(bsl::move(original.d_brokerInstanceName)),
  d_etcDir(bsl::move(original.d_etcDir)),
  d_hostName(bsl::move(original.d_hostName)),
  d_hostTags(bsl::move(original.d_hostTags)),
  d_hostDataCenter(bsl::move(original.d_hostDataCenter)),
  d_latencyMonitorDomain(bsl::move(original.d_latencyMonitorDomain)),
  d_stats(bsl::move(original.d_stats)),
  d_plugins(bsl::move(original.d_plugins)),
  d_networkInterfaces(bsl::move(original.d_networkInterfaces)),
  d_messagePropertiesV2(bsl::move(original.d_messagePropertiesV2)),
  d_dispatcherConfig(bsl::move(original.d_dispatcherConfig)),
  d_bmqconfConfig(bsl::move(original.d_bmqconfConfig)),
  d_brokerVersion(bsl::move(original.d_brokerVersion)),
  d_configVersion(bsl::move(original.d_configVersion)),
  d_logsObserverMaxSize(bsl::move(original.d_logsObserverMaxSize)),
  d_isRunningOnDev(bsl::move(original.d_isRunningOnDev)),
  d_configureStream(bsl::move(original.d_configureStream)),
  d_advertiseSubscriptions(bsl::move(original.d_advertiseSubscriptions))
{
}

AppConfig::AppConfig(AppConfig&& original, bslma::Allocator* basicAllocator)
: d_brokerInstanceName(bsl::move(original.d_brokerInstanceName),
                       basicAllocator)
, d_etcDir(bsl::move(original.d_etcDir), basicAllocator)
, d_hostName(bsl::move(original.d_hostName), basicAllocator)
, d_hostTags(bsl::move(original.d_hostTags), basicAllocator)
, d_hostDataCenter(bsl::move(original.d_hostDataCenter), basicAllocator)
, d_latencyMonitorDomain(bsl::move(original.d_latencyMonitorDomain),
                         basicAllocator)
, d_stats(bsl::move(original.d_stats), basicAllocator)
, d_plugins(bsl::move(original.d_plugins), basicAllocator)
, d_networkInterfaces(bsl::move(original.d_networkInterfaces), basicAllocator)
, d_messagePropertiesV2(bsl::move(original.d_messagePropertiesV2))
, d_dispatcherConfig(bsl::move(original.d_dispatcherConfig))
, d_bmqconfConfig(bsl::move(original.d_bmqconfConfig))
, d_brokerVersion(bsl::move(original.d_brokerVersion))
, d_configVersion(bsl::move(original.d_configVersion))
, d_logsObserverMaxSize(bsl::move(original.d_logsObserverMaxSize))
, d_isRunningOnDev(bsl::move(original.d_isRunningOnDev))
, d_configureStream(bsl::move(original.d_configureStream))
, d_advertiseSubscriptions(bsl::move(original.d_advertiseSubscriptions))
{
}
#endif

AppConfig::~AppConfig()
{
}

// MANIPULATORS

AppConfig& AppConfig::operator=(const AppConfig& rhs)
{
    if (this != &rhs) {
        d_brokerInstanceName     = rhs.d_brokerInstanceName;
        d_brokerVersion          = rhs.d_brokerVersion;
        d_configVersion          = rhs.d_configVersion;
        d_etcDir                 = rhs.d_etcDir;
        d_hostName               = rhs.d_hostName;
        d_hostTags               = rhs.d_hostTags;
        d_hostDataCenter         = rhs.d_hostDataCenter;
        d_isRunningOnDev         = rhs.d_isRunningOnDev;
        d_logsObserverMaxSize    = rhs.d_logsObserverMaxSize;
        d_latencyMonitorDomain   = rhs.d_latencyMonitorDomain;
        d_dispatcherConfig       = rhs.d_dispatcherConfig;
        d_stats                  = rhs.d_stats;
        d_networkInterfaces      = rhs.d_networkInterfaces;
        d_bmqconfConfig          = rhs.d_bmqconfConfig;
        d_plugins                = rhs.d_plugins;
        d_messagePropertiesV2    = rhs.d_messagePropertiesV2;
        d_configureStream        = rhs.d_configureStream;
        d_advertiseSubscriptions = rhs.d_advertiseSubscriptions;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
AppConfig& AppConfig::operator=(AppConfig&& rhs)
{
    if (this != &rhs) {
        d_brokerInstanceName     = bsl::move(rhs.d_brokerInstanceName);
        d_brokerVersion          = bsl::move(rhs.d_brokerVersion);
        d_configVersion          = bsl::move(rhs.d_configVersion);
        d_etcDir                 = bsl::move(rhs.d_etcDir);
        d_hostName               = bsl::move(rhs.d_hostName);
        d_hostTags               = bsl::move(rhs.d_hostTags);
        d_hostDataCenter         = bsl::move(rhs.d_hostDataCenter);
        d_isRunningOnDev         = bsl::move(rhs.d_isRunningOnDev);
        d_logsObserverMaxSize    = bsl::move(rhs.d_logsObserverMaxSize);
        d_latencyMonitorDomain   = bsl::move(rhs.d_latencyMonitorDomain);
        d_dispatcherConfig       = bsl::move(rhs.d_dispatcherConfig);
        d_stats                  = bsl::move(rhs.d_stats);
        d_networkInterfaces      = bsl::move(rhs.d_networkInterfaces);
        d_bmqconfConfig          = bsl::move(rhs.d_bmqconfConfig);
        d_plugins                = bsl::move(rhs.d_plugins);
        d_messagePropertiesV2    = bsl::move(rhs.d_messagePropertiesV2);
        d_configureStream        = bsl::move(rhs.d_configureStream);
        d_advertiseSubscriptions = bsl::move(rhs.d_advertiseSubscriptions);
    }

    return *this;
}
#endif

void AppConfig::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_brokerInstanceName);
    bdlat_ValueTypeFunctions::reset(&d_brokerVersion);
    bdlat_ValueTypeFunctions::reset(&d_configVersion);
    bdlat_ValueTypeFunctions::reset(&d_etcDir);
    bdlat_ValueTypeFunctions::reset(&d_hostName);
    bdlat_ValueTypeFunctions::reset(&d_hostTags);
    bdlat_ValueTypeFunctions::reset(&d_hostDataCenter);
    bdlat_ValueTypeFunctions::reset(&d_isRunningOnDev);
    bdlat_ValueTypeFunctions::reset(&d_logsObserverMaxSize);
    d_latencyMonitorDomain = DEFAULT_INITIALIZER_LATENCY_MONITOR_DOMAIN;
    bdlat_ValueTypeFunctions::reset(&d_dispatcherConfig);
    bdlat_ValueTypeFunctions::reset(&d_stats);
    bdlat_ValueTypeFunctions::reset(&d_networkInterfaces);
    bdlat_ValueTypeFunctions::reset(&d_bmqconfConfig);
    bdlat_ValueTypeFunctions::reset(&d_plugins);
    bdlat_ValueTypeFunctions::reset(&d_messagePropertiesV2);
    d_configureStream        = DEFAULT_INITIALIZER_CONFIGURE_STREAM;
    d_advertiseSubscriptions = DEFAULT_INITIALIZER_ADVERTISE_SUBSCRIPTIONS;
}

// ACCESSORS

bsl::ostream&
AppConfig::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("brokerInstanceName", this->brokerInstanceName());
    printer.printAttribute("brokerVersion", this->brokerVersion());
    printer.printAttribute("configVersion", this->configVersion());
    printer.printAttribute("etcDir", this->etcDir());
    printer.printAttribute("hostName", this->hostName());
    printer.printAttribute("hostTags", this->hostTags());
    printer.printAttribute("hostDataCenter", this->hostDataCenter());
    printer.printAttribute("isRunningOnDev", this->isRunningOnDev());
    printer.printAttribute("logsObserverMaxSize", this->logsObserverMaxSize());
    printer.printAttribute("latencyMonitorDomain",
                           this->latencyMonitorDomain());
    printer.printAttribute("dispatcherConfig", this->dispatcherConfig());
    printer.printAttribute("stats", this->stats());
    printer.printAttribute("networkInterfaces", this->networkInterfaces());
    printer.printAttribute("bmqconfConfig", this->bmqconfConfig());
    printer.printAttribute("plugins", this->plugins());
    printer.printAttribute("messagePropertiesV2", this->messagePropertiesV2());
    printer.printAttribute("configureStream", this->configureStream());
    printer.printAttribute("advertiseSubscriptions",
                           this->advertiseSubscriptions());
    printer.end();
    return stream;
}

// ------------------------
// class ClustersDefinition
// ------------------------

// CONSTANTS

const char ClustersDefinition::CLASS_NAME[] = "ClustersDefinition";

const bdlat_AttributeInfo ClustersDefinition::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_MY_CLUSTERS,
     "myClusters",
     sizeof("myClusters") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_MY_REVERSE_CLUSTERS,
     "myReverseClusters",
     sizeof("myReverseClusters") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_MY_VIRTUAL_CLUSTERS,
     "myVirtualClusters",
     sizeof("myVirtualClusters") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_PROXY_CLUSTERS,
     "proxyClusters",
     sizeof("proxyClusters") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_REVERSED_CLUSTER_CONNECTIONS,
     "reversedClusterConnections",
     sizeof("reversedClusterConnections") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
ClustersDefinition::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 5; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            ClustersDefinition::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* ClustersDefinition::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_MY_CLUSTERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_CLUSTERS];
    case ATTRIBUTE_ID_MY_REVERSE_CLUSTERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_REVERSE_CLUSTERS];
    case ATTRIBUTE_ID_MY_VIRTUAL_CLUSTERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MY_VIRTUAL_CLUSTERS];
    case ATTRIBUTE_ID_PROXY_CLUSTERS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PROXY_CLUSTERS];
    case ATTRIBUTE_ID_REVERSED_CLUSTER_CONNECTIONS:
        return &ATTRIBUTE_INFO_ARRAY
            [ATTRIBUTE_INDEX_REVERSED_CLUSTER_CONNECTIONS];
    default: return 0;
    }
}

// CREATORS

ClustersDefinition::ClustersDefinition(bslma::Allocator* basicAllocator)
: d_myReverseClusters(basicAllocator)
, d_myVirtualClusters(basicAllocator)
, d_reversedClusterConnections(basicAllocator)
, d_proxyClusters(basicAllocator)
, d_myClusters(basicAllocator)
{
}

ClustersDefinition::ClustersDefinition(const ClustersDefinition& original,
                                       bslma::Allocator* basicAllocator)
: d_myReverseClusters(original.d_myReverseClusters, basicAllocator)
, d_myVirtualClusters(original.d_myVirtualClusters, basicAllocator)
, d_reversedClusterConnections(original.d_reversedClusterConnections,
                               basicAllocator)
, d_proxyClusters(original.d_proxyClusters, basicAllocator)
, d_myClusters(original.d_myClusters, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClustersDefinition::ClustersDefinition(ClustersDefinition&& original) noexcept
: d_myReverseClusters(bsl::move(original.d_myReverseClusters)),
  d_myVirtualClusters(bsl::move(original.d_myVirtualClusters)),
  d_reversedClusterConnections(
      bsl::move(original.d_reversedClusterConnections)),
  d_proxyClusters(bsl::move(original.d_proxyClusters)),
  d_myClusters(bsl::move(original.d_myClusters))
{
}

ClustersDefinition::ClustersDefinition(ClustersDefinition&& original,
                                       bslma::Allocator*    basicAllocator)
: d_myReverseClusters(bsl::move(original.d_myReverseClusters), basicAllocator)
, d_myVirtualClusters(bsl::move(original.d_myVirtualClusters), basicAllocator)
, d_reversedClusterConnections(
      bsl::move(original.d_reversedClusterConnections),
      basicAllocator)
, d_proxyClusters(bsl::move(original.d_proxyClusters), basicAllocator)
, d_myClusters(bsl::move(original.d_myClusters), basicAllocator)
{
}
#endif

ClustersDefinition::~ClustersDefinition()
{
}

// MANIPULATORS

ClustersDefinition&
ClustersDefinition::operator=(const ClustersDefinition& rhs)
{
    if (this != &rhs) {
        d_myClusters                 = rhs.d_myClusters;
        d_myReverseClusters          = rhs.d_myReverseClusters;
        d_myVirtualClusters          = rhs.d_myVirtualClusters;
        d_proxyClusters              = rhs.d_proxyClusters;
        d_reversedClusterConnections = rhs.d_reversedClusterConnections;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
ClustersDefinition& ClustersDefinition::operator=(ClustersDefinition&& rhs)
{
    if (this != &rhs) {
        d_myClusters                 = bsl::move(rhs.d_myClusters);
        d_myReverseClusters          = bsl::move(rhs.d_myReverseClusters);
        d_myVirtualClusters          = bsl::move(rhs.d_myVirtualClusters);
        d_proxyClusters              = bsl::move(rhs.d_proxyClusters);
        d_reversedClusterConnections = bsl::move(
            rhs.d_reversedClusterConnections);
    }

    return *this;
}
#endif

void ClustersDefinition::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_myClusters);
    bdlat_ValueTypeFunctions::reset(&d_myReverseClusters);
    bdlat_ValueTypeFunctions::reset(&d_myVirtualClusters);
    bdlat_ValueTypeFunctions::reset(&d_proxyClusters);
    bdlat_ValueTypeFunctions::reset(&d_reversedClusterConnections);
}

// ACCESSORS

bsl::ostream& ClustersDefinition::print(bsl::ostream& stream,
                                        int           level,
                                        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("myClusters", this->myClusters());
    printer.printAttribute("myReverseClusters", this->myReverseClusters());
    printer.printAttribute("myVirtualClusters", this->myVirtualClusters());
    printer.printAttribute("proxyClusters", this->proxyClusters());
    printer.printAttribute("reversedClusterConnections",
                           this->reversedClusterConnections());
    printer.end();
    return stream;
}

// -------------------
// class Configuration
// -------------------

// CONSTANTS

const char Configuration::CLASS_NAME[] = "Configuration";

const bdlat_AttributeInfo Configuration::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_TASK_CONFIG,
     "taskConfig",
     sizeof("taskConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_APP_CONFIG,
     "appConfig",
     sizeof("appConfig") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo* Configuration::lookupAttributeInfo(const char* name,
                                                              int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            Configuration::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* Configuration::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_TASK_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TASK_CONFIG];
    case ATTRIBUTE_ID_APP_CONFIG:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_APP_CONFIG];
    default: return 0;
    }
}

// CREATORS

Configuration::Configuration(bslma::Allocator* basicAllocator)
: d_taskConfig(basicAllocator)
, d_appConfig(basicAllocator)
{
}

Configuration::Configuration(const Configuration& original,
                             bslma::Allocator*    basicAllocator)
: d_taskConfig(original.d_taskConfig, basicAllocator)
, d_appConfig(original.d_appConfig, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Configuration::Configuration(Configuration&& original) noexcept
: d_taskConfig(bsl::move(original.d_taskConfig)),
  d_appConfig(bsl::move(original.d_appConfig))
{
}

Configuration::Configuration(Configuration&&   original,
                             bslma::Allocator* basicAllocator)
: d_taskConfig(bsl::move(original.d_taskConfig), basicAllocator)
, d_appConfig(bsl::move(original.d_appConfig), basicAllocator)
{
}
#endif

Configuration::~Configuration()
{
}

// MANIPULATORS

Configuration& Configuration::operator=(const Configuration& rhs)
{
    if (this != &rhs) {
        d_taskConfig = rhs.d_taskConfig;
        d_appConfig  = rhs.d_appConfig;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
Configuration& Configuration::operator=(Configuration&& rhs)
{
    if (this != &rhs) {
        d_taskConfig = bsl::move(rhs.d_taskConfig);
        d_appConfig  = bsl::move(rhs.d_appConfig);
    }

    return *this;
}
#endif

void Configuration::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_taskConfig);
    bdlat_ValueTypeFunctions::reset(&d_appConfig);
}

// ACCESSORS

bsl::ostream&
Configuration::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("taskConfig", this->taskConfig());
    printer.printAttribute("appConfig", this->appConfig());
    printer.end();
    return stream;
}

}  // close package namespace
}  // close enterprise namespace

// GENERATED BY BLP_BAS_CODEGEN_2024.07.04.1
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mqbcfg --msgComponent messages mqbcfg.xsd
// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright 2024 Bloomberg Finance L.P. All rights reserved.
//      Property of Bloomberg Finance L.P. (BFLP)
//      This software is made available solely pursuant to the
//      terms of a BFLP license agreement which governs its use.
// ------------------------------- END-OF-FILE --------------------------------
