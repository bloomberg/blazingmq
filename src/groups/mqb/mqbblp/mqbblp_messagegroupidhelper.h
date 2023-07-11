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

// mqbblp_messagegroupidhelper.h                                      -*-C++-*-
#ifndef INCLUDED_MQBBLP_MESSAGEGROUPIDHELPER
#define INCLUDED_MQBBLP_MESSAGEGROUPIDHELPER

//@PURPOSE: Provide a mechanism to map Message Group Ids to Handles.
//
//@CLASSES:
//  mqbblp::MessageGroupIdHelper: Map Message Group Ids to Handles
//
//@DESCRIPTION: This component provides a class 'mqbblp::MessageGroupIdHelper'
// which given a collection of Handles, and an opaque MsgGroupId, returns a
// QueueHandle.

// MQB

#include <mqbblp_messagegroupidmanager.h>
#include <mqbconfm_messages.h>

// BMQ
#include <bmqp_protocol.h>

// MWC
#include <mwcsys_time.h>

// BDE
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class MessageGroupIdHelper;
}

namespace mqbblp {

// ==========================
// class MessageGroupIdHelper
// ==========================

/// Mechanism to map Message Group Ids to Handles.
class MessageGroupIdHelper {
  private:
    // DATA
    MessageGroupIdManager d_manager;
    // Manager providing the required functionality

  private:
    // NOT IMPLEMENTED
    MessageGroupIdHelper(const MessageGroupIdHelper&);             // = delete
    MessageGroupIdHelper& operator=(const MessageGroupIdHelper&);  // = delete

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MessageGroupIdHelper,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new object using the specified `config` and using the
    /// specified `allocator`.
    MessageGroupIdHelper(const mqbconfm::MsgGroupIdConfig& config,
                         bslma::Allocator*                 allocator);

    // MANIPULATORS

    /// Returns the `Handle` for a specified `msgGroupId` as defined by
    /// currently available mappings.  If a mapping isn't available, one
    /// will be created.
    mqbi::QueueHandle* getHandle(const bmqp::Protocol::MsgGroupId& msgGroupId);

    /// Add the specified `handle` to the set of available handles.  If the
    /// rebalance mode is enabled, existing Message Group Ids will be
    /// allocated to this `handle`.
    void addHandle(mqbi::QueueHandle* handle);

    /// Remove the specified `handle` from the set of available handles.
    /// Regardless of rebalance mode, all the existing mappings for this
    /// handle will be discarded.
    void removeHandle(mqbi::QueueHandle* handle);

    // ACCESSORS

    /// Load into the specified `out` object the internal details about this
    /// Message Group Id Manager.
    void loadInternals(mqbcmd::MessageGroupIdHelper* out) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------
// class MessageGroupIdHelper
// --------------------------

// CREATORS
inline MessageGroupIdHelper::MessageGroupIdHelper(
    const mqbconfm::MsgGroupIdConfig& config,
    bslma::Allocator*                 allocator)
: d_manager(config.ttlSeconds(),
            config.maxGroups(),
            (config.rebalance() ? MessageGroupIdManager::k_REBALANCE_ON
                                : MessageGroupIdManager::k_REBALANCE_OFF),
            allocator)
{
    // NOTHING
}

// MANIPULATORS
inline mqbi::QueueHandle*
MessageGroupIdHelper::getHandle(const bmqp::Protocol::MsgGroupId& msgGroupId)
{
    return d_manager.getHandle(msgGroupId,
                               mwcsys::Time::highResolutionTimer());
}

inline void MessageGroupIdHelper::addHandle(mqbi::QueueHandle* handle)
{
    d_manager.addHandle(handle, mwcsys::Time::highResolutionTimer());
}

inline void MessageGroupIdHelper::removeHandle(mqbi::QueueHandle* handle)
{
    d_manager.removeHandle(handle);
}

// ACCESSORS
inline void
MessageGroupIdHelper::loadInternals(mqbcmd::MessageGroupIdHelper* out) const
{
    d_manager.loadInternals(out, mwcsys::Time::highResolutionTimer());
}

}  // close package namespace
}  // close enterprise namespace

#endif
