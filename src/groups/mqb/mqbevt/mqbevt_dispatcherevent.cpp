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

// mqbevt_dispatcherevent.cpp                                         -*-C++-*-
#include <mqbevt_dispatcherevent.h>

#include <mqbscm_version.h>

// BDE
#include <bslim_printer.h>

namespace BloombergLP {
namespace mqbevt {

// ---------------------
// class DispatcherEvent
// ---------------------

// CREATORS
DispatcherEvent::DispatcherEvent(bslma::Allocator* allocator)
: mqbi::DispatcherEvent(allocator)
, d_callback(allocator)
, d_finalizeCallback(allocator)
{
    // NOTHING
}

DispatcherEvent::~DispatcherEvent()
{
    // NOTHING
}

// MANIPULATORS
void DispatcherEvent::reset()
{
    if (!d_finalizeCallback.empty()) {
        d_finalizeCallback();
    }

    d_callback.reset();
    d_finalizeCallback.reset();

    mqbi::DispatcherEvent::reset();
}

// ACCESSORS
bsl::ostream& DispatcherEvent::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
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
    printer.printAttribute("hasFinalizeCallback",
                           (d_finalizeCallback.empty() ? "no" : "yes"));

    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
