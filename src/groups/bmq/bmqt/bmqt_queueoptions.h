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

// bmqt_queueoptions.h                                                -*-C++-*-
#ifndef INCLUDED_BMQT_QUEUEOPTIONS
#define INCLUDED_BMQT_QUEUEOPTIONS

//@PURPOSE: Provide a value-semantic type for options related to a queue.
//
//@CLASSES:
//  bmqt::QueueOptions: options related to a queue.
//
//@DESCRIPTION: 'bmqt::QueueOptions' provides a value-semantic type,
// 'QueueOptions', which is used to specify parameters for a queue.
//
//
// The following parameters are supported:
//: o !maxUnconfirmedMessages!:
//:      Maximum number of outstanding messages that can be sent by the broker
//:      without being confirmed.
//:
//: o !maxUnconfirmedBytes!:
//:      Maximum accumulated bytes of all outstanding messages that can be sent
//:      by the broker without being confirmed.
//:
//: o !consumerPriority!:
//:      Priority of a consumer with respect to delivery of messages.
//:
//: o !suspendsOnBadHostHealth!:
//:      Sets whether the queue should suspend operation when the host machine
//:      is unhealthy.

// BMQ

#include <bmqt_subscription.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_optional.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

namespace bmqt {

// ==================
// class QueueOptions
// ==================

/// Value-semantic type for options related to a queue.
class QueueOptions {
  public:
    // PUBLIC CONSTANTS
    static const int k_CONSUMER_PRIORITY_MIN;
    // Constant representing the minimum
    // valid consumer priority

    static const int k_CONSUMER_PRIORITY_MAX;
    // Constant representing the maximum
    // valid consumer priority

    static const int  k_DEFAULT_MAX_UNCONFIRMED_MESSAGES;
    static const int  k_DEFAULT_MAX_UNCONFIRMED_BYTES;
    static const int  k_DEFAULT_CONSUMER_PRIORITY;
    static const bool k_DEFAULT_SUSPENDS_ON_BAD_HOST_HEALTH;

  private:
    // PRIVATE TYPES
    typedef bsl::unordered_map<SubscriptionHandle, Subscription> Subscriptions;

  private:
    // DATA
    Subscription d_info;

    bsl::optional<bool> d_suspendsOnBadHostHealth;
    // Whether the queue suspends operation
    // while the host is unhealthy.

    Subscriptions d_subscriptions;

    bool d_hadSubscriptions;
    // 'true' if 'd_subscriptions' had a value, 'false'
    // otherwise.  Emulates 'bsl::optional' for
    // 'd_subscriptions'.

    bslma::Allocator* d_allocator_p;
    // Allocator

  public:
    // PUBLIC TYPES

    typedef bsl::pair<SubscriptionHandle, Subscription> HandleAndSubscription;

    /// `loadSubscriptions` return types
    ///
    /// EXPERIMENTAL.  Do not use until this feature is announced.
    typedef bsl::vector<HandleAndSubscription> SubscriptionsSnapshot;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(QueueOptions, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new QueueOptions using the optionally specified
    /// `allocator`.
    explicit QueueOptions(bslma::Allocator* allocator = 0);

    /// Create a new QueueOptions by copying values from the specified
    /// `other`, using the optionally specified `allocator`.
    QueueOptions(const QueueOptions& other, bslma::Allocator* allocator = 0);

    // MANIPULATORS

    /// Set the maxUnconfirmedMessages to the specified `value`.  The
    /// behavior is undefined unless `value >= 0`. If the specified `value`
    /// is set to 0, it means that the consumer does not receive any
    /// messages.  This might be useful when the consumer is shutting down
    /// and wants to process only pending messages, or when it is unable to
    /// process new messages because of transient issues.
    QueueOptions& setMaxUnconfirmedMessages(int value);

    /// Set the maxUnconfirmedBytes to the specified `value`.  The behavior
    /// is undefined unless `value >= 0`.
    QueueOptions& setMaxUnconfirmedBytes(int value);

    /// Set the consumerPriority to the specified `value`.  The behavior is
    /// undefined unless 'k_CONSUMER_PRIORITY_MIN <= value <=
    /// k_CONSUMER_PRIORITY_MAX'
    QueueOptions& setConsumerPriority(int value);

    /// Set whether the queue suspends operation while host is unhealthy.
    QueueOptions& setSuspendsOnBadHostHealth(bool value);

    /// "Merges" another `QueueOptions` into this one, by invoking
    ///     setF(other.F())
    /// for all fields `F` for which `other.hasF()` is true.  Returns the
    /// instance on which the method was invoked.
    QueueOptions& merge(const QueueOptions& other);

    /// Add, or update if it exists, the specified `subscription` for the
    /// specified `handle`.  Return true on success, otherwise return false
    /// and load the specified `errorDescription` with a description of the
    /// error.  Note that `errorDescription` may be null if the caller does
    /// not care about getting error messages, but users are strongly
    /// encouraged to log error string if this API returns failure.
    ///
    /// EXPERIMENTAL.  Do not use until this feature is announced.
    bool addOrUpdateSubscription(bsl::string*              errorDescription,
                                 const SubscriptionHandle& handle,
                                 const Subscription&       subscription);

    /// Return false if subscription does not exist.
    ///
    /// EXPERIMENTAL.  Do not use until this feature is announced.
    bool removeSubscription(const SubscriptionHandle& handle);

    /// Remove all subscriptions.
    ///
    /// EXPERIMENTAL.  Do not use until this feature is announced.
    void removeAllSubscriptions();

    // ACCESSORS

    /// Get the number for the maxUnconfirmedMessages parameter.
    int maxUnconfirmedMessages() const;

    /// Get the number for the maxUnconfirmedBytes parameter.
    int maxUnconfirmedBytes() const;

    /// Get the number for the consumerPriority parameter.
    int consumerPriority() const;

    /// Get whether the queue suspends operation while host is unhealthy.
    bool suspendsOnBadHostHealth() const;

    /// Returns whether `maxUnconfirmedMessages` has been set for this
    /// object, or whether it implicitly holds
    /// `k_DEFAULT_MAX_UNCONFIRMED_MESSAGES`.
    bool hasMaxUnconfirmedMessages() const;

    /// Returns whether `maxUnconfirmedBytes` has been set for this object,
    /// or whether it implicitly holds `k_DEFAULT_MAX_UNCONFIRMED_BYTES`.
    bool hasMaxUnconfirmedBytes() const;

    /// Returns whether `consumerPriority` has been set for this object, or
    /// whether it implicitly holds `k_DEFAULT_CONSUMER_PRIORITY`.
    bool hasConsumerPriority() const;

    /// Returns whether `suspendsOnBadHostHealth` has been set for this
    /// object, or whether it implicitly holds
    /// `k_DEFAULT_SUSPENDS_ON_BAD_HOST_HEALTH`.
    bool hasSuspendsOnBadHostHealth() const;

    /// Return false if subscription does not exist.
    ///
    /// EXPERIMENTAL.  Do not use until this feature is announced.
    bool loadSubscription(Subscription*             subscription,
                          const SubscriptionHandle& handle) const;

    /// Load all handles and subscriptions into the specified `snapshot`.
    ///
    /// EXPERIMENTAL.  Do not use until this feature is announced.
    void loadSubscriptions(SubscriptionsSnapshot* snapshot) const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Return `true` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return false otherwise.
bool operator==(const QueueOptions& lhs, const QueueOptions& rhs);

/// Return `false` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return `true` otherwise.
bool operator!=(const QueueOptions& lhs, const QueueOptions& rhs);

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const QueueOptions& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// class QueueOptions
// ------------------

// MANIPULATORS
inline QueueOptions& QueueOptions::setMaxUnconfirmedMessages(int value)
{
    d_info.setMaxUnconfirmedMessages(value);
    return *this;
}

inline QueueOptions& QueueOptions::setMaxUnconfirmedBytes(int value)
{
    d_info.setMaxUnconfirmedBytes(value);
    return *this;
}

inline QueueOptions& QueueOptions::setConsumerPriority(int value)
{
    d_info.setConsumerPriority(value);
    return *this;
}

inline QueueOptions& QueueOptions::setSuspendsOnBadHostHealth(bool value)
{
    d_suspendsOnBadHostHealth.emplace(value);
    return *this;
}

// ACCESSORS
inline int QueueOptions::maxUnconfirmedMessages() const
{
    return d_info.maxUnconfirmedMessages();
}

inline int QueueOptions::maxUnconfirmedBytes() const
{
    return d_info.maxUnconfirmedBytes();
}

inline int QueueOptions::consumerPriority() const
{
    return d_info.consumerPriority();
}

inline bool QueueOptions::suspendsOnBadHostHealth() const
{
    return d_suspendsOnBadHostHealth.value_or(
        k_DEFAULT_SUSPENDS_ON_BAD_HOST_HEALTH);
}

inline bool QueueOptions::hasMaxUnconfirmedMessages() const
{
    return d_info.hasMaxUnconfirmedMessages();
}

inline bool QueueOptions::hasMaxUnconfirmedBytes() const
{
    return d_info.hasMaxUnconfirmedBytes();
}

inline bool QueueOptions::hasConsumerPriority() const
{
    return d_info.hasConsumerPriority();
}

inline bool QueueOptions::hasSuspendsOnBadHostHealth() const
{
    return d_suspendsOnBadHostHealth.has_value();
}

}  // close package namespace

// ------------------
// class QueueOptions
// ------------------

inline bool bmqt::operator==(const bmqt::QueueOptions& lhs,
                             const bmqt::QueueOptions& rhs)
{
    return lhs.maxUnconfirmedMessages() == rhs.maxUnconfirmedMessages() &&
           lhs.maxUnconfirmedBytes() == rhs.maxUnconfirmedBytes() &&
           lhs.consumerPriority() == rhs.consumerPriority() &&
           lhs.suspendsOnBadHostHealth() == rhs.suspendsOnBadHostHealth();
}

inline bool bmqt::operator!=(const bmqt::QueueOptions& lhs,
                             const bmqt::QueueOptions& rhs)
{
    return lhs.maxUnconfirmedMessages() != rhs.maxUnconfirmedMessages() ||
           lhs.maxUnconfirmedBytes() != rhs.maxUnconfirmedBytes() ||
           lhs.consumerPriority() != rhs.consumerPriority() ||
           lhs.suspendsOnBadHostHealth() != rhs.suspendsOnBadHostHealth();
}

inline bsl::ostream& bmqt::operator<<(bsl::ostream&             stream,
                                      const bmqt::QueueOptions& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
