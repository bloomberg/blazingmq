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

// bmqt_queueoptions.cpp                                              -*-C++-*-
#include <bmqt_queueoptions.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqeval_simpleevaluator.h>

// BDE
#include <bsl_limits.h>
#include <bslim_printer.h>
#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bsls_annotation.h>

namespace BloombergLP {
namespace {

static void mergeSubscription(bmqt::Subscription*       to,
                              const bmqt::Subscription& from)
{
    if (from.hasMaxUnconfirmedMessages()) {
        to->setMaxUnconfirmedMessages(from.maxUnconfirmedMessages());
    }
    if (from.hasMaxUnconfirmedBytes()) {
        to->setMaxUnconfirmedBytes(from.maxUnconfirmedBytes());
    }
    if (from.hasConsumerPriority()) {
        to->setConsumerPriority(from.consumerPriority());
    }
    to->setExpression(from.expression());
}

}

namespace bmqt {

// ------------------
// class QueueOptions
// ------------------

const bool QueueOptions::k_DEFAULT_SUSPENDS_ON_BAD_HOST_HEALTH = false;

const int QueueOptions::k_CONSUMER_PRIORITY_MIN =
    bsl::numeric_limits<int>::min() / 2;
const int QueueOptions::k_CONSUMER_PRIORITY_MAX =
    bsl::numeric_limits<int>::max() / 2;
const int QueueOptions::k_DEFAULT_MAX_UNCONFIRMED_MESSAGES = 1000;
const int QueueOptions::k_DEFAULT_MAX_UNCONFIRMED_BYTES    = 33554432;

/// Solaris has a problem with defining constants as:
/// 'const int QueueOptions::k_CONSUMER_PRIORITY_MIN =
/// Subscription::k_CONSUMER_PRIORITY_MIN;'
const int QueueOptions::k_DEFAULT_CONSUMER_PRIORITY = 0;

QueueOptions::QueueOptions(bslma::Allocator* allocator)
: d_info()
, d_suspendsOnBadHostHealth()
, d_subscriptions(allocator)
, d_hadSubscriptions(false)
, d_allocator_p(allocator)
{
    // NOTHING
}

QueueOptions::QueueOptions(const QueueOptions& other,
                           bslma::Allocator*   allocator)
: d_info(other.d_info)
, d_suspendsOnBadHostHealth(other.d_suspendsOnBadHostHealth)
, d_subscriptions(other.d_subscriptions, allocator)
, d_hadSubscriptions(other.d_hadSubscriptions)
, d_allocator_p(allocator)
{
    // NOTHING
}

bsl::ostream&
QueueOptions::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("maxUnconfirmedMessages", maxUnconfirmedMessages());
    printer.printAttribute("maxUnconfirmedBytes", maxUnconfirmedBytes());
    printer.printAttribute("consumerPriority", consumerPriority());
    printer.printAttribute("suspendsOnBadHostHealth",
                           suspendsOnBadHostHealth());

    if (!d_subscriptions.empty()) {
        printer.printIndentation();

        stream << "Subscriptions:";

        for (Subscriptions::const_iterator cit = d_subscriptions.begin();
             cit != d_subscriptions.end();
             ++cit) {
            stream << "\n";

            cit->first.print(stream, level + 1, spacesPerLevel);
            cit->second.print(stream, level + 2, spacesPerLevel);
        }
    }

    printer.end();

    return stream;
}

QueueOptions& QueueOptions::merge(const QueueOptions& other)
{
    mergeSubscription(&d_info, other.d_info);

    if (other.d_hadSubscriptions) {
        d_subscriptions = other.d_subscriptions;
    }
    // else consider 'other.d_subscriptions' optional (like other data members)
    // and keep existing 'd_subscriptions'.

    if (other.hasMaxUnconfirmedMessages()) {
        setMaxUnconfirmedMessages(other.maxUnconfirmedMessages());
    }
    if (other.hasMaxUnconfirmedBytes()) {
        setMaxUnconfirmedBytes(other.maxUnconfirmedBytes());
    }
    if (other.hasConsumerPriority()) {
        setConsumerPriority(other.consumerPriority());
    }
    if (other.hasSuspendsOnBadHostHealth()) {
        setSuspendsOnBadHostHealth(other.suspendsOnBadHostHealth());
    }

    return *this;
}

bool QueueOptions::addOrUpdateSubscription(bsl::string* errorDescription,
                                           const SubscriptionHandle& handle,
                                           const Subscription& subscription)
{
    if (!subscription.expression().isValid()) {
        return false;  // RETURN
    }

    if (bmqt::SubscriptionExpression::e_VERSION_1 ==
        subscription.expression().version()) {
        bmqeval::CompilationContext context(d_allocator_p);

        if (!bmqeval::SimpleEvaluator::validate(
                subscription.expression().text(),
                context)) {
            if (errorDescription) {
                bmqu::MemOutStream os(d_allocator_p);
                os << "Expression validation failed: [ expression: \""
                   << subscription.expression().text()
                   << "\", rc: " << context.lastError() << ", reason: \""
                   << context.lastErrorMessage() << "\" ]";
                errorDescription->assign(os.str().data(), os.str().length());
            }
            return false;  // RETURN
        }
    }

    bsl::pair<Subscriptions::iterator, bool> result =
        d_subscriptions.emplace(handle, subscription);

    d_hadSubscriptions = true;

    if (!result.second) {
        result.first->second = subscription;
    }

    return true;
}

bool QueueOptions::removeSubscription(const SubscriptionHandle& handle)
{
    return d_subscriptions.erase(handle) > 0;
}

void QueueOptions::removeAllSubscriptions()
{
    d_hadSubscriptions = true;

    return d_subscriptions.clear();
}

bool QueueOptions::loadSubscription(Subscription*             subscription,
                                    const SubscriptionHandle& handle) const
{
    BSLS_ASSERT_SAFE(subscription);

    Subscriptions::const_iterator cit = d_subscriptions.find(handle);

    if (cit == d_subscriptions.end()) {
        return false;  // RETURN
    }

    *subscription = cit->second;

    return true;
}

void QueueOptions::loadSubscriptions(SubscriptionsSnapshot* snapshot) const
{
    BSLS_ASSERT_SAFE(snapshot);
    snapshot->clear();

    for (Subscriptions::const_iterator cit = d_subscriptions.begin();
         cit != d_subscriptions.end();
         ++cit) {
        snapshot->emplace_back(cit->first, cit->second);
    }
    // If 'd_hadSubscriptions == false', add default subscription.
    if (snapshot->empty() && !d_hadSubscriptions) {
        bmqt::CorrelationId correlationId;

        snapshot->emplace_back(bmqt::SubscriptionHandle(correlationId),
                               bmqt::Subscription());
    }
}

}  // close package namespace
}  // close enterprise namespace
