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

// bmqa_sessionevent.cpp                                              -*-C++-*-
#include <bmqa_sessionevent.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqimp_brokersession.h>
#include <bmqimp_event.h>

// BDE
#include <bsl_memory.h>
#include <bsl_ostream.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqa {

namespace {
// Do some compilation time validation
BSLMF_ASSERT(sizeof(QueueId) == sizeof(bsl::shared_ptr<bmqimp::Queue>));
}  // close unnamed namespace

// Internal documentation about this component: 'bmqa::SessionEvent' is just a
//  'view' of a 'bmqimp::Event', exposing specific accessors only, related to
//  an 'Event' of type 'SessionEvent'.

// ------------------
// class SessionEvent
// ------------------

SessionEvent::SessionEvent()
: d_impl_sp(0)
{
    // NOTHING
}

SessionEvent::SessionEvent(const SessionEvent& other)
: d_impl_sp(other.d_impl_sp)
{
    // NOTHING
}

SessionEvent& SessionEvent::operator=(const SessionEvent& rhs)
{
    d_impl_sp = rhs.d_impl_sp;
    return *this;
}

bmqt::SessionEventType::Enum SessionEvent::type() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);
    BSLS_ASSERT_SAFE(d_impl_sp->type() == bmqimp::Event::EventType::e_SESSION);

    return d_impl_sp->sessionEventType();
}

const bmqt::CorrelationId& SessionEvent::correlationId() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);
    BSLS_ASSERT_SAFE(d_impl_sp->type() == bmqimp::Event::EventType::e_SESSION);

    return d_impl_sp->correlationId();
}

QueueId SessionEvent::queueId() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);
    BSLS_ASSERT_SAFE(d_impl_sp->sessionEventType() ==
                         bmqt::SessionEventType::e_QUEUE_OPEN_RESULT ||
                     d_impl_sp->sessionEventType() ==
                         bmqt::SessionEventType::e_QUEUE_CONFIGURE_RESULT ||
                     d_impl_sp->sessionEventType() ==
                         bmqt::SessionEventType::e_QUEUE_REOPEN_RESULT ||
                     d_impl_sp->sessionEventType() ==
                         bmqt::SessionEventType::e_QUEUE_CLOSE_RESULT);

    QueueId                         queueId;
    const bmqimp::Event::QueuesMap& queues = d_impl_sp->queues();
    BSLS_ASSERT_OPT(queues.size() == 1U);

    bsl::shared_ptr<bmqimp::Queue> queue = queues.begin()->second;
    BSLS_ASSERT_OPT(queue);

    bsl::shared_ptr<bmqimp::Queue>& queueImplSpRef =
        reinterpret_cast<bsl::shared_ptr<bmqimp::Queue>&>(queueId);
    queueImplSpRef = queue;

    return queueId;
}

int SessionEvent::statusCode() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);
    BSLS_ASSERT_SAFE(d_impl_sp->type() == bmqimp::Event::EventType::e_SESSION);

    return d_impl_sp->statusCode();
}

const bsl::string& SessionEvent::errorDescription() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_impl_sp);
    BSLS_ASSERT_SAFE(d_impl_sp->type() == bmqimp::Event::EventType::e_SESSION);

    return d_impl_sp->errorDescription();
}

bsl::ostream&
SessionEvent::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (!d_impl_sp) {
        return stream;  // RETURN
    }

    return d_impl_sp->print(stream, level, spacesPerLevel);
}

bool operator==(const SessionEvent& lhs, const SessionEvent& rhs)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(lhs.d_impl_sp);
    BSLS_ASSERT_SAFE(rhs.d_impl_sp);

    return *lhs.d_impl_sp == *rhs.d_impl_sp;
}

bool operator!=(const SessionEvent& lhs, const SessionEvent& rhs)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(lhs.d_impl_sp);
    BSLS_ASSERT_SAFE(rhs.d_impl_sp);

    return *lhs.d_impl_sp != *rhs.d_impl_sp;
}

}  // close package namespace
}  // close enterprise namespace
