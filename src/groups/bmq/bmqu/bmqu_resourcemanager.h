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

// bmqu_resourcemanager.h                                             -*-C++-*-
#ifndef INCLUDED_BMQU_RESOURCEMANAGER
#define INCLUDED_BMQU_RESOURCEMANAGER

//@PURPOSE: Provide a mechanism representing a boolean in ThreadLocalStorage.
//
//@CLASSES:
//  bmqu::TLSBool: mechanism representing a boolean value in ThreadLocalStorage
//
//@DESCRIPTION: 'bmqu::TLSBool' is a mechanism representing a boolean value,
// stored in ThreadLocalStorage (that is a bool having a value per thread),
// using the POSIX compliant pthread 'specifics' functionality.
//
/// Functionality
///-------------
// TBD: Comment about Default vs Initial value -- assert vs safe mode
//
/// Limitations
///-----------
// TBD: PTHREAD_KEYS_MAX typically 4096.
//
/// Usage
///-----
// This section illustrates the intended use of this component.
//
/// Example 1: TBD:
///- - - - - - - -
// TBD:

// BDE
#include <bslmt_threadutil.h>
#include <bslmt_mutex.h>
#include <bslmt_lockguard.h>
#include <bsl_vector.h>
#include <bsl_memory.h>
#include <bsl_functional.h>
#include <bslmt_once.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_types.h>
#include <bsls_unspecifiedbool.h>

#include <ball_log.h>

namespace BloombergLP {
namespace bmqu {

// =====================
// class ResourceManager
// =====================

/// Mechanism
class ResourceManager {
  private:
    BALL_LOG_SET_CLASS_CATEGORY("BMQU.RESOURCEMANAGER");

    // PRIVATE TYPES
    template<class TYPE>
    struct ResourceTraits {
        typedef TYPE value_type;
        typedef bsl::shared_ptr<TYPE> pointer_type;
        typedef bsl::function<pointer_type(bslma::Allocator*)> factory_type;
        typedef bsl::shared_ptr<factory_type> factory_pointer_type;

        static unsigned int typeId();
    };

    struct ThreadResources {
        
    };

    // CLASS DATA
    static ResourceManager *g_instance_p;

    // DATA
    bslma::Allocator* d_allocator_p;

    bslmt::Mutex d_mutex;

    bsl::vector<bsl::shared_ptr<void> > d_resourceCreators;
    bsl::vector<bsl::shared_ptr<void> > d_resources;

  private:
    // CREATORS
    explicit ResourceManager(bslma::Allocator* allocator = 0);
    ~ResourceManager();

    // PRIVATE CLASS METHODS

    static unsigned int nextTypeId();

  public:
    // TYPES

  public:
    // CREATORS

    // PUBLIC CLASS METHODS
    static void init(bslma::Allocator* allocator);
    static void deinit();

    // MANIPULATORS
    template<class TYPE>
    static void registerResourceFactory(bslmf::MovableRef<bsl::function<bsl::shared_ptr<TYPE>(bslma::Allocator*)>> creator) {
        BSLS_ASSERT_OPT(g_instance_p && "ResourceManager is not initialized");
        ResourceManager &manager = *g_instance_p;

        typedef ResourceTraits<TYPE> Traits;

        size_t typeId = Traits::typeId();

        BALL_LOG_ERROR << typeId;
        
        bslmt::LockGuard<bslmt::Mutex> guard(&manager.d_mutex);

        if (manager.d_resourceCreators.size() <= typeId) {
            manager.d_resourceCreators.resize(typeId + 1);
        }
        if (manager.d_resources.size() <= typeId) {
            manager.d_resources.resize(typeId + 1);
        }
        BSLS_ASSERT_OPT(!manager.d_resourceCreators.at(typeId));

        typename Traits::factory_pointer_type creator_sp;
        creator_sp.createInplace(manager.d_allocator_p);
        *creator_sp = creator;

        manager.d_resourceCreators.at(typeId) = bslmf::MovableRefUtil::move(creator_sp);
    }

    template<class TYPE>
    static bsl::shared_ptr<TYPE> getResource() {
        BSLS_ASSERT_OPT(g_instance_p && "ResourceManager is not initialized");
        ResourceManager &manager = *g_instance_p;

        typedef ResourceTraits<TYPE> Traits;

        size_t typeId = Traits::typeId();

        bslmt::LockGuard<bslmt::Mutex> guard(&manager.d_mutex);
        
        if (manager.d_resources.at(typeId)) {
            return bsl::reinterpret_pointer_cast<TYPE>(manager.d_resources.at(typeId));
        }

        BSLS_ASSERT_OPT(manager.d_resourceCreators.at(typeId) && "Resource factory for the resource is not registered");

        const typename Traits::factory_type &creator =
         *bsl::reinterpret_pointer_cast<typename Traits::factory_type >(manager.d_resourceCreators.at(typeId));

        typename Traits::pointer_type resource(creator(manager.d_allocator_p));
        manager.d_resources.at(typeId) = resource;
        return resource;
    }

};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

template <class TYPE>
unsigned int ResourceManager::ResourceTraits<TYPE>::typeId() {
    static unsigned int typeId;
    BSLMT_ONCE_DO {
        typeId = ResourceManager::nextTypeId();
    }
    return typeId;
}

}  // close package namespace
}  // close enterprise namespace

#endif
