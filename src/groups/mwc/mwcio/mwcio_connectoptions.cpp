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

// mwcio_connectoptions.cpp                                           -*-C++-*-
#include <mwcio_connectoptions.h>

#include <mwcscm_version.h>
// BDE
#include <bsl_ostream.h>
#include <bslim_printer.h>

namespace BloombergLP {
namespace mwcio {

// --------------------
// class ConnectOptions
// --------------------

// MANIPULATORS
ConnectOptions& ConnectOptions::operator=(const ConnectOptions& rhs)
{
    if (this != &rhs) {
        d_endpoint        = rhs.d_endpoint;
        d_numAttempts     = rhs.d_numAttempts;
        d_attemptInterval = rhs.d_attemptInterval;
        d_autoReconnect   = rhs.d_autoReconnect;
        d_properties      = rhs.d_properties;
    }

    return *this;
}

// ACCESSORS
bsl::ostream& ConnectOptions::print(bsl::ostream& stream,
                                    int           level,
                                    int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("Endpoint", d_endpoint);
    printer.printAttribute("NumAttempts", d_numAttempts);
    printer.printAttribute("AttemptInterval", d_attemptInterval);
    printer.printAttribute("AutoReconnect", d_autoReconnect);
    printer.printAttribute("Properties", d_properties);
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
