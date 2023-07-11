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

// bmqa_event.cpp                                                     -*-C++-*-
#include <bmqa_event.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqimp_event.h>

// BDE
#include <bsl_ostream.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqa {

namespace {
// Do some compile time validation
BSLMF_ASSERT(sizeof(SessionEvent) == sizeof(bsl::shared_ptr<bmqimp::Event>));
BSLMF_ASSERT(sizeof(MessageEvent) == sizeof(bsl::shared_ptr<bmqimp::Event>));
}  // close unnamed namespace

// -----------
// class Event
// -----------

Event::Event()
: d_impl_sp(0)
{
    // NOTHING
}

SessionEvent Event::sessionEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);
    BSLS_ASSERT_SAFE(isSessionEvent());

    SessionEvent                    event;
    bsl::shared_ptr<bmqimp::Event>& eventImpl =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Event>&>(event);
    eventImpl = d_impl_sp;

    return event;
}

MessageEvent Event::messageEvent() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);
    BSLS_ASSERT_SAFE(isMessageEvent());

    MessageEvent                    event;
    bsl::shared_ptr<bmqimp::Event>& eventImpl =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Event>&>(event);
    eventImpl = d_impl_sp;

    return event;
}

bool Event::isSessionEvent() const
{
    return d_impl_sp &&
           d_impl_sp->type() == bmqimp::Event::EventType::e_SESSION;
}

bool Event::isMessageEvent() const
{
    return d_impl_sp &&
           d_impl_sp->type() == bmqimp::Event::EventType::e_MESSAGE;
}

bsl::ostream&
Event::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (!d_impl_sp) {
        return stream;  // RETURN
    }

    return d_impl_sp->print(stream, level, spacesPerLevel);
}

}  // close package namespace
}  // close enterprise namespace
