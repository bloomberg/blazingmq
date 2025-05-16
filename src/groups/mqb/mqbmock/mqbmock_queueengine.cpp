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
#include <bsls_annotation.h>

namespace BloombergLP {
namespace mqbmock {

// -----------------
// class QueueEngine
// -----------------

// CREATORS
QueueEngine::QueueEngine(BSLS_ANNOTATION_UNUSED bslma::Allocator* allocator)
{
    // NOTHING
}

QueueEngine::~QueueEngine()
{
    // NOTHING
}

// MANIPULATORS
int QueueEngine::configure(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription,
    BSLS_ANNOTATION_UNUSED bool          isReconfigure)
{
    return 0;
}

void QueueEngine::resetState(BSLS_ANNOTATION_UNUSED bool keepConfirming)
{
    // NOTHING
}

int QueueEngine::rebuildInternalState(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    return 0;
}

mqbi::QueueHandle* QueueEngine::getHandle(
    BSLS_ANNOTATION_UNUSED const
        bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    BSLS_ANNOTATION_UNUSED const        bmqp_ctrlmsg::QueueHandleParameters&
                                        handleParameters,
    BSLS_ANNOTATION_UNUSED unsigned int upstreamSubQueueId,
    BSLS_ANNOTATION_UNUSED const        mqbi::QueueHandle::GetHandleCallback&
                                        callback)
{
    return 0;
}

void QueueEngine::configureHandle(
    BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* handle,
    BSLS_ANNOTATION_UNUSED const              bmqp_ctrlmsg::StreamParameters&
                                              streamParameters,
    BSLS_ANNOTATION_UNUSED const mqbi::QueueHandle::HandleConfiguredCallback&
                                 configuredCb)
{
    // NOTHING
}

void QueueEngine::releaseHandle(
    BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* handle,
    BSLS_ANNOTATION_UNUSED const bmqp_ctrlmsg::QueueHandleParameters&
                                 handleParameters,
    BSLS_ANNOTATION_UNUSED bool  isFinal,
    BSLS_ANNOTATION_UNUSED const mqbi::QueueHandle::HandleReleasedCallback&
                                 releasedCb)
{
    // NOTHING
}

void QueueEngine::onHandleUsable(
    BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* handle,
    BSLS_ANNOTATION_UNUSED unsigned int       upstreamSubQueueId)
{
    // NOTHING
}

void QueueEngine::afterNewMessage(
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID,
    BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* source)
{
    // NOTHING
}

int QueueEngine::onConfirmMessage(
    BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* handle,
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID,
    BSLS_ANNOTATION_UNUSED unsigned int             subQueueId)
{
    return 0;
}

int QueueEngine::onRejectMessage(
    BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* handle,
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID,
    BSLS_ANNOTATION_UNUSED unsigned int             subQueueId)
{
    // TODO: Implement
    return 0;
}

void QueueEngine::beforeMessageRemoved(
    BSLS_ANNOTATION_UNUSED const bmqt::MessageGUID& msgGUID)
{
    // NOTHING
}

void QueueEngine::afterQueuePurged(
    BSLS_ANNOTATION_UNUSED const bsl::string& appId,
    BSLS_ANNOTATION_UNUSED const mqbu::StorageKey& appKey)
{
    // NOTHING
}

void QueueEngine::afterPostMessage(BSLS_ANNOTATION_UNUSED mqbi::QueueHandle* source)
{
    // NOTHING
}

mqbi::StorageResult::Enum QueueEngine::evaluateAppSubscriptions(
    BSLS_ANNOTATION_UNUSED const bmqp::PutHeader& putHeader,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<bdlbb::Blob>& appData,
    BSLS_ANNOTATION_UNUSED const bmqp::MessagePropertiesInfo& mpi,
    BSLS_ANNOTATION_UNUSED bsls::Types::Uint64 timestamp)
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

void QueueEngine::loadInternals(
    BSLS_ANNOTATION_UNUSED mqbcmd::QueueEngine* out) const
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
