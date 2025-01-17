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

// mqbblp_messagegroupidmanager.h                                     -*-C++-*-
#ifndef INCLUDED_MQBBLP_MESSAGEGROUPIDMANAGER
#define INCLUDED_MQBBLP_MESSAGEGROUPIDMANAGER

/// @file mqbblp_messagegroupidmanager.h
///
/// @brief Provide a class that manages Message Group Id/Handle mapping.
///
/// This component provides a class @bbref{mqbblp::MessageGroupIdManager} that
/// manages the dynamic mapping between Message Group Ids and Handles.  This is
/// necessary in order to support message grouping routing capabilities.  Each
/// Message Group Id gets mapped to a single handle and a handle can possibly
/// have many Message Group Ids mapped to it.  There are configurable rules
/// regarding Garbage Collection of those mappings.  Some of them might be
/// short lived while others more long lived.  Maximum number of message group
/// id mappings might also be limited in which case, old mappings might be
/// discarded in favor of more recent ones.
///
/// Garbage Collection happens after a timeout period configured via the
/// `timeout` constructor argument.  There's also the `maxMsgGroupIds`
/// constructor argument which triggers GC of the least recently used Message
/// Group Id if we are in rebalance mode.  Rebalance is the operation of moving
/// Message Group Ids from one Handle to another and occurs when a new Handle
/// is added.  This will get excessive Message Group Ids from Handles with more
/// than average Message Group Ids and re-distribute them to the other Handles.

// MQB
#include <mqbi_queue.h>

// BMQ
#include <bmqp_protocol.h>

// BDE
#include <bsl_ostream.h>
#include <bsl_set.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace mqbcmd {
class MessageGroupIdHelper;
}

namespace mqbblp {

// ---------------------------
// class MessageGroupIdManager
// ---------------------------

/// Manages Message Group Id/Handle mapping.
class MessageGroupIdManager {
  public:
    // TYPES

    /// @bbref{mqbi::QueueHandle} used as `Handle`.
    typedef mqbi::QueueHandle* Handle;

    /// The type of the identifier of the Message Group Id.
    typedef bmqp::Protocol::MsgGroupId MsgGroupId;

    /// `Int64` used as time.
    typedef bsls::Types::Int64 Time;

    /// A set of Message Group Ids.
    typedef bsl::set<MsgGroupId> IdsForHandle;

    /// Currently supported modes for rebalance functionality.
    enum Rebalance { k_REBALANCE_ON, k_REBALANCE_OFF };

  private:
    // PRIVATE TYPES

    /// Stores and provides operations on Handles and Message Group Ids.
    class Index;

  private:
    // DATA

    /// The allocator to use.
    bslma::Allocator* d_allocator_p;

    /// The timeout for the mappings.
    const Time d_timeout;

    /// The maximum number of mappings.  If it's negative or zero, then they're
    /// unlimited.
    const int d_maxMsgGroupIds;

    /// The rebalance policy used.
    const Rebalance d_rebalance;

    /// Message Group Ids and Handles container.
    bslma::ManagedPtr<Index> d_index;

  private:
    // PRIVATE MANIPULATORS

    /// Clears any records that have expired at the specified `now` time.
    void clearExpired(const Time& now);

    // PRIVATE ACCESSORS

    /// Returns `true` if the number of mappings would be exceeded for the
    /// given configuration with the addition of the specified `extraSpace`,
    /// or `false` otherwise.
    bool exceedsMappingsLimit(const int extraSpace) const;

  private:
    // NOT IMPLEMENTED
    MessageGroupIdManager(const MessageGroupIdManager&);  // = delete
    MessageGroupIdManager&
    operator=(const MessageGroupIdManager&);  // = delete

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(MessageGroupIdManager,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `MessageGroupIdManager` with the specified `timeout`,
    /// `maxMsgGroupIds`, `rebalance` and `allocator`.
    MessageGroupIdManager(const Time&       timeout,
                          const int         maxMsgGroupIds,
                          const Rebalance   rebalance,
                          bslma::Allocator* allocator);

    // MANIPULATORS

    /// Returns the `Handle` for a specified `msgGroupId` as defined by
    /// currently available mappings.  If a mapping isn't available, one
    /// will be created.  In the process mappings might be discarded if they
    /// timeout for the specified `now` time.  The behavior is undefined if
    /// there are no handles available.
    Handle getHandle(const MsgGroupId& msgGroupId, const Time& now);

    /// Add the specified `handle` to the set of available handles.  If the
    /// rebalance mode is enabled, existing Message Group Ids will be
    /// allocated to this `handle`.  In the process mappings might be
    /// discarded if they expire before or at the specified `now` time.
    void addHandle(const Handle& handle, const Time& now);

    /// Remove the specified `handle` from the set of available handles.
    /// Regardless of rebalance mode, all the existing mappings for this
    /// handle will be discarded.
    void removeHandle(const Handle& handle);

    // ACCESSORS

    /// Load into the specified `ids` the Message Group Ids for the
    /// specified `handle`.  Note that this method copies (potentially many)
    /// `MsgGroupId`s that are `bsl::string`s, so it could be slow.
    void idsForHandle(IdsForHandle* ids, const Handle& handle) const;

    /// Returns the timeout.
    Time timeout() const;

    /// Returns the number of known Message Group Ids.
    int msgGroupIdsCount() const;

    /// Returns the number of known handles.
    int handlesCount() const;

    /// Returns the maximum number of Message Group Ids managed by this
    /// instance.
    int maxMsgGroupIds() const;

    /// Returns `true` if rebalance mode is enabled and `false` otherwise.
    bool isRebalance() const;

    /// Returns `true` if the current number of mappings exceeds the
    /// configured one.  This might be the case when rebalance is not
    /// enabled.
    bool exceedsMaxMsgGroupIdsLimit() const;

    /// Load into the specified `out` object the internal details about this
    /// Message Group Id Manager.  The time will be shown as a delta from
    /// the specified `now` time.
    void loadInternals(mqbcmd::MessageGroupIdHelper* out,
                       const Time&                   now) const;
};

}  // close package namespace
}  // close enterprise namespace

#endif
