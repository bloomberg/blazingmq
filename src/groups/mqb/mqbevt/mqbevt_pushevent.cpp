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

// mqbevt_pushevent.cpp                                               -*-C++-*-
#include <mqbevt_pushevent.h>

#include <mqbscm_version.h>

// BDE
#include <bslim_printer.h>

namespace BloombergLP {
namespace mqbevt {

// ---------------
// class PushEvent
// ---------------

PushEvent::PushEvent(bslma::Allocator* allocator)
: mqbi::DispatcherEvent(allocator)
, d_blob_sp()
, d_options_sp()
, d_clusterNode_p(0)
, d_guid()
, d_isRelay(false)
, d_queueId(-1)
, d_subQueueInfos(allocator)
, d_messagePropertiesInfo()
, d_compressionAlgorithmType(bmqt::CompressionAlgorithmType::e_NONE)
, d_isOutOfOrderPush(false)
{
    // NOTHING
}

PushEvent::~PushEvent()
{
    // NOTHING
}

void PushEvent::reset()
{
    d_blob_sp.reset();
    d_options_sp.reset();
    d_clusterNode_p = 0;
    d_guid          = bmqt::MessageGUID();
    d_isRelay       = false;
    d_queueId       = -1;
    d_subQueueInfos.clear();
    d_messagePropertiesInfo    = bmqp::MessagePropertiesInfo();
    d_compressionAlgorithmType = bmqt::CompressionAlgorithmType::e_NONE;
    d_isOutOfOrderPush         = false;
    mqbi::DispatcherEvent::reset();
}

bsl::ostream&
PushEvent::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();

    printer.printAttribute("type", type());
    if (source()) {
        printer.printAttribute("source", source()->description());
    }
    if (destination()) {
        printer.printAttribute("destination", destination()->description());
    }
    printer.printAttribute("isRelay", d_isRelay);
    printer.printAttribute("guid", d_guid);
    printer.printAttribute("queueId", d_queueId);
    printer.printAttribute("compressionAlgorithmType",
                           d_compressionAlgorithmType);
    printer.printAttribute("isOutOfOrderPush", d_isOutOfOrderPush);

    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
