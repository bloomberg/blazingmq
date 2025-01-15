// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbnet_multirequestmanager.h                                       -*-C++-*-
#ifndef INCLUDED_MQBNET_MULTIREQUESTMANAGER
#define INCLUDED_MQBNET_MULTIREQUESTMANAGER

//@PURPOSE: Provide a mechanism to manage multiple requests.
//
//@CLASSES:
//  mqbnet::MultiRequestManager     : Mechanism to manage multiple requests
//
//@DESCRIPTION: 'mqbnet::MultiRequestManager' is a mechanism to manage multiple
// requests sent to peer nodes in a cluster.
//
/// Thread Safety
///-------------
// The 'mqbnet::MultiRequestManager' object is thread safe.

// MQB

#include <mqbnet_cluster.h>
#include <mqbnet_session.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>
#include <bmqp_requestmanager.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlcc_objectpool.h>
#include <bdlcc_sharedobjectpool.h>
#include <bdlf_bind.h>
#include <bsl_functional.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>

namespace BloombergLP {
namespace mqbnet {

// FORWARD DECLARATION
template <class REQUEST, class RESPONSE, class TARGET>
class MultiRequestManager;

// =======================================
// class MultiRequestManagerRequestContext
// =======================================

template <class REQUEST, class RESPONSE, class TARGET>
class MultiRequestManagerRequestContext {
    // FRIENDS
    friend class MultiRequestManager<REQUEST, RESPONSE, TARGET>;

  public:
    // TYPES

    /// SelfType is an alias to this `class`.
    typedef MultiRequestManagerRequestContext<REQUEST, RESPONSE, TARGET>
        SelfType;

    typedef bsl::shared_ptr<SelfType> SelfTypeSp;

    typedef bsl::vector<TARGET> Nodes;

    typedef bsl::pair<TARGET, RESPONSE> NodeResponsePair;

    typedef bsl::vector<NodeResponsePair> NodeResponsePairs;

    typedef typename NodeResponsePairs::iterator NodeResponsePairsIter;

    typedef
        typename NodeResponsePairs::const_iterator NodeResponsePairsConstIter;

    /// Signature of a callback for delivering the response in the specified
    /// `context`.
    typedef bsl::function<void(const SelfTypeSp& context)> ResponseCb;

  private:
    // PRIVATE TYPES
    typedef
        typename bmqp::RequestManager<REQUEST, RESPONSE>::RequestSp RequestSp;

    typedef typename Nodes::iterator NodesIter;

    typedef typename Nodes::const_iterator NodesConstIter;

  private:
    // DATA
    REQUEST d_request;

    NodeResponsePairs d_nodeResponsePairs;

    bsls::AtomicInt d_numOutstandingRequests;

    ResponseCb d_responseCb;

  private:
    // NOT IMPLEMENTED
    MultiRequestManagerRequestContext(
        const MultiRequestManagerRequestContext&);
    MultiRequestManagerRequestContext&
    operator=(const MultiRequestManagerRequestContext&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MultiRequestManagerRequestContext,
                                   bslma::UsesBslmaAllocator)

  public:
    // CREATORS

    /// Create a new object using the specified `allocator`
    explicit MultiRequestManagerRequestContext(bslma::Allocator* allocator);

    // MANIPULATORS

    /// Clear this object to a default constructed state; used by the
    /// ObjectPool upon returning the object to the pool.
    void clear();

    /// Return a reference offering modifiable access to the request
    /// associated with this context.
    REQUEST& request();

    void setDestinationNodes(const Nodes& nodes);

    /// Set the response call to the specified `value`.
    void setResponseCb(const ResponseCb& value);

    // ACCESSORS
    const REQUEST& request() const;

    /// Return a reference offering non-modifiable access to the
    /// corresponding member of this object.
    const NodeResponsePairs& response() const;
};

// =========================
// class MultiRequestManager
// =========================

template <class REQUEST, class RESPONSE, class TARGET = mqbnet::ClusterNode*>
class MultiRequestManager {
  public:
    // TYPES
    typedef MultiRequestManagerRequestContext<REQUEST, RESPONSE, TARGET>
        RequestContextType;

    typedef typename RequestContextType::NodeResponsePair NodeResponsePair;

    typedef typename RequestContextType::NodeResponsePairs NodeResponsePairs;

    typedef typename NodeResponsePairs::iterator NodeResponsePairsIter;

    typedef
        typename NodeResponsePairs::const_iterator NodeResponsePairsConstIter;

    /// Shortcut to a RequestContext object
    typedef bsl::shared_ptr<RequestContextType> RequestContextSp;

  private:
    // PRIVATE TYPES
    typedef bdlcc::SharedObjectPool<
        RequestContextType,
        bdlcc::ObjectPoolFunctors::DefaultCreator,
        bdlcc::ObjectPoolFunctors::Clear<RequestContextType> >
        RequestContextPool;

    typedef typename RequestContextType::NodesIter NodesIter;

    typedef bmqp::RequestManager<REQUEST, RESPONSE> RequestManagerType;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    MultiRequestManager(const MultiRequestManager&);
    MultiRequestManager& operator=(const MultiRequestManager&);

  private:
    // PRIVATE MANIPULATORS
    static bmqt::GenericResult::Enum
    sendHelper(bmqio::Channel*                     channel,
               const bsl::shared_ptr<bdlbb::Blob>& blob);

    /// Create a `MultiRequestManagerRequestContext` object at the specified
    /// `address` using the supplied `allocator`.  This is used by the
    /// Object Pool.
    void poolCreateRequestContext(void* address, bslma::Allocator* allocator);

    void onSingleResponse(
        const typename RequestManagerType::RequestSp& singleRequestContext,
        const RequestContextSp&                       multiRequestContext,
        TARGET                                        node);

    const bsl::string&
    targetDescription(const mqbnet::ClusterNode* target) const;

    const bsl::string&
    targetDescription(const bsl::shared_ptr<mqbnet::Session>& target) const;

    void setGroupId(typename RequestManagerType::RequestSp& context,
                    const mqbnet::ClusterNode*              target) const;

    void setGroupId(typename RequestManagerType::RequestSp& context,
                    const bsl::shared_ptr<mqbnet::Session>& target) const;

    const typename RequestManagerType::SendFn
    sendFn(mqbnet::ClusterNode* target) const;

    const typename RequestManagerType::SendFn
    sendFn(const bsl::shared_ptr<mqbnet::Session>& target) const;

  private:
    // DATA
    RequestManagerType* d_requestManager_p;

    RequestContextPool d_requestContextPool;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MultiRequestManager,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    MultiRequestManager(
        bmqp::RequestManager<REQUEST, RESPONSE>* requestManager,
        bslma::Allocator*                        allocator);

    /// Destructor
    ~MultiRequestManager();

    // MANIPULATORS
    RequestContextSp createRequestContext();

    void sendRequest(const RequestContextSp& context,
                     bsls::TimeInterval      timeout);

    void processResponse(const bmqp_ctrlmsg::ControlMessage& message);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------------------
// class MultiRequestManagerRequestContext
// ---------------------------------------

// CREATORS
template <class REQUEST, class RESPONSE, class TARGET>
MultiRequestManagerRequestContext<REQUEST, RESPONSE, TARGET>::
    MultiRequestManagerRequestContext(bslma::Allocator* allocator)
: d_request(allocator)
, d_nodeResponsePairs(allocator)
, d_numOutstandingRequests(0)
, d_responseCb(bsl::allocator_arg, allocator)
{
    // NOTHING
}

// MANIPULATORS
template <class REQUEST, class RESPONSE, class TARGET>
void MultiRequestManagerRequestContext<REQUEST, RESPONSE, TARGET>::clear()
{
    d_request.reset();
    d_nodeResponsePairs.clear();
    d_numOutstandingRequests = 0;
    d_responseCb             = bsl::nullptr_t();
}

template <class REQUEST, class RESPONSE, class TARGET>
REQUEST&
MultiRequestManagerRequestContext<REQUEST, RESPONSE, TARGET>::request()
{
    return d_request;
}

template <class REQUEST, class RESPONSE, class TARGET>
void MultiRequestManagerRequestContext<REQUEST, RESPONSE, TARGET>::
    setDestinationNodes(const Nodes& nodes)
{
    d_nodeResponsePairs.clear();
    for (NodesConstIter it = nodes.begin(); it != nodes.end(); ++it) {
        d_nodeResponsePairs.push_back(bsl::make_pair(*it, RESPONSE()));
    }

    d_numOutstandingRequests = static_cast<int>(d_nodeResponsePairs.size());
}

template <class REQUEST, class RESPONSE, class TARGET>
void MultiRequestManagerRequestContext<REQUEST, RESPONSE, TARGET>::
    setResponseCb(const ResponseCb& value)
{
    d_responseCb = value;
}

// ACCESSORS
template <class REQUEST, class RESPONSE, class TARGET>
const REQUEST&
MultiRequestManagerRequestContext<REQUEST, RESPONSE, TARGET>::request() const
{
    return d_request;
}

template <class REQUEST, class RESPONSE, class TARGET>
const typename MultiRequestManagerRequestContext<REQUEST,
                                                 RESPONSE,
                                                 TARGET>::NodeResponsePairs&
MultiRequestManagerRequestContext<REQUEST, RESPONSE, TARGET>::response() const
{
    return d_nodeResponsePairs;
}

// -------------------------
// class MultiRequestManager
// -------------------------

// PRIVATE MANIPULATORS
template <class REQUEST, class RESPONSE, class TARGET>
inline bmqt::GenericResult::Enum
MultiRequestManager<REQUEST, RESPONSE, TARGET>::sendHelper(
    bmqio::Channel*                     channel,
    const bsl::shared_ptr<bdlbb::Blob>& blob)
{
    bmqio::Status status;
    channel->write(&status, *blob);

    switch (status.category()) {
    case bmqio::StatusCategory::e_SUCCESS:
        return bmqt::GenericResult::e_SUCCESS;
    case bmqio::StatusCategory::e_CONNECTION:
        return bmqt::GenericResult::e_NOT_CONNECTED;
    case bmqio::StatusCategory::e_LIMIT:
        return bmqt::GenericResult::e_NOT_READY;
    case bmqio::StatusCategory::e_GENERIC_ERROR:
    case bmqio::StatusCategory::e_TIMEOUT:
    case bmqio::StatusCategory::e_CANCELED:
    default: return bmqt::GenericResult::e_UNKNOWN;
    }
    return bmqt::GenericResult::e_UNKNOWN;
}

template <class REQUEST, class RESPONSE, class TARGET>
void MultiRequestManager<REQUEST, RESPONSE, TARGET>::poolCreateRequestContext(
    void*             address,
    bslma::Allocator* allocator)
{
    new (address) RequestContextType(allocator);
}

template <class REQUEST, class RESPONSE, class TARGET>
void MultiRequestManager<REQUEST, RESPONSE, TARGET>::onSingleResponse(
    const typename RequestManagerType::RequestSp& singleRequestContext,
    const RequestContextSp&                       multiRequestContext,
    TARGET                                        node)
{
    BSLS_ASSERT_SAFE(node);
    BSLS_ASSERT_SAFE(0 < multiRequestContext->d_numOutstandingRequests);
    BSLS_ASSERT_SAFE(singleRequestContext->request().choice() ==
                     multiRequestContext->request().choice());
    // Note that above, we compare 'request.choice', instead of just
    // 'request' so as not to compare the 'request.id' field, which will be
    // different (null for multiRequestContext.request, non-null for
    // other).

    int                   numOutstandingRequests = -1;
    NodeResponsePairsIter it;
    for (it = multiRequestContext->d_nodeResponsePairs.begin();
         it != multiRequestContext->d_nodeResponsePairs.end();
         ++it) {
        if (node == it->first) {
            it->second             = singleRequestContext->response();
            numOutstandingRequests = --(
                multiRequestContext->d_numOutstandingRequests);
        }
    }

    BSLS_ASSERT_SAFE(0 <= numOutstandingRequests);
    if (0 == numOutstandingRequests) {
        multiRequestContext->d_responseCb(multiRequestContext);
    }
}

// CREATORS
template <class REQUEST, class RESPONSE, class TARGET>
MultiRequestManager<REQUEST, RESPONSE, TARGET>::MultiRequestManager(
    bmqp::RequestManager<REQUEST, RESPONSE>* requestManager,
    bslma::Allocator*                        allocator)
: d_requestManager_p(requestManager)
, d_requestContextPool(
      bdlf::BindUtil::bind(&MultiRequestManager::poolCreateRequestContext,
                           this,
                           bdlf::PlaceHolders::_1,   // address
                           bdlf::PlaceHolders::_2),  // allocator
      -1,                                            // geometric growth
      allocator)
{
    BSLS_ASSERT_SAFE(d_requestManager_p);
}

template <class REQUEST, class RESPONSE, class TARGET>
MultiRequestManager<REQUEST, RESPONSE, TARGET>::~MultiRequestManager()
{
    BSLS_ASSERT_SAFE((d_requestContextPool.numAvailableObjects() ==
                      d_requestContextPool.numObjects()) &&
                     "There are still outstanding requests");
}

// MANIPULATORS
template <class REQUEST, class RESPONSE, class TARGET>
typename MultiRequestManager<REQUEST, RESPONSE, TARGET>::RequestContextSp
MultiRequestManager<REQUEST, RESPONSE, TARGET>::createRequestContext()
{
    return d_requestContextPool.getObject();
}

template <class REQUEST, class RESPONSE, class TARGET>
void MultiRequestManager<REQUEST, RESPONSE, TARGET>::sendRequest(
    const RequestContextSp& context,
    bsls::TimeInterval      timeout)
{
    NodeResponsePairs& nodeResponsePairs = context->d_nodeResponsePairs;
    int                numRequests       = context->d_numOutstandingRequests;

    for (NodeResponsePairsIter it = nodeResponsePairs.begin();
         it != nodeResponsePairs.end();
         ++it) {
        typename RequestManagerType::RequestSp singleRequestCtx =
            d_requestManager_p->createRequest();
        singleRequestCtx->request() = context->d_request;
        singleRequestCtx->setResponseCb(
            bdlf::BindUtil::bind(&MultiRequestManager::onSingleResponse,
                                 this,
                                 bdlf::PlaceHolders::_1,
                                 context,
                                 it->first));
        setGroupId(singleRequestCtx, it->first);

        bsl::string               errorDescription;
        bmqt::GenericResult::Enum sendRc = d_requestManager_p->sendRequest(
            singleRequestCtx,
            sendFn(it->first),
            targetDescription(it->first),
            timeout,
            &errorDescription);
        if (bmqt::GenericResult::e_SUCCESS != sendRc) {
            BSLS_ASSERT_SAFE(numRequests);

            numRequests = --(context->d_numOutstandingRequests);

            bmqp_ctrlmsg::Status& failure = it->second.choice().makeStatus();
            const int             rc = bmqp_ctrlmsg::StatusCategory::fromInt(
                &failure.category(),
                static_cast<int>(sendRc));
            BSLS_ASSERT_SAFE(rc == 0);
            (void)rc;  // compiler happiness
            failure.code() = static_cast<int>(sendRc);

            bmqu::MemOutStream errorMsg;
            errorMsg << "Unable to send request to '"
                     << targetDescription(it->first)
                     << "' [reason: " << errorMsg.str()
                     << "]: " << singleRequestCtx->request();
            failure.message() = errorMsg.str();
        }
    }

    if (0 == numRequests) {
        context->d_responseCb(context);
    }
}

template <class REQUEST, class RESPONSE, class TARGET>
inline void MultiRequestManager<REQUEST, RESPONSE, TARGET>::processResponse(
    const bmqp_ctrlmsg::ControlMessage& response)
{
    d_requestManager_p->processResponse(response);
};

template <class REQUEST, class RESPONSE, class TARGET>
inline const bsl::string&
MultiRequestManager<REQUEST, RESPONSE, TARGET>::targetDescription(
    const mqbnet::ClusterNode* target) const
{
    return target->nodeDescription();
}

template <class REQUEST, class RESPONSE, class TARGET>
inline const bsl::string&
MultiRequestManager<REQUEST, RESPONSE, TARGET>::targetDescription(
    const bsl::shared_ptr<mqbnet::Session>& target) const
{
    return target->clusterNode() ? target->clusterNode()->nodeDescription()
                                 : target->description();
}

template <class REQUEST, class RESPONSE, class TARGET>
inline void MultiRequestManager<REQUEST, RESPONSE, TARGET>::setGroupId(
    typename RequestManagerType::RequestSp& context,
    const mqbnet::ClusterNode*              target) const
{
    context->setGroupId(target->nodeId());
}

template <class REQUEST, class RESPONSE, class TARGET>
inline void MultiRequestManager<REQUEST, RESPONSE, TARGET>::setGroupId(
    BSLS_ANNOTATION_UNUSED typename RequestManagerType::RequestSp& context,
    BSLS_ANNOTATION_UNUSED const bsl::shared_ptr<mqbnet::Session>& target)
    const
{
    // NOTHING
}

template <class REQUEST, class RESPONSE, class TARGET>
inline const typename MultiRequestManager<REQUEST, RESPONSE, TARGET>::
    RequestManagerType::SendFn
    MultiRequestManager<REQUEST, RESPONSE, TARGET>::sendFn(
        mqbnet::ClusterNode* target) const
{
    return bdlf::BindUtil::bind(&mqbnet::ClusterNode::write,
                                target,
                                bdlf::PlaceHolders::_1,
                                bmqp::EventType::e_CONTROL);
}

template <class REQUEST, class RESPONSE, class TARGET>
inline const typename MultiRequestManager<REQUEST, RESPONSE, TARGET>::
    RequestManagerType::SendFn
    MultiRequestManager<REQUEST, RESPONSE, TARGET>::sendFn(
        const bsl::shared_ptr<mqbnet::Session>& target) const
{
    return bdlf::BindUtil::bind(&sendHelper,
                                target->channel().get(),
                                bdlf::PlaceHolders::_1);  // blob
}

}  // close package namespace
}  // close enterprise namespace

#endif
