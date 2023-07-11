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

// bmqa_abstractsession.cpp                                           -*-C++-*-
#include <bmqa_abstractsession.h>

#include <bmqscm_version.h>
// BDE
#include <bslma_allocator.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqa {

// ---------------------
// class AbstractSession
// ---------------------

AbstractSession::~AbstractSession()
{
    // NOTHING
}

// MANIPULATORS

/// Session management
///------------------
int AbstractSession::start(
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

int AbstractSession::startAsync(
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

void AbstractSession::stop()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");
}

void AbstractSession::stopAsync()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");
}

void AbstractSession::finalizeStop()
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");
}

void AbstractSession::loadMessageEventBuilder(
    BSLS_ANNOTATION_UNUSED MessageEventBuilder* builder)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");
}

void AbstractSession::loadConfirmEventBuilder(
    BSLS_ANNOTATION_UNUSED ConfirmEventBuilder* builder)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");
}

void AbstractSession::loadMessageProperties(
    BSLS_ANNOTATION_UNUSED MessageProperties* buffer)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");
}

/// Queue management
///----------------
int AbstractSession::getQueueId(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri) const
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

int AbstractSession::getQueueId(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bmqt::CorrelationId& correlationId) const
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

int AbstractSession::openQueue(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 flags,
    BSLS_ANNOTATION_UNUSED const bmqt::QueueOptions& options,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

OpenQueueStatus AbstractSession::openQueueSync(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 flags,
    BSLS_ANNOTATION_UNUSED const bmqt::QueueOptions& options,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return bmqa::OpenQueueStatus(bmqa::QueueId(),
                                 bmqt::OpenQueueResult::e_NOT_SUPPORTED,
                                 "Method is undefined in base protocol");
}

int AbstractSession::openQueueAsync(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 flags,
    BSLS_ANNOTATION_UNUSED const bmqt::QueueOptions& options,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

void AbstractSession::openQueueAsync(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bmqt::Uri& uri,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64      flags,
    BSLS_ANNOTATION_UNUSED const OpenQueueCallback& callback,
    BSLS_ANNOTATION_UNUSED const bmqt::QueueOptions& options,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");
}

int AbstractSession::configureQueue(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bmqt::QueueOptions& options,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

ConfigureQueueStatus AbstractSession::configureQueueSync(
    BSLS_ANNOTATION_UNUSED const QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bmqt::QueueOptions& options,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)

{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return bmqa::ConfigureQueueStatus(
        bmqa::QueueId(),
        bmqt::ConfigureQueueResult::e_NOT_SUPPORTED,
        "Method is undefined in base protocol");
}

int AbstractSession::configureQueueAsync(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bmqt::QueueOptions& options,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

void AbstractSession::configureQueueAsync(
    BSLS_ANNOTATION_UNUSED const QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bmqt::QueueOptions&     options,
    BSLS_ANNOTATION_UNUSED const ConfigureQueueCallback& callback,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");
}

int AbstractSession::closeQueue(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

CloseQueueStatus AbstractSession::closeQueueSync(
    BSLS_ANNOTATION_UNUSED const QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return bmqa::CloseQueueStatus(bmqa::QueueId(),
                                  bmqt::CloseQueueResult::e_NOT_SUPPORTED,
                                  "Method is undefined in base protocol");
}

int AbstractSession::closeQueueAsync(
    BSLS_ANNOTATION_UNUSED QueueId* queueId,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

void AbstractSession::closeQueueAsync(
    BSLS_ANNOTATION_UNUSED const QueueId*            queueId,
    BSLS_ANNOTATION_UNUSED const CloseQueueCallback& callback,
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");
}

/// Queue manipulation
///------------------
Event AbstractSession::nextEvent(
    BSLS_ANNOTATION_UNUSED const bsls::TimeInterval& timeout)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return Event();
}

int AbstractSession::post(BSLS_ANNOTATION_UNUSED const MessageEvent& event)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

int AbstractSession::confirmMessage(
    BSLS_ANNOTATION_UNUSED const Message& message)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

int AbstractSession::confirmMessage(
    BSLS_ANNOTATION_UNUSED const MessageConfirmationCookie& cookie)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

int AbstractSession::confirmMessages(
    BSLS_ANNOTATION_UNUSED ConfirmEventBuilder* builder)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

/// Debugging related
///-----------------
int AbstractSession::configureMessageDumping(
    BSLS_ANNOTATION_UNUSED const bslstl::StringRef& command)
{
    // PRECONDITIONS
    BSLS_ASSERT_OPT(false && "Method is undefined in base protocol");

    return -1;
}

}  // close package namespace
}  // close enterprise namespace
