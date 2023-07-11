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

// mwcio_status.cpp                                                   -*-C++-*-
#include <mwcio_status.h>

#include <mwcscm_version.h>
// BDE
#include <bdlb_print.h>
#include <bsl_ostream.h>
#include <bslim_printer.h>

namespace BloombergLP {
namespace mwcio {

// ---------------------
// struct StatusCategory
// ---------------------

bsl::ostream& StatusCategory::print(bsl::ostream&        stream,
                                    StatusCategory::Enum value,
                                    int                  level,
                                    int                  spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << StatusCategory::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* StatusCategory::toAscii(StatusCategory::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(SUCCESS)
        CASE(CONNECTION)
        CASE(TIMEOUT)
        CASE(CANCELED)
        CASE(LIMIT)
        CASE(GENERIC_ERROR)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ------------
// class Status
// ------------

// MANIPULATORS
Status& Status::operator=(const Status& rhs)
{
    if (this != &rhs) {
        d_category   = rhs.d_category;
        d_properties = rhs.d_properties;
    }

    return *this;
}

// ACCESSORS
bsl::ostream&
Status::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printForeign(d_category, &StatusCategory::print, "Category");
    if (!d_properties.isNull()) {
        printer.printAttribute("Properties", d_properties.value());
    }
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
