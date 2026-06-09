// Copyright 2026 Bloomberg Finance L.P.
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

#ifndef INCLUDED_BMQP_CONVERSIONUTIL
#define INCLUDED_BMQP_CONVERSIONUTIL

//@PURPOSE: Provide utilities for converting between old-style and new-style
// BlazingMQ control messages.
//
//@CLASSES:
//  bmqp::ConversionUtil: utilities for control message conversion
//
//@DESCRIPTION: 'bmqp::ConversionUtil' provides a set of utility methods for
// converting between old-style 'ConfigureQueueStream' /
// 'QueueStreamParameters' and new-style 'ConfigureStream' /
// 'StreamParameters' control message formats.

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocol.h>

// BDE
#include <bdlb_nullablevalue.h>
#include <bsl_string.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqp {

// =====================
// struct ConversionUtil
// =====================

/// Utilities for control message conversion
struct ConversionUtil {
    // CLASS METHODS

    /// Convert between old-style 'QueueStreamParameters' and new-style
    /// 'StreamParameters'.
    static void convert(
        bmqp_ctrlmsg::QueueStreamParameters*                     to,
        const bmqp_ctrlmsg::StreamParameters&                    from,
        const bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo>& subIdInfo);

    static void convert(bmqp_ctrlmsg::StreamParameters*            to,
                        const bmqp_ctrlmsg::QueueStreamParameters& from);

    static void convert(bmqp_ctrlmsg::ConfigureStream*            to,
                        const bmqp_ctrlmsg::ConfigureQueueStream& from);

    static void convert(bmqp_ctrlmsg::ConsumerInfo*                to,
                        const bmqp_ctrlmsg::QueueStreamParameters& from);

    static bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo>
    makeSubQueueIdInfo(const bsl::string& appId, unsigned int subId);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// struct ConversionUtil
// ---------------------

inline void ConversionUtil::convert(
    bmqp_ctrlmsg::QueueStreamParameters*                     to,
    const bmqp_ctrlmsg::StreamParameters&                    from,
    const bdlb::NullableValue<bmqp_ctrlmsg::SubQueueIdInfo>& subIdInfo)
{
    BSLS_ASSERT_SAFE(from.subscriptions().size() < 2);

    to->subIdInfo() = subIdInfo;

    if (from.subscriptions().size() > 0) {
        BSLS_ASSERT_SAFE(from.subscriptions().size() == 1);
        // Enable multiple subscriptions in SDK only when all brokers support
        // them.
        BSLS_ASSERT_SAFE(from.subscriptions()[0].consumers().size() > 0);
        // New ConfigureStream request can carry multiple priorities but the
        // first element in the array is the highest priority one.
        const bmqp_ctrlmsg::ConsumerInfo& ci =
            from.subscriptions()[0].consumers()[0];

        to->consumerPriority()       = ci.consumerPriority();
        to->consumerPriorityCount()  = ci.consumerPriorityCount();
        to->maxUnconfirmedMessages() = ci.maxUnconfirmedMessages();
        to->maxUnconfirmedBytes()    = ci.maxUnconfirmedBytes();
    }
    else {
        to->consumerPriority()       = Protocol::k_CONSUMER_PRIORITY_INVALID;
        to->consumerPriorityCount()  = 0;
        to->maxUnconfirmedMessages() = 0;
        to->maxUnconfirmedBytes()    = 0;
    }
}

inline void
ConversionUtil::convert(bmqp_ctrlmsg::ConfigureStream*            to,
                        const bmqp_ctrlmsg::ConfigureQueueStream& from)
{
    const bmqp_ctrlmsg::QueueStreamParameters& oldStype =
        from.streamParameters();

    to->qId() = from.qId();
    to->streamParameters().subscriptions().resize(1);
    bmqp_ctrlmsg::Subscription& subscription =
        to->streamParameters().subscriptions()[0];

    if (!oldStype.subIdInfo().isNull()) {
        subscription.sId()             = oldStype.subIdInfo().value().subId();
        to->streamParameters().appId() = oldStype.subIdInfo().value().appId();
    }
    // else DEFAULT_INITIALIZER_ID      (0),
    //      DEFAULT_INITIALIZER_APP_ID  ("__default")

    subscription.consumers().resize(1);
    convert(&subscription.consumers()[0], oldStype);
}

inline void
ConversionUtil::convert(bmqp_ctrlmsg::StreamParameters*            to,
                        const bmqp_ctrlmsg::QueueStreamParameters& from)
{
    if (!from.subIdInfo().isNull()) {
        to->appId() = from.subIdInfo().value().appId();
    }
    // else DEFAULT_INITIALIZER_APP_ID  ("__default")

    if (from.consumerPriorityCount()) {
        to->subscriptions().resize(1);
        to->subscriptions()[0].consumers().resize(1);

        convert(&to->subscriptions()[0].consumers()[0], from);
    }
    else {
        BSLS_ASSERT_SAFE(from.consumerPriority() ==
                         Protocol::k_CONSUMER_PRIORITY_INVALID);
        BSLS_ASSERT_SAFE(from.consumerPriorityCount() == 0);
        BSLS_ASSERT_SAFE(from.maxUnconfirmedMessages() == 0);
        BSLS_ASSERT_SAFE(from.maxUnconfirmedBytes() == 0);
    }
}

inline void
ConversionUtil::convert(bmqp_ctrlmsg::ConsumerInfo*                to,
                        const bmqp_ctrlmsg::QueueStreamParameters& from)
{
    to->consumerPriority()       = from.consumerPriority();
    to->consumerPriorityCount()  = from.consumerPriorityCount();
    to->maxUnconfirmedMessages() = from.maxUnconfirmedMessages();
    to->maxUnconfirmedBytes()    = from.maxUnconfirmedBytes();
}

}  // close package namespace
}  // close enterprise namespace

#endif
