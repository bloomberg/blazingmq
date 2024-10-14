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

// bmqio_listenoptions.cpp                                            -*-C++-*-
#include <bmqio_listenoptions.h>

#include <bmqscm_version.h>
// BDE
#include <bsl_ostream.h>
#include <bslim_printer.h>

namespace BloombergLP {
namespace bmqio {

// -------------------
// class ListenOptions
// -------------------

// MANIPULATORS
ListenOptions& ListenOptions::operator=(const ListenOptions& rhs)
{
    if (this != &rhs) {
        d_endpoint   = rhs.d_endpoint;
        d_properties = rhs.d_properties;
    }

    return *this;
}

// ACCESSORS
bsl::ostream&
ListenOptions::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("Endpoint", d_endpoint);
    printer.printAttribute("Properties", d_properties);
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
