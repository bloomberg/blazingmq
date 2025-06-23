// Copyright 2014-2025 Bloomberg Finance L.P.
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

// bmqc_batchedpool.h                                                 -*-C++-*-
#ifndef INCLUDED_BMQC_BATCHEDPOOL
#define INCLUDED_BMQC_BATCHEDPOOL

//@PURPOSE:

// BDE
#include <bdlcc_objectpool.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bdlf_bind.h>

namespace BloombergLP {
namespace bmqc {

// FORWARD DECLARATIONS
template <class TYPE>
struct PoolBatch;

template <class TYPE>
class BatchedObjectPool;


template <class TYPE>
struct BatchNode {
  public:
    PoolBatch<TYPE>* d_batch_p;

    /// The user defined object owned by this node
    bsls::ObjectBuffer<TYPE> d_object;

    void release() {
        d_batch_p->onNodeRelease();
    }

    void reset() {
        d_object.object().reset();
    }

    explicit BatchNode(PoolBatch<TYPE>*  batch_p,
                       bslma::Allocator* allocator = 0)
    : d_batch_p(batch_p)
    , d_object()
    {
        bslalg::ScalarPrimitives::defaultConstruct(
            reinterpret_cast<TYPE*>(d_object.buffer()),
            allocator);
    }

    ~BatchNode() {
        d_object.object().~TYPE();
    }

    // NOT IMPLEMENTED
    /// Copy constructor and assignment operator are not implemented.
    BatchNode(const BatchNode&) BSLS_KEYWORD_DELETED;
    BatchNode& operator=(const BatchNode&) BSLS_KEYWORD_DELETED;    
};

template <class TYPE>
struct PoolBatch {
  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(PoolBatch,
                                   bslma::UsesBslmaAllocator)

    bsls::AtomicInt            d_refCount;

    BatchedObjectPool<TYPE>*   d_owner_p;

    bsl::vector<bsl::shared_ptr<BatchNode<TYPE> > > d_nodes;

    void onNodeRelease() {
        const int refCount = d_refCount.subtractRelaxed(1);
        if (0 == refCount) {
            d_owner_p->d_pool.releaseObject(this);
        }
    }

    explicit PoolBatch(size_t batchSize,
                       BatchedObjectPool<TYPE>* owner_p,
                       bslma::Allocator *allocator = 0)
    : d_refCount(batchSize)
    , d_owner_p(owner_p)
    , d_nodes(allocator)
    {
        d_nodes.resize(batchSize);
        for (size_t i = 0; i < batchSize; i++) {
            d_nodes[i] = bsl::allocate_shared<BatchNode<TYPE> >(allocator, this);
        }
    }

    void reset() {
        for (size_t i = 0; i < d_nodes.size(); i++) {
            d_nodes[i]->reset();
        }
        d_refCount = d_nodes.size();
    }
};

template <class TYPE>
// template collection for release/get/size etc
class LocalBatcher {
  private:
    BatchedObjectPool<TYPE> *d_owner_p;

    PoolBatch<TYPE>* d_batch_p;

    size_t     d_batchPos;
    
  public:
    explicit LocalBatcher(BatchedObjectPool<TYPE> *owner_p)
    : d_owner_p(owner_p)
    , d_batch_p(0)
    , d_batchPos(0)
    {
        BSLS_ASSERT_SAFE(d_owner_p);
        
        d_batch_p = d_owner_p->d_pool.getObject();
        d_batchPos = 0;
    }

    ~LocalBatcher() {
        while (d_batchPos < d_batch_p->d_nodes.size()) {
            d_batch_p->d_nodes[d_batchPos++]->release();
        }
    }

    BatchNode<TYPE>* getObject()
    {
        if (d_batch_p->d_nodes.size() <= d_batchPos) {
            d_batch_p = d_owner_p->d_pool.getObject();
            d_batchPos = 0;
        }
        return d_batch_p->d_nodes[d_batchPos++].get();
    }
};

template <class TYPE>
class BatchedObjectPool {
  private:
  public: //rm
    // PRIVATE TYPES
    /// `CreatorFn` is an alias for a functor creating an object of `TYPE`
    /// in the specified `arena` using the specified `allocator`.
    typedef bsl::function<void(void* arena, bslma::Allocator* allocator)>
        CreatorFn;

    typedef PoolBatch<TYPE> BatchedPoolObject;

    typedef bdlcc::
        ObjectPool<BatchedPoolObject, CreatorFn, bdlcc::ObjectPoolFunctors::Reset<BatchedPoolObject> >
            BasePool;

    // PRIVATE DATA
    bslma::Allocator *d_allocator_p;

    BasePool d_pool;

    // PRIVATE CLASS METHODS
    static void batchCreatorFn(size_t batchSize,
                          BatchedObjectPool<TYPE> *owner_p,
                          void* arena,
                          bslma::Allocator* allocator) {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(owner_p);
        BSLS_ASSERT_SAFE(arena);
        BSLS_ASSERT_SAFE(allocator);

        bslalg::ScalarPrimitives::construct(reinterpret_cast<PoolBatch<TYPE>*>(arena),
                                            batchSize,
                                            owner_p,
                                            allocator);
    }

  public:

    explicit BatchedObjectPool(size_t batchSize,
                               bslma::Allocator* allocator = 0)
    : d_allocator_p(bslma::Default::allocator(allocator))
    , d_pool(bdlf::BindUtil::bindS(d_allocator_p,
                              &BatchedObjectPool<TYPE>::batchCreatorFn,
                              batchSize,
                              this,
                              bdlf::PlaceHolders::_1,   // arena
                              bdlf::PlaceHolders::_2),  // allocator
             -1,
             d_allocator_p) {

    }

    bsl::shared_ptr<LocalBatcher<TYPE> > getBatcher() {
        return bsl::allocate_shared<LocalBatcher<TYPE> >(d_allocator_p, this);
    }

};

}  // close package namespace
}  // close enterprise namespace

#endif
