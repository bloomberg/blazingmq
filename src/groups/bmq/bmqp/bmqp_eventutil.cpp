// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqp_eventutil.cpp                                                 -*-C++-*-
#include <bmqp_eventutil.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_event.h>
#include <bmqp_optionsview.h>
#include <bmqp_protocol.h>
#include <bmqp_pusheventbuilder.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqt_resultcode.h>

#include <bmqc_array.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

namespace {

BSLMF_ASSERT(OptionType::k_HIGHEST_SUPPORTED_TYPE ==
             OptionType::e_SUB_QUEUE_INFOS);
// If we add new options (i.e. options other than SubQueueId), we simply
// need to implement the processing of that particular option inside
// 'importOptions' below.

BSLMF_ASSERT(Protocol::SubQueueIdsArrayOld::static_size >= 1);
// We want the static size of the SubQueueIdsArray to be at least one to
// have our 'currSubQueueId' array in the 'flattenPushEvent' method not
// allocate from the heap (that would be inefficient)

// ===============
// class Flattener
// ===============

/// Implements the flattening functionality
class Flattener {
  private:
    // PRIVATE TYPES
    typedef bmqp::BlobPoolUtil::BlobSpPool BlobSpPool;

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // No error
        ,
        rc_ITERATION_ERROR = -1  // An error was encountered while
                                 // iterating
        ,
        rc_OPTIONS_LOAD_ERROR = -2  // An error occurred while loading
                                    // the options of a message
        ,
        rc_SUB_QUEUE_IDS_LOAD_ERROR = -3  // An error occurred while loading a
                                          // SubQueueId option
        ,
        rc_APP_DATA_LOAD_ERROR = -4  // An error occurred while loading
                                     // application data
        ,
        rc_ADD_OPTION_ERROR = -5  // Failed to add an option
        ,
        rc_PACK_MESSAGE_ERROR = -6  // Failed to pack a message
        ,
        rc_SUBSEQUENT_RETRIES_ERROR =
            -7  // Too many subsequent failed attempts
    };

    /// Convenience alias of type type
    typedef bmqt::EventBuilderResult::Enum EventBuilderResult;

    /// Return type for `cloneAndPackSingle`.
    typedef bsl::pair<EventBuilderResult, RcEnum> CloneSingleStatus;

  private:
    // CLASS DATA

    // Convenience alias
    static const EventBuilderResult k_SUCCESS =
        bmqt::EventBuilderResult::e_SUCCESS;

    // DATA
    bsl::vector<EventUtilEventInfo>* d_eventInfos_p;
    // Vector with Event Infos
    bslma::Allocator* d_allocator_p;
    // The allocator
    PushEventBuilder d_builder;
    // The builder
    PushMessageIterator d_msgIterator;
    // Helps go through messages
    EventUtilEventInfo d_currEventInfo;
    // The Event Info we process now
    bdlbb::Blob d_appData;
    // App data for current message
    OptionsView d_optionsView;
    // Helps go through Options

    // PRIVATE TYPES
    typedef bdlma::LocalSequentialAllocator<
        32 * sizeof(Protocol::SubQueueIdsArrayOld::value_type)>
        LocalAllocator;

  private:
    // CLASS METHODS

    /// Compile an appropriate integer return code using the specified
    /// `result` and `error` codes.
    static int packError(const EventBuilderResult result, const int error);

    /// Compile an appropriate integer return code using the specified
    /// `error` and `context` codes.
    static int packError(const int error, const int context);

    /// Return `true` if the specified `optionsView` indicates it has
    /// subqueue ids or SubQueueInfos.
    static bool hasSubQueues(const OptionsView& optionsView);

    // PRIVATE MANIPULATORS

    /// Create copies for each subQueueInfo in the specified `subQInfos` and
    /// packs each copy.  Uses the specified `localAllocator` for
    /// allocations.
    int cloneAndPackEachSubQId(const Protocol::SubQueueInfosArray& subQInfos,
                               LocalAllocator* localAllocator);

    /// Create a single copy for a specified `subQInfo`, and it.  If the
    /// specified `shouldAddSubQueueIdOption` property is set, the method
    /// will add the subId option to the message.  Otherwise it will not.
    CloneSingleStatus
    cloneAndPackSingle(const Protocol::SubQueueInfosArray& subQInfo);

    /// Add the option having any available `type` (with the exception of
    /// `e_SUB_QUEUE_IDS` which is handled separately) in `d_optionsView` to
    /// the current (to-be-packed) message in the event being built by
    /// `d_builder`.  Return 0 on success, or a meaningful non-zero error
    /// code otherwise.  The behavior is undefined unless `d_optionsView` is
    /// valid.
    EventBuilderResult importOptions();

    /// Does the actual packing for a single message with the specified
    /// `subQInfo` (should contain only a single element).
    EventBuilderResult
    packMesage(const Protocol::SubQueueInfosArray& subQInfo);

    /// A wrapper convenience utility used by `flattenPushEvent`.  Append to
    /// `d_eventInfos_p` the latest eventInfo using `d_builder` (for its
    /// event blob) and `d_currEventInfo` (for its queueIds) and reset
    /// `d_builder` and `d_currEventInfo`.  The behavior is undefined unless
    /// the message count in the event currently being built by the
    /// `d_builder` is greater than zero (because we are building from an
    /// already existing, valid event).
    void advanceEvent();

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    Flattener(const Flattener&);             // = delete
    Flattener& operator=(const Flattener&);  // = delete

  public:
    // CREATORS

    /// Create a `Flattener` using the specified `eventInfos`, `event`,
    /// `blobSpPool_p`, `bufferFactory` and `allocator`
    Flattener(bsl::vector<EventUtilEventInfo>* eventInfos,
              const Event&                     event,
              bdlbb::BlobBufferFactory*        bufferFactory,
              BlobSpPool*                      blobSpPool_p,
              bslma::Allocator*                allocator);

    // MANIPULATORS

    /// Iterate over each message and pack that message once per subQueueId
    /// (or once if there is no subQueueId)
    int flattenPushEvent();
};

// ---------------
// class Flattener
// ---------------

int Flattener::packError(const EventBuilderResult result, const int error)
{
    return packError(static_cast<int>(result), error);
}

int Flattener::packError(const int error, const int context)
{
    return 10 * error + context;
}

bool Flattener::hasSubQueues(const OptionsView& optionsView)
{
    return (optionsView.find(OptionType::e_SUB_QUEUE_INFOS) !=
            optionsView.end()) ||
           (optionsView.find(OptionType::e_SUB_QUEUE_IDS_OLD) !=
            optionsView.end());
}

int Flattener::cloneAndPackEachSubQId(
    const Protocol::SubQueueInfosArray& subQInfos,
    LocalAllocator*                     localAllocator)
{
    Protocol::SubQueueInfosArray subQInfo(1, SubQueueInfo(), localAllocator);
    // Value doesn't matter will be overridden soon

    // 'subsequentRetry' is a flag that helps us detect and assert that there
    // won't be an infinite loop in the case where 'cloneAndPackSingle()' keeps
    // returning 'e_EVENT_TOO_BIG'. this should never happen if the
    // implementation is correct.
    bool     subsequentRetry = false;
    unsigned selectedSubId   = 0;

    while (selectedSubId < subQInfos.size()) {
        subQInfo[0] = subQInfos[selectedSubId];

        const CloneSingleStatus result = cloneAndPackSingle(subQInfo);

        if (BSLS_PERFORMANCEHINT_PREDICT_LIKELY(result.first == k_SUCCESS)) {
            ++selectedSubId;
            subsequentRetry = false;
        }
        else if (result.first == bmqt::EventBuilderResult::e_EVENT_TOO_BIG) {
            // Handling the special case where the operation would yield an
            // event that is too big

            // We expect to not get here for the same item more than once.
            BSLS_ASSERT_SAFE(!subsequentRetry);
            if (subsequentRetry) {
                return packError(result.first,
                                 rc_SUBSEQUENT_RETRIES_ERROR);  // RETURN
            }

            // Retry
            subsequentRetry = true;
            advanceEvent();
        }
        else {
            // Any other error: We were unable to perform the operation
            // indicated by `d_potentialRootCause`.  Return with failure.
            return packError(result.first, result.second);  // RETURN
        }
    }

    return rc_SUCCESS;
}

Flattener::CloneSingleStatus
Flattener::cloneAndPackSingle(const Protocol::SubQueueInfosArray& subQInfo)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(subQInfo.size() == 1);

    EventBuilderResult result = k_SUCCESS;

    // Step 1. Add SubQueueInfo to this event. Set packRdaCounter to true.
    result = d_builder.addSubQueueInfosOption(subQInfo, true);

    // Step 2. Copy all options (excluding SubQueueId and rdaCounter which
    //                           was done on Step 1.)
    result = importOptions();
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(result != k_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return CloneSingleStatus(result, rc_ADD_OPTION_ERROR);  // RETURN
    }

    // Step 3. Pack this message using 'd_builder'.
    result = packMesage(subQInfo);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(result != k_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return CloneSingleStatus(result, rc_PACK_MESSAGE_ERROR);  // RETURN
    }

    return CloneSingleStatus(result, rc_SUCCESS);
}

Flattener::EventBuilderResult Flattener::importOptions()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_optionsView.isValid());

    static const int lowI = static_cast<int>(
        OptionType::k_LOWEST_SUPPORTED_TYPE);
    static const int highI = static_cast<int>(
        OptionType::k_HIGHEST_SUPPORTED_TYPE);

    EventBuilderResult result = k_SUCCESS;
    for (OptionsView::const_iterator i = d_optionsView.begin();
         i != d_optionsView.end();
         ++i) {
        const OptionType::Enum type = *i;
        const int              intI = static_cast<int>(*i);

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                !(lowI <= intI && intI <= highI))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // Not supported in this version
            continue;  // CONTINUE
        }

        switch (type) {
        case OptionType::e_SUB_QUEUE_IDS_OLD: {
            // NOTE: This case is explicitly handled by the caller and we do
            //       not need to handle it here.
            result = k_SUCCESS;
        } break;
        case OptionType::e_MSG_GROUP_ID: {
            typedef bdlma::LocalSequentialAllocator<
                bmqp::Protocol::k_MSG_GROUP_ID_MAX_LENGTH + 1>
                LSA;

            LSA                        lsa(d_allocator_p);
            bmqp::Protocol::MsgGroupId msgGroupId(&lsa);

            int rc = d_optionsView.loadMsgGroupIdOption(&msgGroupId);
            BSLS_ASSERT_SAFE(rc == 0);
            (void)rc;  // Compiler happiness
            result = d_builder.addMsgGroupIdOption(msgGroupId);
        } break;
        case OptionType::e_SUB_QUEUE_INFOS: {
            // NOTE: This case is explicitly handled by the caller and we do
            //       not need to handle it here.
            result = k_SUCCESS;
        } break;
        // Add operations for your own 'OptionType's here.
        case OptionType::e_UNDEFINED:
        default: {
            BSLS_ASSERT_SAFE(false && "Unknown option");
        }
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(result != k_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            break;  // BREAK
        }
    }
    return result;
}

Flattener::EventBuilderResult
Flattener::packMesage(const Protocol::SubQueueInfosArray& subQInfo)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(subQInfo.size() == 1u);

    const PushHeader& header = d_msgIterator.header();

    const EventBuilderResult result = d_builder.packMessage(d_appData, header);
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(result != k_SUCCESS)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return result;  // RETURN
    }

    // Successfully packed the current message and associated it with the
    // current SubQueueId being processed.  Add the corresponding queueId for
    // this message associated with the current flattened event.
    d_currEventInfo.d_ids.push_back(
        bmqp::EventUtilQueueInfo(subQInfo[0].id(),
                                 header,
                                 d_msgIterator.applicationDataSize()));

    return result;
}

void Flattener::advanceEvent()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_eventInfos_p);

    // We expect that we only need to advance to the next event if the builder
    // had already built at least one message in the current event because we
    // are building from an already existing, valid message.
    BSLS_ASSERT_SAFE(d_builder.messageCount() > 0);
    BSLS_ASSERT_SAFE(!d_currEventInfo.d_ids.empty());

    d_eventInfos_p->emplace_back(d_builder.blob(), d_currEventInfo.d_ids);

    d_currEventInfo.d_ids.clear();
    d_builder.reset();
}

Flattener::Flattener(bsl::vector<EventUtilEventInfo>* eventInfos,
                     const Event&                     event,
                     bdlbb::BlobBufferFactory*        bufferFactory,
                     BlobSpPool*                      blobSpPool_p,
                     bslma::Allocator*                allocator)
: d_eventInfos_p(eventInfos)
, d_allocator_p(allocator)
, d_builder(blobSpPool_p, allocator)
, d_msgIterator(bufferFactory, allocator)
, d_currEventInfo(allocator)
, d_appData(bufferFactory, allocator)
, d_optionsView(allocator)
{
    event.loadPushMessageIterator(&d_msgIterator);
    BSLS_ASSERT_SAFE(d_msgIterator.isValid());
}

int Flattener::flattenPushEvent()
{
    int rc = rc_SUCCESS;

    // Note: Valid push event means that there will be at least one message and
    //       the following iteration will happen at least once.
    while (BSLS_PERFORMANCEHINT_PREDICT_LIKELY((rc = d_msgIterator.next()) ==
                                               1)) {
        // Reset because it might have data from previous iterations.
        d_appData.removeAll();

        rc = d_msgIterator.loadApplicationData(&d_appData);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != rc_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            return packError(rc, rc_APP_DATA_LOAD_ERROR);  // RETURN
        }

        rc = d_msgIterator.loadOptionsView(&d_optionsView);
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                (rc != rc_SUCCESS) || (!d_optionsView.isValid()))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            return packError(rc, rc_OPTIONS_LOAD_ERROR);  // RETURN
        }

        // Extract subQueueInfos.  Even if there aren't any,
        // 'cloneAndPackEachSubQId()' will still do an iteration.
        LocalAllocator               localAllocator(d_allocator_p);
        Protocol::SubQueueInfosArray subQInfos(&localAllocator);
        if (hasSubQueues(d_optionsView)) {
            rc = d_optionsView.loadSubQueueInfosOption(&subQInfos);
        }
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != rc_SUCCESS)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            return packError(rc, rc_SUB_QUEUE_IDS_LOAD_ERROR);  // RETURN
        }

        // We want to import each subQueueInfo exactly once.  If there are 0
        // subQueueInfos, we still want to have one iteration of the loop below
        // so that we pack the original message once.  The order doesn't have
        // to follow the order of the subQueueInfos in the original message but
        // the fact that it is, is currently leveraged to simplify the tests.
        if (subQInfos.empty()) {
            subQInfos.push_back(
                SubQueueInfo(bmqp::Protocol::k_DEFAULT_SUBSCRIPTION_ID));
        }

        rc = cloneAndPackEachSubQId(subQInfos, &localAllocator);
        if (rc != rc_SUCCESS) {
            return rc;  // RETURN
        }
    }

    // 'd_msgIterator.next()' didn't return '0' or '1'
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return packError(rc, rc_ITERATION_ERROR);  // RETURN
    }

    // Flush the last.  Since the event was valid, there will always be at
    // least one message.
    advanceEvent();

    return rc_SUCCESS;
}

}  // close unnamed namespace

// ----------------
// struct EventUtil
// ----------------
int EventUtil::flattenPushEvent(bsl::vector<EventUtilEventInfo>* eventInfos,
                                const Event&                     event,
                                bdlbb::BlobBufferFactory*        bufferFactory,
                                BlobSpPool*                      blobSpPool_p,
                                bslma::Allocator*                allocator)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(eventInfos);
    BSLS_ASSERT_SAFE(eventInfos->empty());
    BSLS_ASSERT_SAFE(event.isValid() && event.isPushEvent());
    BSLS_ASSERT_SAFE(bufferFactory);
    BSLS_ASSERT_SAFE(allocator);

    Flattener flattener(eventInfos,
                        event,
                        bufferFactory,
                        blobSpPool_p,
                        allocator);
    return flattener.flattenPushEvent();
}

}  // close package namespace
}  // close enterprise namespace
