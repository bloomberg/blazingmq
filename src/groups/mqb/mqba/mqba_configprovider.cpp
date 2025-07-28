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

#include <bmqsys_time.h>
#include <bmqu_memoutstream.h>
#include <bmqu_printutil.h>
#include <bmqu_stringutil.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bdls_filesystemutil.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_cstdlib.h>
#include <bsl_fstream.h>
#include <bsl_iostream.h>
#include <bsla_annotations.h>
#include <bslma_allocator.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutexassert.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqba {

// --------------------
// class ConfigProvider
// --------------------

bool ConfigProvider::cacheLookup(bsl::string*             config,
                                 const bslstl::StringRef& key)
{
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(&d_mutex);  // mutex LOCKED

    CacheMap::const_iterator it = d_cache.find(key);

    if (it == d_cache.end()) {
        // Entry not found
        return false;  // RETURN
    }

    // Check expiration time
    if (it->second.d_expireTime < bmqsys::Time::nowMonotonicClock()) {
        // Value has expired, remove from the map
        d_cache.erase(it);
        return false;  // RETURN
    }

    // Cache entry is still 'alive'
    *config = it->second.d_data;  // Assign a copy of the object
    return true;
}

void ConfigProvider::cacheAdd(const bslstl::StringRef& key,
                              const bsl::string&       config)
{
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(&d_mutex);  // mutex LOCKED

    CacheEntry cacheEntry;
    cacheEntry.d_data = config;
    cacheEntry.d_expireTime =
        bmqsys::Time::nowMonotonicClock() +
        bsls::TimeInterval(
            mqbcfg::BrokerConfig::get().bmqconfConfig().cacheTTLSeconds());
    d_cache[key] = cacheEntry;
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

int ConfigProvider::start(BSLA_UNUSED bsl::ostream& errorDescription)
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
    enum {
        e_SUCCESS       = 0,
        e_FILENOTEXIST  = -1,
        e_FILENOTOPENED = -2,
    };

    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // LOCK

    // First, check in the cache
    bsl::string config;
    if (cacheLookup(&config, domainName) == true) {
        BALL_LOG_INFO << "Retrieved config for domain '" << domainName
                      << "' from cache: '" << config << "'";

        // Update the expiration time in the cache.
        cacheAdd(domainName, config);

        guard.release()->unlock();  // UNLOCK
        callback(e_SUCCESS, config);
        return;  // RETURN
    }

    // We don't have the config in the small cache ..

    bsl::string filePath = mqbcfg::BrokerConfig::get().etcDir() + "/domains/" +
                           domainName + ".json";

    if (!bdls::FilesystemUtil::exists(filePath)) {
        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
        bmqu::MemOutStream                    os(&localAllocator);
        os << "Domain file '" << filePath << "' doesn't exist";

        guard.release()->unlock();  // UNLOCK

        BALL_LOG_INFO << "Failed to retrieve config for domain '" << domainName
                      << "' from file '" << filePath << "': " << os.str();
        callback(e_FILENOTEXIST, os.str());
        return;  // RETURN
    }

    bsl::ifstream fileStream(filePath.c_str(), bsl::ios::in);
    if (!fileStream) {
        bdlma::LocalSequentialAllocator<1024> localAllocator(d_allocator_p);
        bmqu::MemOutStream                    os(&localAllocator);
        os << "Unable to open domain file '" << filePath << "'";

        guard.release()->unlock();  // UNLOCK

        BALL_LOG_INFO << "Failed to retrieve config for domain '" << domainName
                      << "' from file '" << filePath << "': " << os.str();
        callback(e_FILENOTOPENED, os.str());
        return;  // RETURN
    }

    fileStream.seekg(0, bsl::ios::end);
    config.resize(fileStream.tellg());
    fileStream.seekg(0, bsl::ios::beg);
    fileStream.read(config.data(), config.size());
    fileStream.close();

    BALL_LOG_INFO << "Retrieved config for domain '" << domainName
                  << "' from file '" << filePath << "': '" << config << "'";

    // Insert the newly-found configuration into the cache.
    cacheAdd(domainName, config);

    guard.release()->unlock();  // UNLOCK
    callback(e_SUCCESS, config);
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
                    bmqu::MemOutStream os;
                    os << "Domain '" << domainName << "' doesn't exist";
                    error->message() = os.str();
                    return -1;  // RETURN
                }
            }

            clearCache(domainName);
            return 0;  // RETURN
        }
    }
    bmqu::MemOutStream os;
    os << "Unknown command '" << command << "'";
    error->message() = os.str();
    return -1;
}

}  // close package namespace
}  // close enterprise namespace
