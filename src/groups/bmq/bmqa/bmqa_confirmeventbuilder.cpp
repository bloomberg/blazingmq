// Copyright 2016-2023 Bloomberg Finance L.P.
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

// bmqa_confirmeventbuilder.cpp                                       -*-C++-*-
#include <bmqa_confirmeventbuilder.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqimp_queue.h>
#include <bmqp_confirmeventbuilder.h>

// BDE
#include <bsl_memory.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqa {

// Compile-time assert to keep the hardcoded value of size of an object of type
// 'bmqp::ConfirmEventBuilder' in sync with its actual size.  We need to hard
// code the size in 'bmqa_confirmeventbuilder.h' because none of the 'bmqp'
// headers can be included in 'bmqa' headers.  Note that we don't check exact
// size, but 'enough' size, see comment of the constant in header.

BSLMF_ASSERT(ConfirmEventBuilderImpl::k_MAX_SIZEOF_BMQP_CONFIRMEVENTBUILDER >=
             sizeof(bmqp::ConfirmEventBuilder));

// -------------------------
// class ConfirmEventBuilder
// -------------------------

// CREATORS
ConfirmEventBuilder::~ConfirmEventBuilder()
{
    if (d_impl.d_builder_p != 0) {
        d_impl.d_builder_p->bmqp::ConfirmEventBuilder::~ConfirmEventBuilder();
        d_impl.d_builder_p = 0;
    }
}

// MANIPULATORS
void ConfirmEventBuilder::reset()
{
    if (d_impl.d_builder_p) {
        d_impl.d_builder_p->reset();
    }
}

bmqt::EventBuilderResult::Enum ConfirmEventBuilder::addMessageConfirmation(
    const MessageConfirmationCookie& cookie)
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl.d_builder_p);

    const bsl::shared_ptr<bmqimp::Queue>& queue =
        reinterpret_cast<const bsl::shared_ptr<bmqimp::Queue>&>(
            cookie.queueId());

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(!queue->isOpened())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_QUEUE_INVALID;  // RETURN
    }

    // Ensure confirm messages are expected by the queue
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(queue->atMostOnce())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return bmqt::EventBuilderResult::e_SUCCESS;  // RETURN
    }

    return d_impl.d_builder_p->appendMessage(queue->id(),
                                             queue->subQueueId(),
                                             cookie.messageGUID());
}

// ACCESSORS
int ConfirmEventBuilder::messageCount() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl.d_builder_p);

    return d_impl.d_builder_p->messageCount();
}

const bdlbb::Blob& ConfirmEventBuilder::blob() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl.d_builder_p);

    return d_impl.d_builder_p->blob();
}

}  // close package namespace
}  // close enterprise namespace
