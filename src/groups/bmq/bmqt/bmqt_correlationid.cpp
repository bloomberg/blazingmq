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

// bmqt_correlationid.cpp                                             -*-C++-*-
#include <bmqt_correlationid.h>

#include <bmqscm_version.h>
// BDE
#include <bsl_ostream.h>
#include <bslim_printer.h>
#include <bslmt_once.h>
#include <bsls_atomic.h>

namespace BloombergLP {
namespace bmqt {

// -------------------
// class CorrelationId
// -------------------

CorrelationId CorrelationId::autoValue()
{
    static bsls::AtomicInt* g_id_p = 0;  // A unique id for each AutoValue

    BSLMT_ONCE_DO
    {
        g_id_p = new bsls::AtomicInt(0);
        // Heap allocate it to prevent 'exit-time-destructor needed' compiler
        // warning.  Causes valgrind-reported memory leak.
    }

    CorrelationId res;
    res.d_variant.createInPlace<AutoValue>(++(*g_id_p));
    return res;
}

bsl::ostream&
CorrelationId::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();

    switch (type()) {
    case e_POINTER: {
        printer.printAttribute("pointer", thePointer());
    } break;
    case e_NUMERIC: {
        printer.printAttribute("numeric", theNumeric());
    } break;
    case e_SHARED_PTR: {
        printer.printAttribute("sharedPtr", theSharedPtr().get());
    } break;
    case e_AUTO_VALUE: {
        printer.printAttribute("autoValue", theAutoValue());
    } break;
    case e_UNSET: {
        printer.printValue("* unset *");
    } break;
    default: {
        BSLS_ASSERT_OPT(false && "Unknown CorrelationId value type");
    }
    }
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
