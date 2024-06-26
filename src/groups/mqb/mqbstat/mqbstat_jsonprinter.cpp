// Copyright 2024 Bloomberg Finance L.P.
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

// mqbstat_jsonprinter.cpp                                            -*-C++-*-
#include <mqbstat_jsonprinter.h>

#include <mqbscm_version.h>

// MQB
#include <mqbstat_queuestats.h>

// MWC
#include <mwcu_memoutstream.h>

// BDE
#include <ball_log.h>
#include <bdljsn_json.h>
#include <bdljsn_jsonutil.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbstat {

namespace {

// ---------------------
// class JsonPrinterImpl
// ---------------------

/// The implementation class for JsonPrinter, containing all the cached options
/// for printing statistics as json.  This implementation exists and is hidden
/// from the package include for the following reasons:
/// - Don't want to expose `bdljsn` names and symbols to the outer scope.
/// - Member fields and functions defined for this implementation are used only
///   locally, so there is no reason to make it visible.
class JsonPrinterImpl {
  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("MQBSTAT.JSONPRINTERIMPL");

  private:
    // PRIVATE TYPES
    typedef bsl::unordered_map<bsl::string, mwcst::StatContext*>
        StatContextsMap;

    typedef mqbstat::QueueStatsDomain::Stat Stat;

  private:
    // DATA
    /// Config to use
    const mqbcfg::StatsConfig& d_config;

    /// Options for printing a compact json
    const bdljsn::WriteOptions d_opsCompact;

    /// Options for printing a pretty json
    const bdljsn::WriteOptions d_opsPretty;

    /// StatContext-s map
    const StatContextsMap d_contexts;

  private:
    // NOT IMPLEMENTED
    JsonPrinterImpl(const JsonPrinterImpl& other) BSLS_CPP11_DELETED;
    JsonPrinterImpl&
    operator=(const JsonPrinterImpl& other) BSLS_CPP11_DELETED;

    // ACCESSORS

    /// "domainQueues"
    /// Populate the specified `bdljsn::JsonObject*` with the values
    /// from the specified `ctx`.
    void populateQueueValues(bdljsn::JsonObject*       queueObject,
                             const mwcst::StatContext& ctx) const;
    void populateQueueStats(bdljsn::JsonObject*       domainObject,
                            const mwcst::StatContext& ctx) const;
    void populateDomainQueueStats(bdljsn::JsonObject*       parent,
                                  const mwcst::StatContext& ctx) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(JsonPrinterImpl, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new `JsonPrinterImpl` object, using the specified `config`,
    /// `statContextsMap` and the specified `allocator`.
    JsonPrinterImpl(const mqbcfg::StatsConfig& config,
                    const StatContextsMap&     statContextsMap,
                    bslma::Allocator*          allocator);

    // ACCESSORS

    /// Print the json-encoded stats to the specified `out`.
    /// If the specified `compact` flag is `true`, the json is printed in a
    /// compact form, otherwise the json is printed in a pretty form.
    /// Return `0` on success, and non-zero return code on failure.
    ///
    /// THREAD: This method is called in the *StatController scheduler* thread.
    int printStats(bsl::string* out, bool compact) const;
};

inline JsonPrinterImpl::JsonPrinterImpl(const mqbcfg::StatsConfig& config,
                                        const StatContextsMap& statContextsMap,
                                        bslma::Allocator*      allocator)
: d_config(config)
, d_opsCompact(bdljsn::WriteOptions().setSpacesPerLevel(0).setStyle(
      bdljsn::WriteStyle::e_COMPACT))
, d_opsPretty(bdljsn::WriteOptions().setSpacesPerLevel(4).setStyle(
      bdljsn::WriteStyle::e_PRETTY))
, d_contexts(statContextsMap, allocator)
{
}

inline void
JsonPrinterImpl::populateQueueValues(bdljsn::JsonObject*       queueObject,
                                     const mwcst::StatContext& ctx) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(queueObject);

    static const bsl::vector<Stat::Enum> defs = {Stat::e_NB_PRODUCER,
                                                 Stat::e_NB_CONSUMER,
                                                 Stat::e_PUT_MESSAGES_DELTA,
                                                 Stat::e_PUT_BYTES_DELTA,
                                                 Stat::e_PUSH_MESSAGES_DELTA,
                                                 Stat::e_PUSH_BYTES_DELTA,
                                                 Stat::e_ACK_DELTA,
                                                 Stat::e_ACK_TIME_AVG,
                                                 Stat::e_ACK_TIME_MAX,
                                                 Stat::e_NACK_DELTA,
                                                 Stat::e_CONFIRM_DELTA,
                                                 Stat::e_CONFIRM_TIME_AVG,
                                                 Stat::e_CONFIRM_TIME_MAX};

    if (ctx.numValues() > 0) {
        bdljsn::JsonObject& values = (*queueObject)["values"].makeObject();

        for (int i = 0; i < defs.size(); i++) {
            Stat::Enum d = defs[i];

            const bsls::Types::Int64 value =
                mqbstat::QueueStatsDomain::getValue(ctx, -1, d);

            values[Stat::toString(d)].makeNumber() = value;
        }
    }
}

inline void
JsonPrinterImpl::populateQueueStats(bdljsn::JsonObject*       domainObject,
                                    const mwcst::StatContext& ctx) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(domainObject);

    for (mwcst::StatContextIterator queueIt = ctx.subcontextIterator();
         queueIt;
         ++queueIt) {
        bdljsn::JsonObject& queueObj =
            (*domainObject)[queueIt->name()].makeObject();
        populateQueueValues(&queueObj, *queueIt);

        if (queueIt->numSubcontexts() > 0) {
            bdljsn::JsonObject& appIdsObject = queueObj["appIds"].makeObject();

            // Add metrics per appId, if any
            for (mwcst::StatContextIterator appIdIt =
                     queueIt->subcontextIterator();
                 appIdIt;
                 ++appIdIt) {
                // Do not expect another nested StatContext within appId
                BSLS_ASSERT_SAFE(0 == appIdIt->numSubcontexts());

                populateQueueValues(
                    &appIdsObject[appIdIt->name()].makeObject(),
                    *appIdIt);
            }
        }
    }
}

inline void
JsonPrinterImpl::populateDomainQueueStats(bdljsn::JsonObject*       parent,
                                          const mwcst::StatContext& ctx) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(parent);

    bdljsn::JsonObject& nodes = (*parent)["domains"].makeObject();
    for (mwcst::StatContextIterator domainIt = ctx.subcontextIterator();
         domainIt;
         ++domainIt) {
        populateQueueStats(&nodes[domainIt->name()].makeObject(), *domainIt);
    }
}

inline int JsonPrinterImpl::printStats(bsl::string* out, bool compact) const
{
    // executed by *StatController scheduler* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);

    bdljsn::Json        json;
    bdljsn::JsonObject& obj = json.makeObject();

    {
        const mwcst::StatContext& ctx =
            *d_contexts.find("domainQueues")->second;
        bdljsn::JsonObject& domainQueuesObj = obj["domainQueues"].makeObject();

        populateDomainQueueStats(&domainQueuesObj, ctx);
    }

    const bdljsn::WriteOptions& ops = compact ? d_opsCompact : d_opsPretty;

    mwcu::MemOutStream os;
    const int          rc = bdljsn::JsonUtil::write(os, json, ops);
    if (0 != rc) {
        BALL_LOG_ERROR << "failed to encode stats json, rc = " << rc;
        return rc;  // RETURN
    }
    (*out) = os.str();
    return 0;
}

}  // close unnamed namespace

// -----------------
// class JsonPrinter
// -----------------

JsonPrinter::JsonPrinter(const mqbcfg::StatsConfig& config,
                         const StatContextsMap&     statContextsMap,
                         bslma::Allocator*          allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_impl_mp(new (*d_allocator_p)
                JsonPrinterImpl(config, statContextsMap, d_allocator_p),
            d_allocator_p)
{
}

int JsonPrinter::printStats(bsl::string* out, bool compact) const
{
    // executed by *StatController scheduler* thread

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out);
    BSLS_ASSERT_SAFE(d_impl_mp);

    return d_impl_mp->printStats(out, compact);
}

}  // close package namespace
}  // close enterprise namespace
