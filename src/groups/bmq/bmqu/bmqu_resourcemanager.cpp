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

// bmqu_resourcemanager.cpp                                           -*-C++-*-
#include <bmqu_resourcemanager.h>

#include <bmqscm_version.h>

namespace BloombergLP {
namespace bmqu {

ResourceManager *ResourceManager::g_instance_p = 0;


ResourceManager::ResourceManager(bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_mutex()
, d_resourceCreators(allocator)
, d_resources(allocator)
{
    // NOTHING
}

ResourceManager::~ResourceManager() {
    for (size_t i = 0; i < d_resourceCreators.size(); i++) {
        if (d_resourceCreators.at(i).numReferences() > 1) {
            BALL_LOG_ERROR << "Resource factory with " << d_resourceCreators.at(i).numReferences() << " references "
                           << "will not be freed";
        }
    }
    for (size_t i = 0; i < d_resources.size(); i++) {
        if (d_resources.at(i).numReferences() > 1) {
            BALL_LOG_ERROR << "Resource " << d_resourceCreators.at(i).numReferences() << " references "
                           << "will not be freed, something holds onto this resource";
        }
    }
}

void ResourceManager::init(bslma::Allocator* allocator) {
    BSLS_ASSERT_OPT(!g_instance_p && "ResourceManager already initialized");

    bslma::Allocator *alloc = bslma::Default::allocator(allocator);
    g_instance_p = new (*alloc) ResourceManager(alloc);
}

void ResourceManager::deinit() {
    BSLS_ASSERT_OPT(g_instance_p && "No active ResourceManager to deinitialize");
    g_instance_p->~ResourceManager();
    g_instance_p->d_allocator_p->deallocate(g_instance_p);
    g_instance_p = 0;
}

unsigned int ResourceManager::nextTypeId()
{
    static bsls::AtomicUint* s_id = 0;

    BSLMT_ONCE_DO
    {
        s_id = new bsls::AtomicUint(0);
        // Heap allocate it to prevent 'exit-time-destructor needed' compiler
        // warning.  Causes valgrind-reported memory leak.
    }

    return (*s_id)++;
}

}  // close package namespace
}  // close enterprise namespace
