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

// mqbu_loadbalancer.h                                                -*-C++-*-
#ifndef INCLUDED_MQBU_LOADBALANCER
#define INCLUDED_MQBU_LOADBALANCER

//@PURPOSE: Provide a mechanism to load-balance objects across processors.
//
//@CLASSES:
//  mqbu::LoadBalancer: Load-balance objects across processors
//
//@DESCRIPTION: 'mqbu::LoadBalancer' provides a simple templated mechanism to
// load-balance objects (aka 'clients') of parameterized type across a set of
// processors by associating them with a processorId (an integer in the range
// '[0 .. processorsCount() - 1]'.
//
/// Thread Safety
///-------------
// The 'mqbu::LoadBalancer' object is fully thread-safe, meaning that two
// threads can safely call any methods on the *same* *instance* without
// external synchronization.
//
/// Usage Example
///-------------
// The following example illustrates how an 'mqbu::LoadBalancer' is typically
// used.
//
// First, we create a 'LoadBalancer' configured to load-balance objects of type
// 'MyClient' across 4 processors.
//..
//  mqbu::LoadBalancer<MyClient> loadBalancer(4, allocator);
//..
//
// Then we register a client to the loadBalancer, and retrieve it's associated
// processorId (this is usually done during the constructor of the client
// object).
//..
//  MyClient myClient;
//  int processorId = loadBalancer.getProcessorForClient(&myClient);
//..
//
// Finally, once the client object is no longer needed, we remove its
// association to the processor from the loadBalancer (this is usually done
// during the destructor of the client object).
//..
//  loadBalancer.removeClient(&myClient);
//..
//

// MQB

// BDE
#include <bsl_algorithm.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bslmt_mutexassert.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbu {

// ==================
// class LoadBalancer
// ==================

/// Load-balance objects of type `TYPE` across processors.
template <class TYPE>
class LoadBalancer {
  private:
    // PRIVATE TYPES

    /// ClientMap[client] = processorId
    typedef bsl::unordered_map<const TYPE*, int> ClientMap;

  private:
    // DATA
    bsl::vector<int> d_counters;
    // Client counters per processor

    ClientMap d_clients;
    // Map between the registered clients and the
    // corresponding processorId

    mutable bslmt::Mutex d_mutex;
    // Lock to protect this object in multi-threaded
    // environment

  private:
    // PRIVATE ACCESSORS

    /// Return the id of the processor that have the lowest number of
    /// clients associated with it.
    int findSmallestCounterLocked() const;

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator are not implemented.
    LoadBalancer(const LoadBalancer&);             // = delete
    LoadBalancer& operator=(const LoadBalancer&);  // = delete

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(LoadBalancer, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a `LoadBalancer` instance that will distribute clients across
    /// the specified `numProcessors` processors, using the specified
    /// `allocator` for memory allocation.  The behaviour is undefined
    /// unless `0 < numProcessors`.
    LoadBalancer(int numProcessors, bslma::Allocator* allocator);

    // MANIPULATORS

    /// Return the processorId associated to the specified `client`.  If
    /// `client` was not associated with any processor (i.e., this is the
    /// first call for this client), then the processor with the lowest
    /// number of clients gets assigned to `client` and its processorId is
    /// returned.
    int getProcessorForClient(const TYPE* client);

    /// Assign the specified `client` with the specified `processorId`.  The
    /// behaviour is undefined unless `0 <= processorId < processorsCount()`
    /// and `client` is currently not associated with any processor.  This
    /// method is useful if the association of a `processorId` for a given
    /// client must be preserved and restored.
    void setProcessorForClient(const TYPE* client, int processorId);

    /// Remove the association of the specified `client` with its processor.
    /// This method has no effect if `client` is not associated with any
    /// processor.
    void removeClient(const TYPE* client);

    // ACCESSORS

    /// Return the number of processors configured for this object.
    int processorsCount() const;

    /// Return the number of clients registered to this object.
    int clientsCount() const;

    /// Return the number of clients associated to the specified
    /// `processorId`.  The behavior is undefined unless '0 <= processorId <
    /// processorsCount()'.
    int clientsCountForProcessor(int processorId) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ------------------
// class LoadBalancer
// ------------------

template <class TYPE>
int LoadBalancer<TYPE>::findSmallestCounterLocked() const
{
    // PRECONDITIONS
    BSLMT_MUTEXASSERT_IS_LOCKED_SAFE(&d_mutex);  // d_mutex was LOCKED

    return bsl::distance(d_counters.begin(),
                         bsl::min_element(d_counters.begin(),
                                          d_counters.end()));
}

template <class TYPE>
LoadBalancer<TYPE>::LoadBalancer(int               numProcessors,
                                 bslma::Allocator* allocator)
: d_counters(numProcessors, 0, allocator)
, d_clients(allocator)
, d_mutex()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(0 < numProcessors);
}

template <class TYPE>
int LoadBalancer<TYPE>::getProcessorForClient(const TYPE* client)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCKED

    typename ClientMap::const_iterator it = d_clients.find(client);
    if (it != d_clients.end()) {
        // Client already has a processor associated with it
        return it->second;  // RETURN
    }

    // This is brand new client
    int processorId = findSmallestCounterLocked();

    // Add the client to the map and bump the counter for the selected
    // processor indicating that it now have one more client to serve
    d_counters[processorId] += 1;
    d_clients[client] = processorId;

    return processorId;
}

template <class TYPE>
void LoadBalancer<TYPE>::setProcessorForClient(const TYPE* client,
                                               int         processorId)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCKED

    // PRECONDITIONS
    BSLS_ASSERT_OPT(0 <= processorId && processorId < processorsCount());
    BSLS_ASSERT_SAFE(d_clients.find(client) == d_clients.end());

    // Add the client to the map and bump the counter for the selected
    // processor indicating that it now have one more client to serve
    d_counters[processorId] += 1;
    d_clients[client] = processorId;
}

template <class TYPE>
void LoadBalancer<TYPE>::removeClient(const TYPE* client)
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCKED

    typename ClientMap::const_iterator it = d_clients.find(client);
    if (it == d_clients.end()) {
        // Client was not found
        return;  // RETURN
    }

    // Remove the client from the map and decrement the counter for the
    // associated processor.
    d_counters[it->second] -= 1;
    d_clients.erase(it);
}

template <class TYPE>
int LoadBalancer<TYPE>::processorsCount() const
{
    return d_counters.size();
}

template <class TYPE>
int LoadBalancer<TYPE>::clientsCount() const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCKED
    return d_clients.size();
}

template <class TYPE>
int LoadBalancer<TYPE>::clientsCountForProcessor(int processorId) const
{
    bslmt::LockGuard<bslmt::Mutex> guard(&d_mutex);  // d_mutex LOCKED

    // PRECONDITIONS
    BSLS_ASSERT_SAFE(processorId >= 0 && processorId < processorsCount());

    return d_counters[processorId];
}

}  // close package namespace
}  // close enterprise namespace

#endif
