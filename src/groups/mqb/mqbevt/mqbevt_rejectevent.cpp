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

// mqbevt_rejectevent.cpp                                             -*-C++-*-
#include <mqbevt_rejectevent.h>

#include <mqbscm_version.h>

// BDE
#include <bslim_printer.h>

namespace BloombergLP {
namespace mqbevt {

// -----------------
// class RejectEvent
// -----------------

RejectEvent::RejectEvent(bslma::Allocator* allocator)
: mqbi::DispatcherEvent(allocator)
, d_blob_sp()
, d_clusterNode_p(0)
, d_rejectMessage()
, d_isRelay(false)
, d_partitionId(-1)
{
    // NOTHING
}

RejectEvent::~RejectEvent()
{
    // NOTHING
}

void RejectEvent::reset()
{
    d_blob_sp.reset();
    d_clusterNode_p = 0;
    d_rejectMessage = bmqp::RejectMessage();
    d_isRelay       = false;
    d_partitionId   = -1;
    mqbi::DispatcherEvent::reset();
}

bsl::ostream&
RejectEvent::print(bsl::ostream& stream, int level, int spacesPerLevel) const
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
    printer.printAttribute("partitionId", d_partitionId);
    printer.printAttribute("rejectMessage.queueId", d_rejectMessage.queueId());
    printer.printAttribute("rejectMessage.subQueueId",
                           d_rejectMessage.subQueueId());
    printer.printAttribute("rejectMessage.messageGUID",
                           d_rejectMessage.messageGUID());

    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
