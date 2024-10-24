// Copyright 2019-2023 Bloomberg Finance L.P.
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

// bmqc_monitoredqueue.cpp                                            -*-C++-*-

// Include guards in cpp needed by Sun compiler.
#include <bmqscm_version.h>
// See {internal-ticket D19728733}
#ifndef INCLUDED_BMQC_MONITOREDQUEUE_CPP
#define INCLUDED_BMQC_MONITOREDQUEUE_CPP

#include <bmqc_monitoredqueue.h>

#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>

// BDE
#include <ball_log.h>
#include <bdlb_print.h>

namespace BloombergLP {
namespace bmqc {

namespace {
const char k_LOG_CATEGORY[] = "BMQC.MONITOREDQUEUE";
}  // close unnamed namespace

// --------------------------
// struct MonitoredQueueState
// --------------------------

// CLASS METHODS
bsl::ostream& MonitoredQueueState::print(bsl::ostream&             stream,
                                         MonitoredQueueState::Enum value,
                                         int                       level,
                                         int spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << MonitoredQueueState::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* MonitoredQueueState::toAscii(MonitoredQueueState::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(NORMAL)
        CASE(HIGH_WATERMARK_REACHED)
        CASE(HIGH_WATERMARK_2_REACHED)
        CASE(QUEUE_FILLED)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// -------------------------
// struct MonitoredQueueUtil
// -------------------------

// CLASS METHODS
void MonitoredQueueUtil::stateLogCallback(const bsl::string& queueName,
                                          const bsl::string& warningString,
                                          bsls::Types::Int64 lowWatermark,
                                          bsls::Types::Int64 highWatermark,
                                          bsls::Types::Int64 highWatermark2,
                                          bsls::Types::Int64 queueSize,
                                          MonitoredQueueState::Enum state)
{
    BALL_LOG_SET_CATEGORY(k_LOG_CATEGORY);

    bmqu::MemOutStream buffer;
    buffer << queueName << " [Watermarks: low="
           << bmqu::PrintUtil::prettyNumber(lowWatermark)
           << ", high=" << bmqu::PrintUtil::prettyNumber(highWatermark)
           << ", high2=" << bmqu::PrintUtil::prettyNumber(highWatermark2)
           << ", limit=" << bmqu::PrintUtil::prettyNumber(queueSize) << "]";

    switch (state) {
    case MonitoredQueueState::e_NORMAL: {
        BALL_LOG_INFO << buffer.str() << " has reached its low-watermark";
    } break;
    case MonitoredQueueState::e_HIGH_WATERMARK_REACHED: {
        BALL_LOG_WARN << warningString << " " << buffer.str()
                      << " has reached its high-watermark";
    } break;
    case MonitoredQueueState::e_HIGH_WATERMARK_2_REACHED: {
        BALL_LOG_WARN << warningString << " " << buffer.str()
                      << " has reached its high-watermark2";
    } break;
    case bmqc::MonitoredQueueState::e_QUEUE_FILLED: {
        BALL_LOG_ERROR << warningString << " " << buffer.str() << " is full";
    } break;
    default: {
        BALL_LOG_WARN << warningString << " " << buffer.str()
                      << " has reached an unknown state";
    } break;
    }
}

}  // close package namespace
}  // close enterprise namespace

#endif
