// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mwct_rcdescriptionerror.cpp                                        -*-C++-*-
#include <mwct_rcdescriptionerror.h>

#include <mwcscm_version.h>
// BDE
#include <bslim_printer.h>

namespace BloombergLP {
namespace mwct {

// ------------------------
// class RcDescriptionError
// ------------------------

bsl::ostream& RcDescriptionError::print(bsl::ostream&             stream,
                                        const RcDescriptionError& value,
                                        int                       level,
                                        int spacesPerLevel)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();

    printer.printAttribute("rc", value.rc());
    printer.printAttribute("description", value.description());

    printer.end();
    return stream;
}

}  // close package namespace
}  // close enterprise namespace
