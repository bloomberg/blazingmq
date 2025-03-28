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

// mqbplug_statconsumer.h                                             -*-C++-*-
#ifndef INCLUDED_MQBPLUG_STATCONSUMER
#define INCLUDED_MQBPLUG_STATCONSUMER

//@PURPOSE: Provide base classes for the 'StatConsumer' plugin.
//
//@CLASSES:
//  mqbplug::StatConsumer: interface for a plugin of type 'e_STATS_CONSUMER'.
//  mqbplug::StatConsumerPluginFactory: base class for the 'StatConsumer'
//                                      plugin factory.
//
//@DESCRIPTION: This component provide definitions for classes
// 'mqbplug::StatConsumer', 'mqbplug::StatConsumerPluginFactory' used as base
// classes for plugins that publish statistics and for their factories.

// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbplug_pluginfactory.h>

// BDE
#include <bdlbb_blob.h>
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_managedptr.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_timeinterval.h>
#include <bslstl_stringref.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqst {
class StatContext;
}

namespace mqbplug {

// ==================
// class StatConsumer
// ==================

/// Interface for a StatConsumer.
class StatConsumer {
  public:
    // TYPES
    typedef bsl::unordered_map<bsl::string, bmqst::StatContext*>
        StatContextsMap;
    // Map of StatContext names to StatContext

    typedef bsl::function<int(const bslstl::StringRef& source,
                              const bsl::string&       cmd,
                              bsl::ostream&            os)>
        CommandProcessorFn;

    // CREATORS

    /// Destroy this object.
    virtual ~StatConsumer();

    // ACESSORS

    /// Return the name of the plugin.
    virtual bslstl::StringRef name() const = 0;

    /// Return true if the stats reporting is enabled, false otherwise.
    virtual bool isEnabled() const = 0;

    /// Return current value of publish interval.
    virtual bsls::TimeInterval publishInterval() const = 0;

    // MANIPULATORS

    /// Start the StatConsumer and return 0 on success, or return a non-zero
    /// value and populate the specified `errorDescription` with the
    /// description of any failure encountered.
    virtual int start(bsl::ostream& errorDescription) = 0;

    /// Stop the StatConsumer.
    virtual void stop() = 0;

    /// Publish the stats if publishing at the intervals specified by the
    /// config.
    virtual void onSnapshot() = 0;

    /// Set the stats publish interval with the specified `interval`.
    /// Disable the stats publishing if `interval` is 0.  It is expected
    /// that specified `interval` is a multiple of the snapshot interval or
    /// 0.
    virtual void setPublishInterval(bsls::TimeInterval interval) = 0;
};

// ===============================
// class StatConsumerPluginFactory
// ===============================

/// This is the base class for the factory of plugins of type
/// `StatConsumer`. All it does is allows to instantiate a concrete object
/// of the `StatConsumer` interface, taking any required (plugin specific)
/// arguments..
class StatConsumerPluginFactory : public PluginFactory {
  public:
    // CREATORS
    StatConsumerPluginFactory();

    ~StatConsumerPluginFactory() BSLS_KEYWORD_OVERRIDE;

    // MANIPULATORS
    virtual bslma::ManagedPtr<StatConsumer>
    create(const StatConsumer::StatContextsMap&    statContextsMap,
           const StatConsumer::CommandProcessorFn& commandProcessor,
           bdlbb::BlobBufferFactory*               bufferFactory,
           bslma::Allocator*                       allocator) = 0;
};

// =======================
// struct StatConsumerUtil
// =======================

struct StatConsumerUtil {
    // STATIC CLASS METHODS

    /// Find consumer config with the specified `name`
    static const mqbcfg::StatPluginConfig*
    findConsumerConfig(bslstl::StringRef name);
};

}  // close package namespace
}  // close enterprise namespace

#endif
