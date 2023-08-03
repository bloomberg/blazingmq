// Copyright 2014-2023 Bloomberg Finance L.P.
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

// mqba_configprovider.cpp                                            -*-C++-*-
#include <mqba_configprovider.h>

#include <mqbscm_version.h>
// MQB
#include <mqbcfg_brokerconfig.h>
#include <mqbcfg_messages.h>
#include <mqbcmd_messages.h>

// BMQ
#include <bmqscm_versiontag.h>

// MWC
#include <mwcsys_time.h>
#include <mwcu_memoutstream.h>
#include <mwcu_printutil.h>
#include <mwcu_stringutil.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bdls_filesystemutil.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_cstdlib.h>
#include <bsl_fstream.h>
#include <bsl_iostream.h>
#include <bslma_allocator.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutexassert.h>
#include <bsls_annotation.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqba {

// --------------------
// class ConfigProvider
// --------------------

bool ConfigProvider::cacheLookup(mqbconfm::Response*      response,
                                 const bslstl::StringRef& key)
{
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(&d_mutex);  // mutex LOCKED

    CacheMap::const_iterator it = d_cache.find(key);

    if (it == d_cache.end()) {
        // Entry not found
        return false;  // RETURN
    }

    // Check expiration time
    if (it->second.d_expireTime < mwcsys::Time::nowMonotonicClock()) {
        // Value has expired, remove from the map
        d_cache.erase(it);
        return false;  // RETURN
    }

    // Cache entry is still 'alive'
    *response = it->second.d_data;  // Assign a copy of the object
    return true;
}

void ConfigProvider::onDomainConfigResponseCb(
    const mqbconfm::Response response,
    const GetDomainConfigCb& callback)
{
    if (response.isFailureValue()) {
        callback(response.failure().code(), response.failure().message());
        return;  // RETURN
    }

    BSLS_ASSERT_OPT(response.isDomainConfigValue());

    BALL_LOG_INFO << "Received domain config for domain '"
                  << response.domainConfig().domainName() << "': '"
                  << response.domainConfig().config() << "'";
    {
        bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // mutex LOCKED

        // Save to cache
        CacheEntry cacheEntry;
        cacheEntry.d_data = response;
        cacheEntry.d_expireTime =
            mwcsys::Time::nowMonotonicClock() +
            bsls::TimeInterval(
                mqbcfg::BrokerConfig::get().bmqconfConfig().cacheTTLSeconds());
        d_cache[response.domainConfig().domainName()] = cacheEntry;
    }

    // Call callback
    callback(0, response.domainConfig().config());
}

ConfigProvider::ConfigProvider(bslma::Allocator* allocator)
: d_mode(Mode::e_NORMAL)
, d_cache(allocator)
, d_allocator_p(allocator)
{
    // NOTHING
}

ConfigProvider::~ConfigProvider()
{
    stop();
}

int ConfigProvider::start(
    BSLS_ANNOTATION_UNUSED bsl::ostream& errorDescription)
{
    BALL_LOG_INFO << "Starting ConfigProvider";

    return 0;
}

void ConfigProvider::stop()
{
    // NOTHING
}

void ConfigProvider::getDomainConfig(const bslstl::StringRef& domainName,
                                     const GetDomainConfigCb& callback)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // mutex LOCKED

    // First, check in the cache
    mqbconfm::Response response;
    if (cacheLookup(&response, domainName) == true) {
        guard.release()->unlock();  // mutex UNLOCK
        BALL_LOG_INFO << "Config for domain '" << domainName << "' retrieved "
                      << "from cache";
        onDomainConfigResponseCb(response, callback);
        return;  // RETURN
    }

    // We don't have the config in the small cache ..
    int         rc = 0;
    bsl::string config;

    bsl::string filePath = mqbcfg::BrokerConfig::get().etcDir() + "/domains/" +
                           domainName + ".json";

    if (!bdls::FilesystemUtil::exists(filePath)) {
        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
        mwcu::MemOutStream                    os(&localAllocator);
        os << "Domain file '" << filePath << "' doesn't exist";
        config.assign(os.str().data(), os.str().length());
        rc = -1;
    }
    else {
        bsl::ifstream fileStream(filePath.c_str(), bsl::ios::in);
        if (!fileStream) {
            bdlma::LocalSequentialAllocator<1024> localAllocator(
                d_allocator_p);
            mwcu::MemOutStream os(&localAllocator);
            os << "Unable to open domain file '" << filePath << "'";
            config.assign(os.str().data(), os.str().length());
            rc = -2;
        }
        else {
            fileStream.seekg(0, bsl::ios::end);
            config.resize(fileStream.tellg());
            fileStream.seekg(0, bsl::ios::beg);
            fileStream.read(config.data(), config.size());
            fileStream.close();
        }
    }

    if (rc != 0) {
        response.makeFailure();
        response.failure().code()    = rc;
        response.failure().message() = config;
    }
    else {
        response.makeDomainConfig();
        response.domainConfig().config()     = config;
        response.domainConfig().domainName() = domainName;
    }
    guard.release()->unlock();  // unlock

    BALL_LOG_INFO << "Config for domain '" << domainName << "' retrieved "
                  << "from file '" << filePath << "'";

    onDomainConfigResponseCb(response, callback);
}

void ConfigProvider::clearCache(const bslstl::StringRef& domainName)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // mutex LOCKED

    if (domainName.length() == 0) {
        BALL_LOG_INFO << "Clearing up entire conf cache";
        d_cache.clear();
    }
    else {
        BALL_LOG_INFO << "Clearing up conf cache for '" << domainName << "'";
        d_cache.erase(domainName);
    }
}

int ConfigProvider::processCommand(
    const mqbcmd::ConfigProviderCommand& command,
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
