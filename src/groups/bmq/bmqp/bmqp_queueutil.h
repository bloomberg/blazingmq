// Copyright 2023 Bloomberg Finance L.P.
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

// bmqp_queueutil.h                                                 -*-C++-*-
#ifndef INCLUDED_BMQP_QUEUEUTIL
#define INCLUDED_BMQP_QUEUEUTIL

//@PURPOSE: Provide utilities related to queue management.
//
//@CLASSES:
//  bmqp::QueueUtil: utilities related to queue management.
//
//@DESCRIPTION: 'bmqp::QueueUtil' provide a utility namespace for operations
// related to queue management, such as aggregating openQueue parameters.
//

// BMQ

#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_protocolutil.h>
#include <bmqp_queueid.h>
#include <bmqt_queueflags.h>

// BDE
#include <bsl_vector.h>
#include <bslmt_once.h>
#include <bsls_objectbuffer.h>

namespace BloombergLP {
namespace bmqp {

// ================
// struct QueueUtil
// ================

/// Utility namespace for queue management
struct QueueUtil {
    // CLASS METHODS

    /// Return true if the specified handle `parameters` are valid, false
    /// otherwise.
    static bool isValid(const bmqp_ctrlmsg::QueueHandleParameters& parameters);

    /// Return true if the specified handle parameter `subset` is a valid
    /// subset of the specified `aggregate` handle parameters.  Note that an
    /// improper `subset` is considered to be a valid subset.
    static bool
    isValidSubset(const bmqp_ctrlmsg::QueueHandleParameters& subset,
                  const bmqp_ctrlmsg::QueueHandleParameters& aggregate);

    /// Merge the specified `in` handle parameters with the specified `out`
    /// parameters.  The behavior is undefined unless `in` and `out`
    /// parameters represent the same queue URI, and `in` represents valid
    /// handle parameters (ie, `isValid(in)` returns true).
    static void
    mergeHandleParameters(bmqp_ctrlmsg::QueueHandleParameters*       out,
                          const bmqp_ctrlmsg::QueueHandleParameters& in);

    /// Subtract the specified `in` handle parameters from the specified
    /// `out` parameters using the optionally specified `isFinal` flag.  The
    /// behavior is undefined unless `in` and `out` represent the same queue
    /// URI, and `in` is a valid subset of `out` (ie, 'isValidSubset(in,
    /// *out)` returns true.  If `isFinal' is true, then `out` is reset.
    static void
    subtractHandleParameters(bmqp_ctrlmsg::QueueHandleParameters*       out,
                             const bmqp_ctrlmsg::QueueHandleParameters& in);
    static void
    subtractHandleParameters(bmqp_ctrlmsg::QueueHandleParameters*       out,
                             const bmqp_ctrlmsg::QueueHandleParameters& in,
                             bool isFinal);

    /// Create and return a `bmqp::QueueId` object constructed from the id
    static bmqp::QueueId createQueueIdFromHandleParameters(
        const bmqp_ctrlmsg::QueueHandleParameters& handleParameters);

    template <typename PARAMS>
    static const bmqp_ctrlmsg::SubQueueIdInfo&
    extractSubQueueInfo(const PARAMS& parameters);

    /// Extract and return the subId attribute from the specified
    /// `parameters` if a subQueueId was specified, otherwise return the
    /// default subQueueId.  Note that `params` can be handle or stream
    /// parameters.
    template <typename PARAMS>
    static unsigned int extractSubQueueId(const PARAMS& parameters);

    /// Return the appId from the specified `parameters` if they contain
    /// subIdInfo, otherwise return k_DEFAULT_APP_ID.  Note that
    /// `parameters` can be handle or stream parameters.
    template <typename PARAMS>
    static const char* extractAppId(const PARAMS& parameters);

    /// Return true if the specified `subIdInfo` is the default app and the
    /// default subStream.
    static bool
    isDefaultSubstream(const bmqp_ctrlmsg::SubQueueIdInfo& subIdInfo);

    /// Return true if the specified `parameters` has the default app and
    /// the default subStream.
    static bool
    isDefaultSubstream(const bmqp_ctrlmsg::QueueHandleParameters& parameters);

    /// Return the consumer/producer portion/subset of the specified
    /// `queueHandleParamers` with READ flag, full URI, and the specified
    /// `readCount` combined with the specified  `subStreamInfo`
    /// into one coherent handle parameters object.  The full URI is
    /// canonical uri and appId from the `queueHandleParameters`.  The
    /// behavior is undefined unless `readCount` >= 1 and `subStreamInfo`
    /// has a non-null appId.  The behavior is undefined unless the uri in
    /// the `queueHandleParameters` is canonical.
    static bmqp_ctrlmsg::QueueHandleParameters createHandleParameters(
        const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
        const bmqp_ctrlmsg::SubQueueIdInfo&        subStreamInfo,
        int                                        readCount);

    /// Copy into the specified `canonicalHandleParameters` the specified
    /// `queueHandleParameters`, replacing the uri member with the canonical
    /// representation of that uri and excluding the contained subStream
    /// information.  Return a reference to `canonicalHandleParameters`.
    static bmqp_ctrlmsg::QueueHandleParameters&
    extractCanonicalHandleParameters(
        bmqp_ctrlmsg::QueueHandleParameters*       canonicalHandleParameters,
        const bmqp_ctrlmsg::QueueHandleParameters& queueHandleParameters);

    /// Return true if all the counts in the specified `handleParameters`
    /// are <= 0, and false otherwise.
    static bool
    isEmpty(const bmqp_ctrlmsg::QueueHandleParameters& handleParameters);

    /// Return true if the specified `subQueueId` has a valid value for a
    /// *fanout* consumer, false otherwise.
    static bool isValidFanoutConsumerSubQueueId(unsigned int subQueueId);

    /// Return `true` is the specified `sp` have no consumer parameters.
    static bool isEmpty(const bmqp_ctrlmsg::StreamParameters& sp);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------
// struct QueueUtil
// ----------------

inline bmqp::QueueId QueueUtil::createQueueIdFromHandleParameters(
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters)
{
    const int          id    = handleParameters.qId();
    const unsigned int subId = extractSubQueueId(handleParameters);

    return bmqp::QueueId(id, subId);
}

template <typename PARAMS>
inline const bmqp_ctrlmsg::SubQueueIdInfo&
QueueUtil::extractSubQueueInfo(const PARAMS& parameters)
{
    if (parameters.subIdInfo().isNull()) {
        // Use an objectBuffer to avoid 'exit-time destructor' compiler warning
        static bsls::ObjectBuffer<bmqp_ctrlmsg::SubQueueIdInfo> s_consumerInfo;
        BSLMT_ONCE_DO
        {
            // use default allocator
            new (s_consumerInfo.buffer()) bmqp_ctrlmsg::SubQueueIdInfo();
        }

        return s_consumerInfo.object();  // RETURN
    }

    return parameters.subIdInfo().value();
}

template <typename PARAMS>
inline unsigned int QueueUtil::extractSubQueueId(const PARAMS& parameters)
{
    if (parameters.subIdInfo().isNull()) {
        return bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;  // RETURN
    }

    return parameters.subIdInfo().value().subId();
}

template <typename PARAMS>
inline const char* QueueUtil::extractAppId(const PARAMS& parameters)
{
    return parameters.subIdInfo().isNull()
               ? bmqp::ProtocolUtil::k_DEFAULT_APP_ID
               : parameters.subIdInfo().value().appId().c_str();
}

inline bool
QueueUtil::isDefaultSubstream(const bmqp_ctrlmsg::SubQueueIdInfo& subIdInfo)
{
    return subIdInfo.appId() == bmqp::ProtocolUtil::k_DEFAULT_APP_ID &&
           subIdInfo.subId() == bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID;
}

inline bool
QueueUtil::isDefaultSubstream(const bmqp_ctrlmsg::QueueHandleParameters& p)
{
    if (p.subIdInfo().isNull()) {
        return true;  // RETURN;
    }
    return isDefaultSubstream(p.subIdInfo().value());
}

inline bool
QueueUtil::isEmpty(const bmqp_ctrlmsg::QueueHandleParameters& handleParameters)
{
    return handleParameters.readCount() <= 0 &&
           handleParameters.writeCount() <= 0 &&
           handleParameters.adminCount() <= 0;
}

inline bool QueueUtil::isValidFanoutConsumerSubQueueId(unsigned int subQueueId)
{
    return bmqp::QueueId::k_RESERVED_SUBQUEUE_ID != subQueueId &&
           bmqp::QueueId::k_UNASSIGNED_SUBQUEUE_ID != subQueueId &&
           bmqp::QueueId::k_DEFAULT_SUBQUEUE_ID != subQueueId;
}

inline bool QueueUtil::isEmpty(const bmqp_ctrlmsg::StreamParameters& sp)
{
    if (sp.subscriptions().empty()) {
        return true;  // RETURN
    }
    if (sp.subscriptions()[0].consumers().empty()) {
        return true;  // RETURN
    }
    if (sp.subscriptions()[0].consumers()[0].consumerPriorityCount() == 0) {
        return true;  // RETURN
    }

    return false;
}

}  // close package namespace
}  // close enterprise namespace

#endif
