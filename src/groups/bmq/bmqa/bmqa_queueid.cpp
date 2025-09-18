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

// bmqa_queueid.cpp                                                   -*-C++-*-
#include <bmqa_queueid.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqimp_queue.h>

// BDE
#include <bsl_ostream.h>
#include <bsla_annotations.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqa {

// -------------
// class QueueId
// -------------

QueueId::QueueId(bslma::Allocator* allocator)
: d_impl_sp()
{
    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    d_impl_sp.createInplace(alloc, alloc);
    d_impl_sp->setCorrelationId(bmqt::CorrelationId::autoValue());
}

QueueId::QueueId(const QueueId& other, BSLA_UNUSED bslma::Allocator* allocator)
: d_impl_sp(other.d_impl_sp)
{
    // NOTHING
}

QueueId::QueueId(const bmqt::CorrelationId& correlationId,
                 bslma::Allocator*          allocator)
: d_impl_sp()
{
    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    d_impl_sp.createInplace(alloc, alloc);
    d_impl_sp->setCorrelationId(correlationId);
}

QueueId::QueueId(bsls::Types::Int64 numeric, bslma::Allocator* allocator)
: d_impl_sp()
{
    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    d_impl_sp.createInplace(alloc, alloc);
    d_impl_sp->setCorrelationId(bmqt::CorrelationId(numeric));
}

QueueId::QueueId(void* pointer, bslma::Allocator* allocator)
: d_impl_sp()
{
    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    d_impl_sp.createInplace(alloc, alloc);
    d_impl_sp->setCorrelationId(bmqt::CorrelationId(pointer));
}

QueueId::QueueId(const bsl::shared_ptr<void>& sharedPtr,
                 bslma::Allocator*            allocator)
: d_impl_sp()
{
    bslma::Allocator* alloc = bslma::Default::allocator(allocator);

    d_impl_sp.createInplace(alloc, alloc);
    d_impl_sp->setCorrelationId(bmqt::CorrelationId(sharedPtr));
}

QueueId& QueueId::operator=(const QueueId& rhs)
{
    d_impl_sp = rhs.d_impl_sp;
    return *this;
}

const bmqt::CorrelationId& QueueId::correlationId() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);

    return d_impl_sp->correlationId();
}

bsls::Types::Uint64 QueueId::flags() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);

    return d_impl_sp->flags();
}

const bmqt::Uri& QueueId::uri() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);

    return d_impl_sp->uri();
}

const bmqt::QueueOptions& QueueId::options() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);

    return d_impl_sp->options();
}

bool QueueId::isValid(bsl::ostream* reason_p) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);

    return d_impl_sp->isValid(reason_p);
}

bsl::ostream&
QueueId::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);

    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("uri", uri());
    printer.printAttribute("correlationId", correlationId());
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
