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

// mqba_domainresolver.cpp                                            -*-C++-*-
#include <mqba_domainresolver.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbcmd_messages.h>

// MWC
#include <mwcsys_executil.h>
#include <mwcsys_time.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>
#include <mwcu_stringutil.h>

// BDE
#include <baljsn_decoder.h>
#include <baljsn_decoderoptions.h>
#include <ball_log.h>
#include <bdlma_localsequentialallocator.h>
#include <bdls_filesystemutil.h>
#include <bsl_cstddef.h>
#include <bsl_fstream.h>
#include <bsl_iostream.h>
#include <bslma_allocator.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutexassert.h>

namespace BloombergLP {
namespace mqba {

namespace {
BALL_LOG_SET_NAMESPACE_CATEGORY("MQBA.DOMAINRESOLVER");

const bsls::TimeInterval k_DIR_CHECK_TTL = bsls::TimeInterval(60.0);
// Minimum interval between checking the last
// modification time of the script or config
// directory.
}  // close unnamed namespace

// --------------------
// class DomainResolver
// --------------------

void DomainResolver::updateTimestamps()
{
    const bsls::TimeInterval now = mwcsys::Time::nowRealtimeClock();

    if (now <= d_timestampsValidUntil) {
        // We last checked recently, don't do anything
        return;  // RETURN
    }

    // Get the last config directory modification time
    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
    if (bdls::FilesystemUtil::getLastModificationTime(&d_lastCfgDirTimestamp,
                                                      brkrCfg.etcDir()) != 0) {
        BALL_LOG_ERROR << "#DOMAIN_RESOLVER_CFGDIR_TIMESTAMP "
                       << "Error getting last modification time of '"
                       << brkrCfg.etcDir() << "'";

        // Reset to default timestamp
        d_lastCfgDirTimestamp = bdlt::Datetime();
        return;  // RETURN
    }

    d_timestampsValidUntil = now + k_DIR_CHECK_TTL;
}

bool DomainResolver::cacheLookup(mqbconfm::DomainResolver* out,
                                 const bslstl::StringRef&  domainName)
{
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(&d_mutex);  // mutex LOCKED

    CacheMap::const_iterator it = d_cache.find(domainName);

    if (it == d_cache.end()) {
        // Entry not found
        return false;  // RETURN
    }

    // Check if entry is stale: the entry is stale if its 'd_cfgDirTimestamp'
    // member is different than the 'd_lastCfgDirTimestamp'.
    //
    // NOTE: the caller must call 'updateTimestamps()' to update the
    //       d_lastCfgDirTimestamp' prior to calling this method.
    if (it->second.d_cfgDirTimestamp != d_lastCfgDirTimestamp) {
        // Stale entry, clear from the map
        d_cache.erase(it);
        return false;  // RETURN
    }

    *out = it->second.d_data;  // Assign a copy of the object
    return true;
}

int DomainResolver::getOrRead(bsl::ostream&             errorDescription,
                              mqbconfm::DomainResolver* out,
                              const bslstl::StringRef&  domainName)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // mutex LOCKED

    // Make sure we have the latest script timestamp
    updateTimestamps();

    // First, check in the cache
    if (cacheLookup(out, domainName)) {
        BALL_LOG_INFO << "Domain '" << domainName << "' resolved from cache";
        return 0;  // RETURN
    }

    // We don't have the config in the cache, or the config was stale and got
    // erased.
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS           = 0,
        rc_READ_ERROR        = -1,
        rc_DECODING_ERROR    = -2,
        rc_REDIRECTION_ERROR = -3
    };

    int         rc = rc_SUCCESS;
    bsl::string content;
    bsl::string resolvedDomainName = domainName;
    int         redirection        = 0;

    for (; redirection < 2; ++redirection) {
        // 1. read config
        bsl::string filePath = mqbcfg::BrokerConfig::get().etcDir() +
                               "/domains/" + resolvedDomainName + ".json";

        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);

        // This is copy-pasted from mqba_configprovider.cpp. Maybe we are
        // going to merge the two? If not, consider factoring this bit.
        if (!bdls::FilesystemUtil::exists(filePath)) {
            bdlma::LocalSequentialAllocator<1024> localAllocator(
                d_allocator_p);
            mwcu::MemOutStream os(&localAllocator);
            os << "Domain file '" << filePath << "' doesn't exist";
            content.assign(os.str().data(), os.str().length());
            rc = rc_READ_ERROR;
        }
        else {
            bsl::ifstream fileStream(filePath.c_str(), bsl::ios::in);
            if (!fileStream) {
                bdlma::LocalSequentialAllocator<1024> localAllocator(
                    d_allocator_p);
                mwcu::MemOutStream os(&localAllocator);
                os << "Unable to open domain file '" << filePath << "'";
                content.assign(os.str().data(), os.str().length());
                rc = rc_READ_ERROR;
            }
            else {
                fileStream.seekg(0, bsl::ios::end);
                content.resize(fileStream.tellg());
                fileStream.seekg(0, bsl::ios::beg);
                fileStream.read(content.data(), content.size());
                fileStream.close();
            }
        }

        if (rc != 0) {
            // In case of error, per contract the first line of stdout
            // contains the short summary description of the error - which is
            // what we will return to the caller.  We still print the full
            // output for debugging assistance, but at info level only because
            // the caller will be printing an error.

            // Remove the trailing '\n' (if any) for cleaner log printing
            // (avoid extra blank lines added by BALL_LOG).
            mwcu::StringUtil::rtrim(&content);

            BALL_LOG_INFO << "Error reading the domain config file "
                          << "[domain: '" << domainName << "'"
                          << ", rc: " << rc << ", output:\n"
                          << content;

            size_t eol = content.find("\n");
            if (eol != bsl::string::npos) {
                errorDescription << bslstl::StringRef(content.c_str(), eol);
            }
            else {
                errorDescription << content;
            }

            return rc;  // RETURN
        }

        // 2. decode JSON into object

        // We read a Domain but here we just need the DomainResolver, which is
        // a subset of the former. Just decode the whole thing, skipping the
        // other fields. We should consider merging the two caches. Not doing
        // it now because they have different caching strategies. TODO: discuss
        mqbconfm::DomainVariant domainVariant;
        baljsn::Decoder         decoder;
        baljsn::DecoderOptions  options;
        bsl::istringstream      jsonStream(content);

        options.setSkipUnknownElements(true);

        rc = decoder.decode(jsonStream, &domainVariant, options);
        if (rc != 0) {
            errorDescription << "Error while decoding domain configuration "
                             << "[domain: '" << domainName << "'"
                             << ", rc: " << rc << ", error: '"
                             << decoder.loggedMessages() << "'"
                             << ", from content: '" << jsonStream.str()
                             << "']";
            return rc_DECODING_ERROR;  // RETURN
        }

        if (domainVariant.isRedirectValue()) {
            BALL_LOG_INFO << "Redirecting " << resolvedDomainName << " to "
                          << domainVariant.redirect();
            resolvedDomainName = domainVariant.redirect();
            continue;
        }

        // Add to cache
        CacheEntry cacheEntry;
        cacheEntry.d_data.name()    = resolvedDomainName;
        cacheEntry.d_data.cluster() = domainVariant.definition().location();
        *out                        = cacheEntry.d_data;
        // REVIEW: suggestion: s/location/cluster/
        cacheEntry.d_cfgDirTimestamp = d_lastCfgDirTimestamp;
        // This is fine, because we updated
        // them by calling
        // 'updateTimestamps()'.

        d_cache[domainName] = cacheEntry;

        return rc_SUCCESS;  // RETURN
    }

    errorDescription << "Too many redirections [domain: '" << domainName
                     << ", redirections: " << redirection << "']";
    return rc_REDIRECTION_ERROR;
}

DomainResolver::DomainResolver(bslma::Allocator* allocator)
: d_cache(allocator)
, d_lastCfgDirTimestamp()
, d_timestampsValidUntil()
, d_allocator_p(allocator)
{
}

DomainResolver::~DomainResolver()
{
    stop();
}

int DomainResolver::start(bsl::ostream& errorDescription)
{
    // Verify that the config directory exists
    const mqbcfg::AppConfig& brkrCfg = mqbcfg::BrokerConfig::get();
    if (!bdls::FilesystemUtil::exists(brkrCfg.etcDir())) {
        errorDescription << "Domain configuration directory '"
                         << brkrCfg.etcDir() << "' doesn't exist";
        return -1;  // RETURN
    }

    return 0;
}

void DomainResolver::stop()
{
    // NOTHING
}

bmqp_ctrlmsg::Status
DomainResolver::getOrReadDomain(mqbconfm::DomainResolver* out,
                                const bslstl::StringRef&  domainName)
{
    mwcu::MemOutStream errorDescription;

    int rc = getOrRead(errorDescription, out, domainName);

    bmqp_ctrlmsg::Status status;
    status.category() = (rc == 0 ? bmqp_ctrlmsg::StatusCategory::E_SUCCESS
                                 : bmqp_ctrlmsg::StatusCategory::E_UNKNOWN);
    status.code()     = rc;
    status.message().assign(errorDescription.str().data(),
                            errorDescription.str().length());

    return status;
}

void DomainResolver::qualifyDomain(
    const bslstl::StringRef&                      domainName,
    const mqbi::DomainFactory::QualifiedDomainCb& callback)
{
    mqbconfm::DomainResolver response;
    bmqp_ctrlmsg::Status     status = getOrReadDomain(&response, domainName);

    callback(status, response.name());
}

void DomainResolver::locateDomain(const bslstl::StringRef& domainName,
                                  const LocateDomainCb&    callback)
{
    mqbconfm::DomainResolver response;
    bmqp_ctrlmsg::Status     status = getOrReadDomain(&response, domainName);

    callback(status, response.cluster());
}

void DomainResolver::clearCache(const bslstl::StringRef& domainName)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // mutex LOCKED

    if (domainName.length() == 0) {
        BALL_LOG_INFO << "Clearing up entire domain resolver cache ("
                      << d_cache.size() << " entries).";
        d_cache.clear();
    }
    else {
        BALL_LOG_INFO << "Clearing up domain resolver cache for '"
                      << domainName << "'";

        d_cache.erase(domainName);
    }
}

int DomainResolver::processCommand(
    const mqbcmd::DomainResolverCommand& command,
    mqbcmd::Error*                       error)
{
    if (command.isClearCacheValue()) {
        if (command.clearCache().isAllValue()) {
            clearCache();
            return 0;  // RETURN
        }
        else if (command.clearCache().isDomainValue()) {
            const bsl::string& domainName = command.clearCache().domain();
            {
                bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK
                if (d_cache.find(domainName) == d_cache.end()) {
                    mwcu::MemOutStream os;
                    os << "Domain '" << domainName << "' doesn't exist";
                    error->message() = os.str();
                    return -1;  // RETURN
                }
            }

            clearCache(domainName);
            return 0;  // RETURN
        }
    }

    mwcu::MemOutStream os;
    os << "Unknown command '" << command << "'";
    error->message() = os.str();
    return -1;
}

}  // close package namespace
}  // close enterprise namespace
