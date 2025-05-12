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

// bmqp_queueutil.cpp                                               -*-C++-*-
#include <bmqp_queueutil.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqt_queueflags.h>
#include <bmqt_uri.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bsla_annotations.h>
#include <bslma_default.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqp {

// ----------------
// struct QueueUtil
// ----------------

bool QueueUtil::isValid(const bmqp_ctrlmsg::QueueHandleParameters& parameters)
{
    if (bmqt::QueueFlagsUtil::isReader(parameters.flags())) {
        if (parameters.readCount() <= 0) {
            return false;  // RETURN
        }
    }

    if (bmqt::QueueFlagsUtil::isWriter(parameters.flags())) {
        if (parameters.writeCount() <= 0) {
            return false;  // RETURN
        }
    }

    if (bmqt::QueueFlagsUtil::isAdmin(parameters.flags())) {
        if (parameters.adminCount() <= 0) {
            return false;  // RETURN
        }
    }

    return true;
}

bool QueueUtil::isValidSubset(
    const bmqp_ctrlmsg::QueueHandleParameters& subset,
    const bmqp_ctrlmsg::QueueHandleParameters& aggregate)
{
    if (bmqt::QueueFlagsUtil::isReader(subset.flags())) {
        if (subset.readCount() > aggregate.readCount()) {
            return false;  // RETURN
        }
    }

    if (bmqt::QueueFlagsUtil::isWriter(subset.flags())) {
        if (subset.writeCount() > aggregate.writeCount()) {
            return false;  // RETURN
        }
    }

    if (bmqt::QueueFlagsUtil::isAdmin(subset.flags())) {
        if (subset.adminCount() > aggregate.adminCount()) {
            return false;  // RETURN
        }
    }

    return true;
}

void QueueUtil::mergeHandleParameters(
    bmqp_ctrlmsg::QueueHandleParameters*       out,
    const bmqp_ctrlmsg::QueueHandleParameters& in)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out && "'out' must be specified");
    BSLS_ASSERT_SAFE(isValid(in));
    // NOTE: The uris of 'out' and 'in' can differ due to the presence of
    //       'appIds'

    bsls::Types::Uint64 outFlags = out->flags();

    if (bmqt::QueueFlagsUtil::isReader(in.flags())) {
        bmqt::QueueFlagsUtil::setReader(&outFlags);
        out->readCount() += in.readCount();
    }

    if (bmqt::QueueFlagsUtil::isWriter(in.flags())) {
        bmqt::QueueFlagsUtil::setWriter(&outFlags);
        out->writeCount() += in.writeCount();
    }

    if (bmqt::QueueFlagsUtil::isAdmin(in.flags())) {
        bmqt::QueueFlagsUtil::setAdmin(&outFlags);
        out->adminCount() += in.adminCount();
    }

    out->flags() = outFlags;
}

void QueueUtil::subtractHandleParameters(
    bmqp_ctrlmsg::QueueHandleParameters*       out,
    const bmqp_ctrlmsg::QueueHandleParameters& in)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out && "'out' must be specified");
    BSLS_ASSERT_SAFE(isValidSubset(in, *out));
    // NOTE: The uris of 'out' and 'in' can differ due to the presence of
    //       'appIds'

    bsls::Types::Uint64 outFlags = out->flags();

    if (bmqt::QueueFlagsUtil::isReader(in.flags())) {
        out->readCount() -= in.readCount();
        if (out->readCount() == 0) {
            bmqt::QueueFlagsUtil::unsetReader(&outFlags);
        }
    }

    if (bmqt::QueueFlagsUtil::isWriter(in.flags())) {
        out->writeCount() -= in.writeCount();
        if (out->writeCount() == 0) {
            bmqt::QueueFlagsUtil::unsetWriter(&outFlags);
        }
    }

    if (bmqt::QueueFlagsUtil::isAdmin(in.flags())) {
        out->adminCount() -= in.adminCount();
        if (out->adminCount() == 0) {
            bmqt::QueueFlagsUtil::unsetAdmin(&outFlags);
        }
    }

    out->flags() = outFlags;
}

void QueueUtil::subtractHandleParameters(
    bmqp_ctrlmsg::QueueHandleParameters*       out,
    const bmqp_ctrlmsg::QueueHandleParameters& in,
    bool                                       isFinal)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out && "'out' must be specified");
    BSLS_ASSERT_SAFE(isValidSubset(in, *out));
    // NOTE: The uris of 'out' and 'in' can differ due to the presence of
    //       'appIds'

    if (isFinal) {
        out->reset();
        return;  // RETURN
    }

    subtractHandleParameters(out, in);
}

bmqp_ctrlmsg::QueueHandleParameters QueueUtil::createHandleParameters(
    const bmqp_ctrlmsg::QueueHandleParameters& handleParameters,
    const bmqp_ctrlmsg::SubQueueIdInfo&        subStreamInfo,
    int                                        readCount)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(subStreamInfo.appId() !=
                     bmqp::ProtocolUtil::k_NULL_APP_ID);

    bmqp_ctrlmsg::QueueHandleParameters result;
    result.qId()        = handleParameters.qId();
    result.flags()      = 0;
    result.writeCount() = 0;
    result.readCount()  = readCount;
    result.adminCount() = 0;

    if (readCount) {
        bmqt::QueueFlagsUtil::setReader(&result.flags());
    }

    if (isDefaultSubstream(subStreamInfo)) {
        result.uri() = handleParameters.uri();

        if (handleParameters.writeCount()) {
            BSLS_ASSERT_SAFE(
                bmqt::QueueFlagsUtil::isWriter(handleParameters.flags()));
            bmqt::QueueFlagsUtil::setWriter(&result.flags());
            result.writeCount() = handleParameters.writeCount();

            if (bmqt::QueueFlagsUtil::isAck(handleParameters.flags())) {
                bmqt::QueueFlagsUtil::setAck(&result.flags());
            }
        }
    }
    else {
        // Set the consumer's subStream info (subId, appId)
        result.subIdInfo().makeValue(subStreamInfo);

        // Set the full 'uri' (including appId)
        bmqt::Uri uri(handleParameters.uri());
        BSLS_ASSERT_SAFE(uri.isValid());
        BSLS_ASSERT_SAFE(handleParameters.uri() == uri.canonical() &&
                         "'uri' of 'queueHandleParameters' must be canonical");

        bmqt::UriBuilder uriBuilder(uri);
        uriBuilder.setId(subStreamInfo.appId());
        BSLA_MAYBE_UNUSED int rc = uriBuilder.uri(&uri, 0);
        BSLS_ASSERT_SAFE(rc == 0);

        result.uri() = uri.asString();
    }

    return result;
}

bmqp_ctrlmsg::QueueHandleParameters&
QueueUtil::extractCanonicalHandleParameters(
    bmqp_ctrlmsg::QueueHandleParameters*       canonicalHandleParameters,
    const bmqp_ctrlmsg::QueueHandleParameters& queueHandleParameters)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(canonicalHandleParameters &&
                     "'canonicalHandleParameters' must be sepcified");

    (*canonicalHandleParameters) = queueHandleParameters;

    // Replace the 'uri' with its canonical representation
    bdlma::LocalSequentialAllocator<1024> lsa(bslma::Default::allocator());
    bmqt::Uri uri(queueHandleParameters.uri(), &lsa);
    BSLS_ASSERT_SAFE(uri.isValid());

    canonicalHandleParameters->uri() = uri.canonical();

    // Exclude the subStream information
    canonicalHandleParameters->subIdInfo().reset();

    return *canonicalHandleParameters;
}

}  // close package namespace
}  // close enterprise namespace
