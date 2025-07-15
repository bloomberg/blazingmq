// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbmock_queueengine.cpp                                            -*-C++-*-
#include <mqbmock_queueengine.h>

#include <mqbscm_version.h>
// BDE
#include <bsl_iostream.h>
#include <bsla_annotations.h>

namespace BloombergLP {
namespace mqbmock {

// -----------------
// class QueueEngine
// -----------------

// CREATORS
QueueEngine::QueueEngine(BSLA_UNUSED bslma::Allocator* allocator)
{
    // NOTHING
}

QueueEngine::~QueueEngine()
{
    // NOTHING
}

// MANIPULATORS
int QueueEngine::configure(BSLA_UNUSED bsl::ostream& errorDescription,
                           BSLA_UNUSED bool          isReconfigure)
{
    return 0;
}

void QueueEngine::resetState(BSLA_UNUSED bool keepConfirming)
{
    // NOTHING
}

int QueueEngine::rebuildInternalState(
    BSLA_UNUSED bsl::ostream& errorDescription)
{
    return 0;
}

mqbi::QueueHandle* QueueEngine::getHandle(
    BSLA_UNUSED const mqbi::OpenQueueConfirmationCookieSp& context,
    BSLA_UNUSED const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>&
                      clientContext,
    BSLA_UNUSED const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
    BSLA_UNUSED unsigned int                               upstreamSubQueueId,
    BSLA_UNUSED const mqbi::QueueHandle::GetHandleCallback& callback)
{
    return 0;
}

void QueueEngine::configureHandle(
    BSLA_UNUSED mqbi::QueueHandle* handle,
    BSLA_UNUSED const bmqp_ctrlmsg::StreamParameters& streamParameters,
    BSLA_UNUSED const mqbi::QueueHandle::HandleConfiguredCallback&
                      configuredCb)
{
    // NOTHING
}

void QueueEngine::releaseHandle(
    BSLA_UNUSED mqbi::QueueHandle* handle,
    BSLA_UNUSED const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
    BSLA_UNUSED bool                                       isFinal,
    BSLA_UNUSED const mqbi::QueueHandle::HandleReleasedCallback& releasedCb)
{
    // NOTHING
}

void QueueEngine::onHandleUsable(BSLA_UNUSED mqbi::QueueHandle* handle,
                                 BSLA_UNUSED unsigned int upstreamSubQueueId)
{
    // NOTHING
}

void QueueEngine::afterNewMessage(BSLA_UNUSED const bmqt::MessageGUID& msgGUID,
                                  BSLA_UNUSED mqbi::QueueHandle* source)
{
    // NOTHING
}

int QueueEngine::onConfirmMessage(BSLA_UNUSED mqbi::QueueHandle* handle,
                                  BSLA_UNUSED const bmqt::MessageGUID& msgGUID,
                                  BSLA_UNUSED unsigned int subQueueId)
{
    return 0;
}

int QueueEngine::onRejectMessage(BSLA_UNUSED mqbi::QueueHandle* handle,
                                 BSLA_UNUSED const bmqt::MessageGUID& msgGUID,
                                 BSLA_UNUSED unsigned int subQueueId)
{
    // TODO: Implement
    return 0;
}

void QueueEngine::beforeMessageRemoved(
    BSLA_UNUSED const bmqt::MessageGUID& msgGUID)
{
    // NOTHING
}

void QueueEngine::afterQueuePurged(BSLA_UNUSED const bsl::string& appId,
                                   BSLA_UNUSED const mqbu::StorageKey& appKey)
{
    // NOTHING
}

void QueueEngine::afterPostMessage()
{
    // executed by the *QUEUE DISPATCHER* thread

    // NOTHING
}

mqbi::StorageResult::Enum QueueEngine::evaluateAppSubscriptions(
    BSLA_UNUSED const bmqp::PutHeader& putHeader,
    BSLA_UNUSED const bsl::shared_ptr<bdlbb::Blob>& appData,
    BSLA_UNUSED const bmqp::MessagePropertiesInfo& mpi,
    BSLA_UNUSED bsls::Types::Uint64 timestamp)
{
    // executed by the *QUEUE DISPATCHER* thread

    // NOTHING
    return mqbi::StorageResult::e_SUCCESS;
}

// ACCESSORS
unsigned int QueueEngine::messageReferenceCount() const
{
    return 0;
}

void QueueEngine::loadInternals(BSLA_UNUSED mqbcmd::QueueEngine* out) const
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
