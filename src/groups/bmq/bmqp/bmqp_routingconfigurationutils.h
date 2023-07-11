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

// bmqp_routingconfigurationutils.h                                   -*-C++-*-
#ifndef INCLUDED_BMQP_ROUTINGCONFIGURATIONUTILS
#define INCLUDED_BMQP_ROUTINGCONFIGURATIONUTILS

//@PURPOSE: Provide utility methods to manage 'RoutingConfiguration' instances.
//
//@CLASSES:
//  bmqp::RoutingConfigurationUtils: Utilities for 'RoutingConfiguration'
//
//@DESCRIPTION: 'bmqp::RoutingConfigurationUtils' is a class that provides
// utility functions to query, set and unset properties of a given 'Flags'.
// Many combinations of states are expected to be redundant or invalid, and by
// using this class, we avoid the maintenance costs associated with operating
// on the flags directly.

// BMQ

#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqp {

// ===============================
// class RoutingConfigurationUtils
// ===============================

/// Provide utility methods to manage `RoutingConfiguration` instances.
class RoutingConfigurationUtils {
  private:
    // PRIVATE TYPES
    typedef bmqp_ctrlmsg::RoutingConfigurationFlags Flags;

    enum Enum {
        // Enum that converts from bit position to bit mask.  You shouldn't
        // need to use this enum directly.  'RoutingConfigurationUtils''s
        // purpose is to hide those details.

        e_AT_MOST_ONCE              = (1 << Flags::E_AT_MOST_ONCE),
        e_DELIVER_CONSUMER_PRIORITY = (1
                                       << Flags::E_DELIVER_CONSUMER_PRIORITY),
        e_DELIVER_ALL               = (1 << Flags::E_DELIVER_ALL),
        e_HAS_MULTIPLE_SUB_STREAMS  = (1 << Flags::E_HAS_MULTIPLE_SUB_STREAMS)
    };

  public:
    // CLASS METHODS

    /// Return true if the specified `config` is empty/clear or false
    /// otherwise.
    static bool isClear(const bmqp_ctrlmsg::RoutingConfiguration& config);

    /// Reset the value of the specified `config`.
    static void clear(bmqp_ctrlmsg::RoutingConfiguration* config);

    /// Set the non-confirmable property of the specified `config`.  This
    /// indicates that this domain won't confirm messages.  This property
    /// implies that no store will be used.
    static void setAtMostOnce(bmqp_ctrlmsg::RoutingConfiguration* config);

    /// Set the deliver-to-all property of the specified `config`.  This
    /// flag indicates that the domain requires relay nodes to deliver any
    /// given message to every available downstream consumer.
    static void setDeliverAll(bmqp_ctrlmsg::RoutingConfiguration* config);

    /// Set the priority-consumers property of the specified `config`.
    /// This flag indicates that the domain requires relay nodes to deliver
    /// any given message only to highest priority downstream consumers.
    static void
    setDeliverConsumerPriority(bmqp_ctrlmsg::RoutingConfiguration* config);

    /// Set the has-multiple-sub-streams property of the specified
    /// `config`.  This flag indicates that the domain requires relay nodes
    /// to consider multiple substreams when delivering a given message.
    static void
    setHasMultipleSubStreams(bmqp_ctrlmsg::RoutingConfiguration* config);

    /// Unset the non-confirmable property of the specified `config`.  For
    /// more information see `setAtMostOnce()`.
    static void unsetAtMostOnce(bmqp_ctrlmsg::RoutingConfiguration* config);

    /// Unset the deliver-to-all property of the specified `config`.  For
    /// more information see `setDeliverAll()`.
    static void unsetDeliverAll(bmqp_ctrlmsg::RoutingConfiguration* config);

    /// Unset the priority-consumers property of the specified `config`.
    /// For more information see `setDeliverConsumerPriority()`.
    static void
    unsetDeliverConsumerPriority(bmqp_ctrlmsg::RoutingConfiguration* config);

    /// Unset the has-multiple-sub-streams property of the specified
    /// `config`.  For more information see `setHasMultipleSubStreams()`.
    static void
    unsetHasMultipleSubStreams(bmqp_ctrlmsg::RoutingConfiguration* config);

    /// Returns `true` if the configuration  of the specified `config`
    /// defines at-most-once semantics or `false` otherwise.
    static bool isAtMostOnce(const bmqp_ctrlmsg::RoutingConfiguration& config);

    /// Return `true` if the deliver-to-all property of the specified
    /// `config` is set or `false` otherwise.
    static bool isDeliverAll(const bmqp_ctrlmsg::RoutingConfiguration& config);

    /// Return `true` if the priority-consumers property of the specified
    /// `config` is set or `false` otherwise.
    static bool isDeliverConsumerPriority(
        const bmqp_ctrlmsg::RoutingConfiguration& config);

    /// Return `true` if the has-multiple-sub-streams property of the
    /// specified `config` is set or `false` otherwise.
    static bool
    hasMultipleSubStreams(const bmqp_ctrlmsg::RoutingConfiguration& config);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------------------
// class RoutingConfigurationUtils
// -------------------------------

inline bool RoutingConfigurationUtils::isClear(
    const bmqp_ctrlmsg::RoutingConfiguration& config)
{
    return (config.flags() == 0);
}

inline void
RoutingConfigurationUtils::clear(bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);

    config->flags() = 0;
}

inline void RoutingConfigurationUtils::setAtMostOnce(
    bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);

    config->flags() |= e_AT_MOST_ONCE;
}

inline void RoutingConfigurationUtils::setDeliverAll(
    bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);

    config->flags() |= e_DELIVER_ALL;
}

inline void RoutingConfigurationUtils::setDeliverConsumerPriority(
    bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);

    config->flags() |= e_DELIVER_CONSUMER_PRIORITY;
}

inline void RoutingConfigurationUtils::setHasMultipleSubStreams(
    bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);

    config->flags() |= e_HAS_MULTIPLE_SUB_STREAMS;
}

inline void RoutingConfigurationUtils::unsetAtMostOnce(
    bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);

    config->flags() &= ~e_AT_MOST_ONCE;
}

inline void RoutingConfigurationUtils::unsetDeliverAll(
    bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);

    config->flags() &= ~e_DELIVER_ALL;
}

inline void RoutingConfigurationUtils::unsetDeliverConsumerPriority(
    bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);

    config->flags() &= ~e_DELIVER_CONSUMER_PRIORITY;
}

inline void RoutingConfigurationUtils::unsetHasMultipleSubStreams(
    bmqp_ctrlmsg::RoutingConfiguration* config)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(config);

    config->flags() &= ~e_HAS_MULTIPLE_SUB_STREAMS;
}

inline bool RoutingConfigurationUtils::isAtMostOnce(
    const bmqp_ctrlmsg::RoutingConfiguration& config)
{
    return ((config.flags() & e_AT_MOST_ONCE) != 0);
}

inline bool RoutingConfigurationUtils::isDeliverAll(
    const bmqp_ctrlmsg::RoutingConfiguration& config)
{
    return ((config.flags() & e_DELIVER_ALL) != 0);
}

inline bool RoutingConfigurationUtils::isDeliverConsumerPriority(
    const bmqp_ctrlmsg::RoutingConfiguration& config)
{
    return ((config.flags() & e_DELIVER_CONSUMER_PRIORITY) != 0);
}

inline bool RoutingConfigurationUtils::hasMultipleSubStreams(
    const bmqp_ctrlmsg::RoutingConfiguration& config)
{
    return ((config.flags() & e_HAS_MULTIPLE_SUB_STREAMS) != 0);
}

}  // close package namespace
}  // close enterprise namespace

#endif
