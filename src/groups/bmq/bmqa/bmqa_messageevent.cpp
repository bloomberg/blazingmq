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

// bmqa_messageevent.cpp                                              -*-C++-*-
#include <bmqa_messageevent.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqimp_event.h>
#include <bmqp_event.h>

// BDE
#include <bsl_ostream.h>
#include <bsla_annotations.h>
#include <bslmf_assert.h>
#include <bslmf_ispolymorphic.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqa {

namespace {
// Do some compile time validation
BSLMF_ASSERT(sizeof(MessageIterator) == sizeof(MessageIteratorImpl));
BSLMF_ASSERT(false == bsl::is_polymorphic<MessageIterator>::value);
BSLMF_ASSERT(sizeof(Message) == sizeof(MessageImpl));
BSLMF_ASSERT(false == bsl::is_polymorphic<Message>::value);
}  // close unnamed namespace

// ------------------
// class MessageEvent
// ------------------

MessageEvent::MessageEvent()
: d_impl_sp(0)
{
    // NOTHING
}

MessageIterator MessageEvent::messageIterator() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);

    BSLA_MAYBE_UNUSED const bmqp::Event& rawEvent = d_impl_sp->rawEvent();

    BSLS_ASSERT(rawEvent.isAckEvent() || rawEvent.isPushEvent() ||
                rawEvent.isPutEvent());

    d_impl_sp->resetIterators();

    // Initialize the 'MessageIterator' to point to the iterator over the blob
    // this message event is linked to.
    MessageIterator      mi;
    MessageIteratorImpl& msgItImplRef = reinterpret_cast<MessageIteratorImpl&>(
        mi);
    msgItImplRef.d_event_p = d_impl_sp.get();

    MessageImpl& msgImplRef = reinterpret_cast<MessageImpl&>(
        msgItImplRef.d_message);
    msgImplRef.d_event_p = d_impl_sp.get();

    return mi;
}

bmqt::MessageEventType::Enum MessageEvent::type() const
{
    if (!d_impl_sp) {
        return bmqt::MessageEventType::e_UNDEFINED;  // RETURN
    }

    const bmqp::Event& rawEvent = d_impl_sp->rawEvent();

    if (rawEvent.isAckEvent()) {
        return bmqt::MessageEventType::e_ACK;  // RETURN
    }

    if (rawEvent.isPushEvent()) {
        return bmqt::MessageEventType::e_PUSH;  // RETURN
    }

    if (rawEvent.isPutEvent()) {
        return bmqt::MessageEventType::e_PUT;  // RETURN
    }

    BSLS_ASSERT_OPT(false && "Unreachable by design");

    return bmqt::MessageEventType::e_UNDEFINED;  // pacify compiler
}

bsl::ostream&
MessageEvent::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (!d_impl_sp) {
        return stream;  // RETURN
    }

    return d_impl_sp->print(stream, level, spacesPerLevel);
}

}  // close package namespace
}  // close enterprise namespace
