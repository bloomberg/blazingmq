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

// bmqa_configurequeuestatus.cpp                                      -*-C++-*-
#include <bmqa_configurequeuestatus.h>

#include <bmqscm_version.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bslim_printer.h>

namespace BloombergLP {
namespace bmqa {

// --------------------------
// class ConfigureQueueStatus
// --------------------------

// ACCESSORS
bsl::ostream& ConfigureQueueStatus::print(bsl::ostream& stream,
                                          int           level,
                                          int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("queueId", queueId());
    bmqu::MemOutStream out;
    out << result() << " (" << static_cast<int>(result()) << ")";
    printer.printAttribute("result", out.str());
    if (!errorDescription().empty()) {
        printer.printAttribute("errorDescription", errorDescription());
    }
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
