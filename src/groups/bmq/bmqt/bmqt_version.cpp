// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqt_version.cpp                                                   -*-C++-*-
#include <bmqt_version.h>

#include <bmqscm_version.h>
// BDE
#include <bslim_printer.h>

namespace BloombergLP {
namespace bmqt {

// -------------
// class Version
// -------------

// ACCESSORS
bsl::ostream&
Version::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("major", int(d_major));
    printer.printAttribute("minor", int(d_minor));
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
