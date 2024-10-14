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

// bmqp_queueid.cpp                                                   -*-C++-*-
#include <bmqp_queueid.h>

#include <bmqscm_version.h>
// BDE
#include <bsl_limits.h>
#include <bsl_ostream.h>
#include <bslim_printer.h>
#include <bslmf_isfundamental.h>

namespace bsl {

// Both QueueId::QueueIdInt and QueueId::SubQueueIdInt are primitive types and
// to make 'bslim::Printer::print' call 'operator<<' instead of 'print' they
// need to have 'is_fundamental' trait.

template <>
struct is_fundamental<BloombergLP::bmqp::QueueId::QueueIdInt> : true_type {};

template <>
struct is_fundamental<BloombergLP::bmqp::QueueId::SubQueueIdInt> : true_type {
};

}  // close namespace bsl

namespace BloombergLP {
namespace bmqp {

// -------------
// class QueueId
// -------------

const unsigned int QueueId::k_RESERVED_QUEUE_ID;
const unsigned int QueueId::k_PRIMARY_QUEUE_ID;
const unsigned int QueueId::k_UNASSIGNED_QUEUE_ID;

/// Force variable/symbol definition so that it can be used in other files
const unsigned int QueueId::k_RESERVED_SUBQUEUE_ID;

/// Force variable/symbol definition so that it can be used in other files
const unsigned int QueueId::k_UNASSIGNED_SUBQUEUE_ID;

/// Force variable/symbol definition so that it can be used in other files
const unsigned int QueueId::k_DEFAULT_SUBQUEUE_ID;

// FREE OPERATORS
bsl::ostream&
QueueId::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("qId", d_id);
    printer.printAttribute("subId", d_subId);
    printer.end();

    return stream;
}

bsl::ostream& operator<<(bsl::ostream&                    stream,
                         const bmqp::QueueId::QueueIdInt& rhs)
{
    switch (static_cast<unsigned int>(rhs.d_value)) {
    case bmqp::QueueId::k_RESERVED_QUEUE_ID: {
        stream << "RESERVED";
    } break;
    case bmqp::QueueId::k_PRIMARY_QUEUE_ID: {
        stream << "PRIMARY";
    } break;
    case bmqp::QueueId::k_UNASSIGNED_QUEUE_ID: {
        stream << "UNASSIGNED";
    } break;
    default: {
        stream << rhs.d_value;
    }
    };

    return stream;
}

bsl::ostream& operator<<(bsl::ostream&                       stream,
                         const bmqp::QueueId::SubQueueIdInt& rhs)
{
    switch (rhs.d_value) {
    case bmqp::QueueId::k_RESERVED_SUBQUEUE_ID: {
        stream << "RESERVED";
    } break;
    case bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID: {
        stream << "UNASSIGNED";
    } break;
    case bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID: {
        stream << "DEFAULT";
    } break;
    default: {
        stream << rhs.d_value;
    }
    };
    return stream;
}

}  // close package namespace
}  // close enterprise namespace
