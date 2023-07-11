// Copyright 2017-2023 Bloomberg Finance L.P.
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

// mwcma_countingallocatorstore.h                                     -*-C++-*-
#ifndef INCLUDED_MWCMA_COUNTINGALLOCATORSTORE
#define INCLUDED_MWCMA_COUNTINGALLOCATORSTORE

//@PURPOSE: Provide a collection of 'mwcma::CountingAllocator' objects.
//
//@CLASSES:
//  mwcma::CountingAllocatorStore: a collection of 'mwcma::CountingAllocator'
//
//@SEE_ALSO: mwcma::CountingAllocator
//
//@DESCRIPTION: This component defines a mechanism,
// 'mwcma::CountingAllocatorStore' which can be used for creating and
// dispensing 'mwcma::CountingAllocator' objects sharing a common ancestor
// allocator.  Using 'mwcma::CountingAllocatorStore' objects consistently
// allows an application to determine at program start time whether to collect
// allocator statistics or not, and to incur no runtime overhead in this case.
//
// If the 'mwcma::CountingAllocatorStore' is created with a
// 'mwcma::CountingAllocator' (as determined by 'dynamic_cast') then a call to
// 'get' will return a 'mwcma::CountingAllocator' with the specified name,
// creating it if necessary and keeping it alive until the
// 'mwcma::CountingAllocatorStore' is cleared or destroyed.  If the allocator
// provided at construction is not a 'mwcma::CountingAllocator' then calls to
// 'get' will simply return this allocator.
//
/// Usage Example
///-------------
// To use counting allocators, the first thing to do is to install them in the
// application, typically inside the 'main' function.  The following lines of
// code will take care of it:
//..
//  mwcst::StatContext allocatorContext(mwcst::StatContextConfiguration("test")
//                                                     .defaultHistorySize(2));
//
//  mwcma::CountingAllocator topAllocator("Allocators", &allocatorContext);
//
//..
// This creates a stat context named 'test' and a counting allocator named
// 'Allocators' under it which gets allocation stats reported to it.
//
// Next, let's assume that the application uses a top-level class containing
// all static objects and defining the startup and shutdown sequences.  The
// typical name for this class is 'Application'.  Ours looks like the
// following:
//..
//  class Application {
//      // Main static object container for the program.
//
//  private:
//      // DATA
//      bslma::Allocator              *d_allocator_p;
//
//      mwcma::CountingAllocatorStore  d_allocators;
//                                          // Container for counting
//                                          // allocators created at the
//                                          // application's level.
//
//      mwcst::StatContext            *d_globalStatContext_p;
//                                          // Held, not owned.
//
//      ErrantSubsystem                d_errantSubsystem;
//                                          // Implements surprising behaviors
//                                          // in production.
//
//  public:
//      // CREATORS
//      Application(mwcst::StatContext *globalStatContext,
//                  bslma::Allocator   *allocator);
//
//      ~Application();
//
//      // MANIPULATORS
//      int start();
//          // Start the application, which will use memory.  Return 0 on
//          // success and non-zero otherwise.
//
//      void reportMemoryUsage(bsl::ostream& stream);
//          // Report the memory usage of the application to the specified
//          // 'stream'.
//  };
//..
// For the sake of simplicity, our application only has one static object, of
// type 'ErrantSubsystem'.  The application class also holds a pointer to the
// global stat context created by 'initGlobalAllocators'.  Finally, the
// application class uses a 'mwcma::CountingAllocatorStore' initialized with
// the top-level counting allocator.  This store allows us to create a new
// level of counting allocators.
//
// The constructor initializes the class as follow.  The 'ErrantSubsystem'
// object is passed an allocator created from the application's allocator
// store using method 'get'.  It is a good idea to provide a separate allocator
// to each subsystem using this method, in order to track memory usage for each
// of them.
//..
//  Application::Application(mwcst::StatContext *globalStatContext,
//                           bslma::Allocator   *allocator)
//  : d_allocator_p(allocator)
//  , d_allocators(d_allocator_p)
//  , d_globalStatContext_p(globalStatContext)
//  , d_errantSubsystem(d_allocators.get("ErrantSubsystem"))
//  {
//      // NOTHING
//  }
//..
// Let's assume that the 'ErrantSubsystem' itself uses several components,
// including an 'InverseProbabilityMatrix' for which we want to track memory
// usage.  'ErrantSubsystem' uses the same design as the application class: it
// has its own counting allocator store to create allocators for its own data
// members.  The container store of 'ErrantSubsystem' is created one level
// below the one of the application class.
//..
//  class ErrantSubsystem {
//  private:
//      // DATA
//      bslma::Allocator              *d_allocator_p;
//
//      mwcma::CountingAllocatorStore  d_allocators;
//                                          // Container for counting
//                                          // allocators created at the
//                                          // subsystem level.
//
//      InverseProbabilityMatrix       d_matrix;
//                                          // Subsystem
//
//  public:
//      // CREATORS
//      ErrantSubsystem(bslma::Allocator *allocator);
//
//      // MANIPULATOR
//      void misbehave();
//  };
//
//  ErrantSubsystem::ErrantSubsystem(bslma::Allocator *allocator)
//  : d_allocator_p(allocator)
//  , d_allocators(d_allocator_p)
//  , d_matrix(20, d_allocators.get("Matrix"))
//  {
//      // NOTHING
//  }
//..
// The 'InverseProbabilityMatrix' is a low-level component and does not have
// its own counting allocator store.  It just uses the allocator that it is
// supplied at construction.
//..
//  struct ExistentialBuffer {
//      int    d_meaninglessnessFactor;
//      double d_complacencyRate;
//      bool   d_nihilismFlag;
//  };
//
//  class InverseProbabilityMatrix {
//  private:
//      // DATA
//      bslma::Allocator              *d_allocator_p;
//
//      bsl::vector<ExistentialBuffer> d_buffers;
//
//  public:
//      // CREATORS
//      InverseProbabilityMatrix(int               numBuffers,
//                               bslma::Allocator *allocator);
//
//      void compute();
//
//      void perturbate();
//  };
//
//  InverseProbabilityMatrix::InverseProbabilityMatrix(
//                                        int               numBuffers,
//                                        bslma::Allocator *allocator)
//  : d_allocator_p(allocator)
//  , d_buffers(d_allocator_p)
//  {
//      d_buffers.resize(numBuffers);
//  }
//..
// The last thing we need to is to implement the application's
// 'reportMemoryUsage' method, as follow:
//..
//  void Application::reportMemoryUsage(bsl::ostream& stream)
//  {
//      // First, snapshot the global stat context.  It crashes if we forget to
//      // do it once before printing.
//      d_globalStatContext_p->snapshot();
//
//      // Second, print a pretty table.
//      stream << "\n==================== ALLOCATORS ======================\n";
//
//      const mwcst::StatContext *allocatorStatContext =
//              d_globalStatContext_p->getSubcontext("Allocators");
//
//      mwcst::StatContextTableInfoProvider tip;
//      mwcma::CountingAllocator::configureStatContextTableInfoProvider(&tip);
//      tip.setContext(allocatorStatContext);
//      tip.update();
//
//      mwcu::TableUtil::printTable(stream, tip);
//      stream << bsl::endl;
//  }
//..
// On first call. 'reportMemoryUsage' prints a table like the following:
//..
//  ====================== ALLOCATORS ========================
//
//                   | Bytes Allocated| -delta-| Max Bytes Allocated|
//                   Allocations| -delta-| Deallocations| -delta-
//  -----------------+----------------+--------+--------------------+------------+--------+--------------+--------
//  Allocators       |           1,200|   1,200|               1,200| 7| 7| 0|
//  0
//    *direct*       |             208|     208|                 208| 3| 3| 0|
//    0 ErrantSubsystem|             992|     992|                 992| 4| 4|
//    0|       0
//      *direct*     |             208|     208|                 208| 3| 3| 0|
//      0 Matrix       |             784|     784|                 784| 1| 1|
//      0|       0
//..
// As you can see the table shows the direct memory usage by the application
// class, then the direct memory usage for the 'ErrantSubsystem', and the
// memory usage of the matrix in hierarchical format.
//
/// Thread Safety
///-------------
// Thread safe.

// MWC

// BDE
#include <bsl_string.h>
#include <bsl_unordered_map.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_keyword.h>
#include <bsls_spinlock.h>

namespace BloombergLP {
namespace mwcma {

// FORWARD DECLARATIONS
class CountingAllocator;

// ============================
// class CountingAllocatorStore
// ============================

/// Factory and container for `mwcma::CountingAllocator` objects created
/// from a common allocator.
class CountingAllocatorStore BSLS_KEYWORD_FINAL {
  private:
    // PRIVATE TYPES

    /// name -> countingAllocator
    typedef bsl::unordered_map<bsl::string, mwcma::CountingAllocator*>
        AllocatorMap;

    // DATA
    AllocatorMap d_allocators;  // Map of counting allocators by name

    bsls::SpinLock d_spinLock;

    bslma::Allocator* d_allocator_p;  // Allocator to use

  private:
    // NOT IMPLEMENTED
    CountingAllocatorStore(const CountingAllocatorStore&) BSLS_KEYWORD_DELETED;
    CountingAllocatorStore&
    operator=(const CountingAllocatorStore&) BSLS_KEYWORD_DELETED;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(CountingAllocatorStore,
                                   bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a counting allocator store initialized with the specified
    /// `allocator`.  If `allocator` is a null pointer, use the default
    /// allocator.  If `allocator` is of type `mwcma::CountingAllocator`, a
    /// call to `get` will return a `mwcma::CountingAllocator`; otherwise
    /// `get` will simply return `allocator`.
    explicit CountingAllocatorStore(bslma::Allocator* allocator = 0);

    /// Destroy this object and all contained counting allocators.
    ~CountingAllocatorStore();

    // MANIPULATORS

    /// Return the base allocator used by this component, i.e., the one
    /// supplied at construction of this object (or the by then installed
    /// default allocator if none was provided).  If this allocator was an
    /// instance of `mwcma::CountingAllocator`, it then corresponds to the
    /// parent of all allocators created by a call to `get`.
    bslma::Allocator* baseAllocator();

    /// If the `allocator` provided at construction was not a
    /// `mwcma::CountingAllocator`, simply return that allocator.  Otherwise
    /// return a `mwcma::CountingAllocator` descended from that allocator,
    /// creating it if necessary' with the specified `name`.
    bslma::Allocator* get(const bsl::string& name);

    /// Delete all contained counting allocators.
    void clear();
};

}  // close package namespace
}  // close enterprise namespace

#endif
