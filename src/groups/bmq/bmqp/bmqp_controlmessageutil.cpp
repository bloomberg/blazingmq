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

// bmqp_controlmessageutil.cpp                                        -*-C++-*-
#include <bmqp_controlmessageutil.h>

#include <bmqscm_version.h>
// BDE
#include <bsls_assert.h>
#include <bsls_performancehint.h>

namespace BloombergLP {
namespace bmqp {

// -------------------------
// struct ControlMessageUtil
// -------------------------

// CLASS METHODS
void ControlMessageUtil::makeStatus(
    bmqp_ctrlmsg::ControlMessage*       controlMessage,
    bmqp_ctrlmsg::StatusCategory::Value category,
    int                                 code,
    const bslstl::StringRef&            message)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(controlMessage);

    bmqp_ctrlmsg::Status& status = controlMessage->choice().makeStatus();
    status.category()            = category;
    status.code()                = code;
    status.message().assign(message.data(), message.length());
}

int ControlMessageUtil::validate(
    const bmqp_ctrlmsg::ControlMessage& controlMessage)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS = 0  // Success
        ,
        rc_INVALID_ID = -1  // Invalid id
        ,
        rc_INVALID_CHOICE_SELECTION = -2  // Invalid choice selection
    };

    if (controlMessage.choice().isClusterMessageValue()) {
        // ClusterMessages are permitted to not have an id
        return rc_SUCCESS;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(controlMessage.rId().isNull() ||
                                              controlMessage.rId().value() <
                                                  0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INVALID_ID;  // RETURN
    }

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
            controlMessage.choice().isUndefinedValue())) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        return rc_INVALID_CHOICE_SELECTION;  // RETURN
    }

    return rc_SUCCESS;
}

}  // close package namespace
}  // close enterprise namespace
