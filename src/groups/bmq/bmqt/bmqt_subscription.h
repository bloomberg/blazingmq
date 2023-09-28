// Copyright 2022-2023 Bloomberg Finance L.P.
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

// bmqt_subscription.h                                                -*-C++-*-
#ifndef INCLUDED_BMQT_SUBSCRIPTION
#define INCLUDED_BMQT_SUBSCRIPTION

//@PURPOSE: Provide a value-semantic types for subscription related API.
//
//@CLASSES:
//  bmqt::SubscriptionHandle:       uniquely identifies Subscription
//  bmqt::SubscriptionExpression:   Subscription criteria
//  bmqt::Subscription:             Subscription parameters
//
//@DESCRIPTION: 'bmqt::Subscription' provides a value-semantic type carried
// by 'bmqt::QueueOptions', when opening and configuring a queue.
//
//@NOTE: Experimental.  Do not use until this feature is announced.
//

// BMQ

#include <bmqt_correlationid.h>

// BDE
#include <bsl_optional.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>
#include <bsls_types.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqa {
class MessageImpl;
}
namespace bmqa {
class MessageIterator;
}

namespace bmqt {

class QueueOptions;
// =========================
// class Subscription_Handle
// =========================

/// Value-semantic type for unique Subscription id.
class SubscriptionHandle {
    friend class bmqa::MessageImpl;
    friend class bmqa::MessageIterator;

  private:
    // PRIVATE DATA
    unsigned int d_id;
    // Internal, unique key

    bmqt::CorrelationId d_correlationId;
    // User-specified, not required to be
    // unique
  private:
    // PRIVATE CLASS METHODS
    static unsigned int nextId();

    // PRIVATE CREATORS
    SubscriptionHandle();
    SubscriptionHandle(unsigned int id, const bmqt::CorrelationId& cid);

  public:
    // CREATORS
    SubscriptionHandle(const bmqt::CorrelationId& cid);

    // ACCESSORS
    const bmqt::CorrelationId& correlationId() const;

    /// Do *NOT* use.  Internal function, reserved for BlazingMQ internal
    /// usage.  It is subject to change without notice.
    unsigned int id() const;

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

    // FRIENDS
    friend bool operator<(const SubscriptionHandle& lhs,
                          const SubscriptionHandle& rhs);
    friend bool operator==(const SubscriptionHandle& lhs,
                           const SubscriptionHandle& rhs);
    friend bool operator!=(const SubscriptionHandle& lhs,
                           const SubscriptionHandle& rhs);

    template <class HASH_ALGORITHM>
    friend void hashAppend(HASH_ALGORITHM&           hashAlgo,
                           const SubscriptionHandle& id);

    // Will only consider 'd_id' field when comparing 'lhs' and 'rhs'
};

/// Value-semantic type to carry Subscription criteria.
class SubscriptionExpression {
  public:
    // TYPES
    enum Enum {
        // Enum representing criteria format
        e_NONE = 0  // EMPTY
        ,
        e_VERSION_1 = 1  // Simple Evaluator
    };

  private:
    bsl::string d_expression;  // e.g., "firmId == foo"

    Enum d_version;  // Required to support newer style
                     // of expressions in future

  public:
    // CREATORS
    SubscriptionExpression();
    SubscriptionExpression(const bsl::string& expression, Enum version);

    // ACCESSORS
    const bsl::string& text() const;

    Enum version() const;

    bool isValid() const;

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

/// Value-semantic type to carry Subscription parameters.
class Subscription {
  public:
    // PUBLIC CONSTANTS
    static const int k_CONSUMER_PRIORITY_MIN;
    // Constant representing the minimum
    // valid consumer priority

    static const int k_CONSUMER_PRIORITY_MAX;
    // Constant representing the maximum
    // valid consumer priority

    static const int k_DEFAULT_MAX_UNCONFIRMED_MESSAGES;
    static const int k_DEFAULT_MAX_UNCONFIRMED_BYTES;
    static const int k_DEFAULT_CONSUMER_PRIORITY;

  private:
    // PRIVATE DATA
    bsl::optional<int> d_maxUnconfirmedMessages;
    // Maximum number of outstanding
    // messages that can be sent by the
    // broker without being confirmed.

    bsl::optional<int> d_maxUnconfirmedBytes;
    // Maximum accumulated bytes of all
    // outstanding messages that can be
    // sent by the broker without being
    // confirmed.

    bsl::optional<int> d_consumerPriority;
    // Priority of a consumer with respect
    // to delivery of messages

    SubscriptionExpression d_expression;

  public:
    // CREATORS
    Subscription();

    // Create a new Subscription

    /// Create a new Subscription by copying values from the specified
    /// `other`.
    Subscription(const Subscription& other);

    // MANIPULATORS

    /// Set the maxUnconfirmedMessages to the specified `value`.  The
    /// behavior is undefined unless `value >= 0`. If the specified `value`
    /// is set to 0, it means that the consumer does not receive any
    /// messages.  This might be useful when the consumer is shutting down
    /// and wants to process only pending messages, or when it is unable to
    /// process new messages because of transient issues.
    Subscription& setMaxUnconfirmedMessages(int value);

    /// Set the maxUnconfirmedBytes to the specified `value`.  The behavior
    /// is undefined unless `value >= 0`.
    Subscription& setMaxUnconfirmedBytes(int value);

    /// Set the consumerPriority to the specified `value`.  The behavior is
    /// undefined unless 'k_CONSUMER_PRIORITY_MIN <= value <=
    /// k_CONSUMER_PRIORITY_MAX'
    Subscription& setConsumerPriority(int value);

    Subscription& setExpression(const SubscriptionExpression& value);

    // ACCESSORS

    /// Get the number for the maxUnconfirmedMessages parameter.
    int maxUnconfirmedMessages() const;

    /// Get the number for the maxUnconfirmedBytes parameter.
    int maxUnconfirmedBytes() const;

    /// Get the number for the consumerPriority parameter.
    int consumerPriority() const;

    const SubscriptionExpression& expression() const;

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

/// Format the specified `rhs` to the specified output `stream` and return a
/// reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const SubscriptionHandle& rhs);
bsl::ostream& operator<<(bsl::ostream& stream, const Subscription& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ----------------------
// class Subscription_Handle
// ----------------------

inline SubscriptionHandle::SubscriptionHandle()
: d_id(0)
, d_correlationId()
{
    // NOTHING
}

inline SubscriptionHandle::SubscriptionHandle(unsigned int               id,
                                              const bmqt::CorrelationId& cid)
: d_id(id)
, d_correlationId(cid)
{
    // NOTHING
}

inline SubscriptionHandle::SubscriptionHandle(const bmqt::CorrelationId& cid)
: d_id(nextId())
, d_correlationId(cid)
{
    // NOTHING
}

// ACCESSORS
inline const bmqt::CorrelationId& SubscriptionHandle::correlationId() const
{
    return d_correlationId;
}

inline unsigned int SubscriptionHandle::id() const
{
    return d_id;
}

inline bsl::ostream& SubscriptionHandle::print(bsl::ostream& stream,
                                               int           level,
                                               int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();

    printer.printAttribute("id", d_id);
    printer.printAttribute("CorrelationId", d_correlationId);

    printer.end();

    return stream;
}
// ----------------------------
// class SubscriptionExpression
// ----------------------------
inline SubscriptionExpression::SubscriptionExpression()
: d_expression()
, d_version(e_NONE)
{
    // NOTHING
}

inline SubscriptionExpression::SubscriptionExpression(
    const bsl::string& expression,
    Enum               version)
: d_expression(expression)
, d_version(version)
{
    // NOTHING
}

inline const bsl::string& SubscriptionExpression::text() const
{
    return d_expression;
}

inline SubscriptionExpression::Enum SubscriptionExpression::version() const
{
    return d_version;
}

inline bool SubscriptionExpression::isValid() const
{
    return d_expression.length() == 0 ? d_version == e_NONE
                                      : d_version > e_NONE;
}

inline bsl::ostream& SubscriptionExpression::print(bsl::ostream& stream,
                                                   int           level,
                                                   int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();

    printer.printAttribute("Expression", d_expression);
    printer.printAttribute("Version", d_version);

    printer.end();

    return stream;
}

// ----------------------
// class Subscription
// ----------------------

inline Subscription::Subscription()
: d_maxUnconfirmedMessages()
, d_maxUnconfirmedBytes()
, d_consumerPriority()
, d_expression()
{
    // NOTHING
}

inline Subscription::Subscription(const Subscription& other)
: d_maxUnconfirmedMessages(other.d_maxUnconfirmedMessages)
, d_maxUnconfirmedBytes(other.d_maxUnconfirmedBytes)
, d_consumerPriority(other.d_consumerPriority)
, d_expression(other.d_expression)
{
    // NOTHING
}

inline bsl::ostream&
Subscription::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("maxUnconfirmedMessages", maxUnconfirmedMessages());
    printer.printAttribute("maxUnconfirmedBytes", maxUnconfirmedBytes());
    printer.printAttribute("consumerPriority", consumerPriority());

    d_expression.print(stream, level, spacesPerLevel);

    printer.end();

    return stream;
}

// MANIPULATORS
inline Subscription& Subscription::setMaxUnconfirmedMessages(int value)
{
    d_maxUnconfirmedMessages.emplace(value);
    return *this;
}

inline Subscription& Subscription::setMaxUnconfirmedBytes(int value)
{
    d_maxUnconfirmedBytes.emplace(value);
    return *this;
}

inline Subscription& Subscription::setConsumerPriority(int value)
{
    d_consumerPriority.emplace(value);
    return *this;
}

inline Subscription&
Subscription::setExpression(const SubscriptionExpression& value)
{
    d_expression = value;
    return *this;
}

// ACCESSORS
inline int Subscription::maxUnconfirmedMessages() const
{
    return d_maxUnconfirmedMessages.value_or(
        k_DEFAULT_MAX_UNCONFIRMED_MESSAGES);
}

inline int Subscription::maxUnconfirmedBytes() const
{
    return d_maxUnconfirmedBytes.value_or(k_DEFAULT_MAX_UNCONFIRMED_BYTES);
}

inline int Subscription::consumerPriority() const
{
    return d_consumerPriority.value_or(k_DEFAULT_CONSUMER_PRIORITY);
}

inline const SubscriptionExpression& Subscription::expression() const
{
    return d_expression;
}

inline bool Subscription::hasMaxUnconfirmedMessages() const
{
    return d_maxUnconfirmedMessages.has_value();
}

inline bool Subscription::hasMaxUnconfirmedBytes() const
{
    return d_maxUnconfirmedBytes.has_value();
}

inline bool Subscription::hasConsumerPriority() const
{
    return d_consumerPriority.has_value();
}

// FREE FUNCTIONS

/// Apply the specified `hashAlgo` to the specified `id`.
template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const SubscriptionHandle& id)
{
    using bslh::hashAppend;  // for ADL
    hashAppend(hashAlgo, id.d_id);
}

inline bool operator<(const SubscriptionHandle& lhs,
                      const SubscriptionHandle& rhs)
{
    return lhs.d_id < rhs.d_id;
}

inline bool operator==(const SubscriptionHandle& lhs,
                       const SubscriptionHandle& rhs)
{
    return lhs.d_id == rhs.d_id;
}

inline bool operator!=(const SubscriptionHandle& lhs,
                       const SubscriptionHandle& rhs)
{
    return lhs.d_id != rhs.d_id;
}

inline bsl::ostream& operator<<(bsl::ostream&             stream,
                                const SubscriptionHandle& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bsl::ostream& operator<<(bsl::ostream& stream, const Subscription& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close package namespace

}  // close enterprise namespace

#endif
