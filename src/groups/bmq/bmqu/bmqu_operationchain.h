// Copyright 2021-2023 Bloomberg Finance L.P.
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

// bmqu_operationchain.h                                              -*-C++-*-
#ifndef INCLUDED_BMQU_OPERATIONCHAIN
#define INCLUDED_BMQU_OPERATIONCHAIN

//@PURPOSE: Provide a mechanism to serialize execution of async operations.
//
//@CLASSES:
//  OperationChain:     operation chain
//  OperationChainLink: a link in the chain
//
//@DESCRIPTION:
// This component provides a mechanism, 'bmqu::OperationChain', to serialize
// execution of groups of asynchronous operations in a way that each next group
// starts executing after the completion of all operations in the previous
// group. Inline with this component's terminology, a group of operations is
// called a "link". A link can contain one or more operations executing in
// parallel. Links are indexed and ordered, such that the first link has index
// 0, operations in the link 'N' starts executing after the completion of all
// operations in the link 'N - 1', and operations in link 0 starts
// executing as soon as the chain is started. After all operations in link 0
// are executed, the link is removed from the chain and the next link in line
// becomes link 0.
//..
//          ---------------------
//  LINK 0  |         A         |
//          ---------------------
//                    |
//                    V
//          ---------------------
//  LINK 1  |       B, C        |
//          ---------------------
//                    |
//                    V
//          ---------------------
//  LINK 2  |         D         |
//          ---------------------
//..
// In the illustration above operation 'A' is executed first, after its
// completion 'B' and 'C' are executed in parallel, and after the completion of
// those 'D' is executed.
//
/// Operations
///----------
// An asynchronous operation submitted to 'bmqu::OperationChain' is represented
// by two functors, the "operation callback" and the "completion callback". An
// operation callback initiates execution of the async operation, and should
// accept exactly one argument - the completion callback, which is passed to
// the operation callback as a parameter. The signature of the completion
// callback is operation-specific and is irrelevant to this component. It is
// expected that the completion callback will be invoked on completion of the
// async operation. Note that the completion callback can be invoked in-place,
// i.e. during the call to the operation callback.
//
/// Links
///-----
// A link in the operation chain is represented by 'bmqu::OperationChainLink',
// which is a move-only container for operations. A link can be atomically
// appended to or removed from the back of the chain.
//
/// Callbacks lifetime
///------------------
// The lifetime of both, the operation callback and the completion callback, is
// extended until the completion of the async operation, meaning that both
// functors are destroyed at the same time, immediately after the invocation of
// the completion callback. Manually removing a link from the chain, or
// destroying a non-empty chain will also destroy any associated callback.
//
/// Exception handling
///------------------
// Exception thrown by the completion callback are expected to be handled by
// the caller. Any exception thrown by the operation callback results in a call
// to 'bsl::terminate()'.
//
/// Thread safety
///-------------
//: o 'bmqu::OperationChain' is fully thread-safe.
//: o 'bmqu::OperationChainLink' is not thread-safe.
//
/// Usage
///-----
// Given two async operations, 'send' and 'receive'.
//..
//  struct UsageExample {
//      // A namespace for functions from the usage example.
//
//      // TYPES
//      typedef bsl::function<void()>
//              SendCallback;
//
//      typedef bsl::function<void(int clientId, int payload)>
//              ReceiveCallback;
//
//      // CLASS FUNCTIONS
//      static void onDataSent();
//          // Callback invoked on completion of a 'send' operation.
//
//      static void onDataReceived(int clientId, int payload);
//          // Callback invoked on completion of a 'receive' operation.
//
//      static void send(int                 clientId,
//                       int                 payload,
//                       const SendCallback& completionCallback);
//          // Send the specified 'payload' to the client identified by the
//          // specified 'clientId' and invoke the specified
//          // 'completionCallback' when the payload is sent.
//
//      static void receive(const ReceiveCallback& completionCallback);
//          // Receive a payload send to us by another client and invoke the
//          // specified 'completionCallback' with the payload and the sender
//          // client ID.
//  };
//..
// Lets say we want to "receive" data from 10 clients, and then "send" data
// to some other client. The code below demonstrates the way to do that using
// 'bmqu::OperationChain'.
//..
//  // create a chain
//  bmqu::OperationChain chain(bmqtst::TestHelperUtil::allocator());
//
//  static const int k_MY_PAYLOAD   = 42;
//  static const int k_MY_CLIENT_ID = 42;
//
//  // receive data from 10 clients
//  bmqu::OperationChainLink link(chain.allocator());
//  for (int i = 0; i < 10; ++i) {
//      link.insert(&createNewLink,
//                  bdlf::BindUtil::bind(&UsageExample::receive,
//                                       bdlf::PlaceHolders::_1),
//                  &UsageExample::onDataReceived);
//  }
//
//  chain.append(&link);
//
//  // send data to another client
//  chain.appendInplace(bdlf::BindUtil::bind(&UsageExample::send,
//                                           k_MY_CLIENT_ID,
//                                           k_MY_PAYLOAD,
//                                           bdlf::PlaceHolders::_1),
//                      &UsageExample::onDataSent);
//
//  // execute operations in the chain
//  chain.start();
//
//  // for for completion of all executing operations
//  chain.join();
//..

#include <bmqu_objectplaceholder.h>

// BDE
#include <bdlf_noop.h>
#include <bsl_list.h>
#include <bslalg_constructorproxy.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_assert.h>
#include <bslmf_decay.h>
#include <bslmf_integralconstant.h>
#include <bslmf_isnothrowmoveconstructible.h>
#include <bslmf_movableref.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bslmf_util.h>
#include <bslmt_condition.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bsls_assert.h>
#include <bsls_compilerfeatures.h>
#include <bsls_exceptionutil.h>  // BSLS_NOTHROW_SPEC
#include <bsls_keyword.h>

#ifdef BSLS_COMPILERFEATURES_SUPPORT_GENERALIZED_INITIALIZERS
#include <bsl_initializer_list.h>
#endif
#ifdef BSLS_COMPILERFEATURES_SUPPORT_TRAITS_HEADER
#include <bsl_type_traits.h>
#endif

#if BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES
// Include version that can be compiled with C++03
// Generated on Wed Jun 18 14:44:06 2025
// Command line: sim_cpp11_features.pl bmqu_operationchain.h
# define COMPILING_BMQU_OPERATIONCHAIN_H
# include <bmqu_operationchain_cpp03.h>
#undef COMPILING_BMQU_OPERATIONCHAIN_H
#else

namespace BloombergLP {

namespace bmqu {

// FORWARD DECLARATION
class OperationChain;
class OperationChainLink;
class OperationChain_Job;

// ====================================================
// struct OperationChain_IsCompletionCallbackCompatible
// ====================================================

/// A metafunction returning whether or not the specified `TYPE` satisfies
/// requirements for a completion callback. To satisfy those requirements
/// `TYPE` must meet the requirements of Destructible and MoveConstructible
/// as specified in the C++ standard.
///
/// Note that unless compiled with C++17 support, this metafunction always
/// returns `true`.
template <class TYPE>
struct OperationChain_IsCompletionCallbackCompatible
#if __cplusplus >= 201703L
: bsl::integral_constant<
      bool,
      bsl::is_destructible_v<bsl::decay_t<TYPE> > &&
          bsl::is_move_constructible_v<bsl::decay_t<TYPE> > >
#else
: bsl::integral_constant<bool, true>
#endif
{
};

// ===================================================
// struct OperationChain_IsOperationCallbackCompatible
// ===================================================

/// A metafunction returning whether or not the specified `TYPE` satisfies
/// requirements for a operation callback. To satisfy those requirements
/// `TYPE` must meet the requirements of Destructible and MoveConstructible
/// as specified in the C++ standard, as well as be Invocable with a single
/// parameter that is a functor object of unspecified type taking an
/// arbitrary number of template arguments.
///
/// Note that unless compiled with C++17 support, this metafunction always
/// returns `true`.
template <class TYPE>
struct OperationChain_IsOperationCallbackCompatible
#if __cplusplus >= 201703L
: bsl::integral_constant<
      bool,
      bsl::is_destructible_v<bsl::decay_t<TYPE> > &&
          bsl::is_move_constructible_v<bsl::decay_t<TYPE> > &&
          bsl::is_invocable_v<bsl::add_rvalue_reference_t<bsl::decay_t<TYPE> >,
                              bdlf::NoOp> >
#else
: bsl::integral_constant<bool, true>
#endif
{
};

// ==============================================
// class OperationChain_CompletionCallbackWrapper
// ==============================================

/// A wrapper around a completion callback object that invokes the callback
/// and notifies the operation chain.
template <class CO_CALLBACK>
class OperationChain_CompletionCallbackWrapper {
    // PRECONDITIONS
    BSLMF_ASSERT(
        OperationChain_IsCompletionCallbackCompatible<CO_CALLBACK>::value);

  public:
    // TYPES

    /// Defines the result type of the call operator.
    typedef void ResultType;

    typedef bsl::list<OperationChain_Job>::iterator JobHandle;

  private:
    // PRIVATE DATA
    OperationChain* d_chain_p;

    JobHandle d_jobHandle;

    CO_CALLBACK* d_coCallback_p;

  public:
    // CREATORS

    /// Create a `OperationChain_CompletionCallbackWrapper` object having
    /// the associated operation `chain`, `jobHandle` and `coCallback`
    /// completion callback.
    OperationChain_CompletionCallbackWrapper(OperationChain* chain,
                                             JobHandle       jobHandle,
                                             CO_CALLBACK*    coCallback)
        BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS
#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES  // $var-args=9

    /// Invoke the associated completion callback with the specified `args`
    /// arguments and notify the associated operation chain. Propagate any
    /// exception thrown by the completion callback to the caller.
    template <class... ARGS>
    void operator()(ARGS&&... args) const;
#endif

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(OperationChain_CompletionCallbackWrapper,
                                   bsl::is_nothrow_move_constructible)
};

// ========================
// class OperationChain_Job
// ========================

/// A job representing an async operation to be executed and accounted for.
class OperationChain_Job {
  public:
    // TYPES
    typedef bsl::list<OperationChain_Job>::iterator JobHandle;

  private:
    // PRIVATE TYPES

    /// An interface used to implement the type erasure technique.
    class TargetBase {
      public:
        // CREATORS

        /// Destroy this object and the contained callback objects with it.
        virtual ~TargetBase();

      public:
        // MANIPULATORS

        /// Start executing the associated async operation bound to the
        /// contained operation callback. On completion, invoke the
        /// contained completion callback and notify the specified operation
        /// `chain`.
        virtual void execute(OperationChain* chain, JobHandle jobHandle) = 0;
    };

    /// Provides an implementation of the `TargetBase` interface.
    template <class OP_CALLBACK, class CO_CALLBACK>
    class Target : public TargetBase {
        // PRECONDITIONS
        BSLMF_ASSERT(
            OperationChain_IsOperationCallbackCompatible<OP_CALLBACK>::value);
        BSLMF_ASSERT(
            OperationChain_IsCompletionCallbackCompatible<CO_CALLBACK>::value);

      private:
        // PRIVATE DATA
        bslalg::ConstructorProxy<OP_CALLBACK> d_opCallback;
        bslalg::ConstructorProxy<CO_CALLBACK> d_coCallback;

      private:
        // NOT IMPLEMENTED
        Target(const Target&) BSLS_KEYWORD_DELETED;
        Target& operator=(const Target&) BSLS_KEYWORD_DELETED;

      public:
        // CREATORS

        /// Create a `Target` object containing the specified `opCallback`
        /// operation callback and `coCallback` completion callback. Specify
        /// an `allocator` used to supply memory.
        template <class OP_CALLBACK_ARG, class CO_CALLBACK_ARG>
        Target(BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK_ARG) opCallback,
               BSLS_COMPILERFEATURES_FORWARD_REF(CO_CALLBACK_ARG) coCallback,
               bslma::Allocator* allocator);

      public:
        // MANIPULATORS

        /// Implements `TargetBase::execute`.
        void execute(OperationChain* chain,
                     JobHandle       jobHandle) BSLS_KEYWORD_OVERRIDE;
    };

    /// A "small" dummy functor used to help calculate the size of the
    /// on-stack buffer.
    struct Dummy : public bdlf::NoOp {
        void* d_padding[5];
    };

  private:
    // PRIVATE DATA

    // Job ID, used to tell the number of jobs in a link. This value is
    // unique per link, starts with 1, and is incremented with each new
    // job inserted into a link.
    unsigned d_id;

    // Uses an on-stack buffer to allocate memory for "small" objects, and
    // falls back to requesting memory from the supplied allocator if
    // the buffer is not large enough. Note that the size of the on-stack
    // buffer is an arbitrary value.
    bmqu::ObjectPlaceHolder<sizeof(Target<Dummy, Dummy>)> d_target;

  private:
    // NOT IMPLEMENTED
    OperationChain_Job(const OperationChain_Job&) BSLS_KEYWORD_DELETED;
    OperationChain_Job&
    operator=(const OperationChain_Job&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `OperationChain_Job` object having the specified `id` and
    /// containing the specified `opCallback` operation callback and
    /// `coCallback` completion callback. Specify an `allocator` used to
    /// supply memory.
    ///
    /// `bsl::decay_t<OP_CALLBACK>` and `bsl::decay_t<CO_CALLBACK>` must
    /// meet the requirements of Destructible and MoveConstructible as
    /// specified in the C++ standard. Given an object `f1` of type
    /// `bsl::decay_t<OP_CALLBACK>&&`, `f1(f2)` shall be a valid
    /// expression, where `f2` is a function object of unspecified type
    /// callable with the same arguments as `bsl::decay_t<CO_CALLBACK>&&`.
    template <class OP_CALLBACK, class CO_CALLBACK>
    explicit OperationChain_Job(unsigned id,
                                BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK)
                                    opCallback,
                                BSLS_COMPILERFEATURES_FORWARD_REF(CO_CALLBACK)
                                    coCallback,
                                bslma::Allocator* allocator);

    /// Destroy this object and the contained callback objects with it.
    ~OperationChain_Job();

  public:
    // MANIPULATORS

    /// Start executing the associated async operation bound to the
    /// contained operation callback. On completion, invoke the contained
    /// completion callback and notify the specified operation `chain` using
    /// the specified `jobHandle`.
    void execute(OperationChain* chain, JobHandle jobHandle) BSLS_NOTHROW_SPEC;

  public:
    // ACCESSORS

    /// Return the ID of this job.
    unsigned id() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(OperationChain_Job,
                                   bslma::UsesBslmaAllocator)
};

// ====================
// class OperationChain
// ====================

/// A mechanism to serialize execution of async operations.
class OperationChain {
  public:
    // TYPES
    typedef OperationChainLink Link;

  private:
    // PRIVATE TYPES
    typedef OperationChain_Job Job;

    typedef bsl::list<Job> JobList;

    typedef JobList::iterator JobHandle;

    typedef bslmt::LockGuard<bslmt::Mutex> LockGuard;

  private:
    // PRIVATE DATA

    // Used for general thread-safety.
    mutable bslmt::Mutex d_mutex;

    // Used to synchronize with completion of async operations.
    mutable bslmt::Condition d_condition;

    // Whether or not this operation chain is started.
    bool d_isStarted;

    // The number of links in this operation chain.
    unsigned d_numLinks;

    // The number of jobs currently executing.
    unsigned d_numJobsRunning;

    // Jobs executing async operations.
    JobList d_jobList;

    // FRIENDS
    template <class>
    friend class OperationChain_CompletionCallbackWrapper;

  private:
    // PRIVATE MANIPULATORS

    /// Callback invoked to notify this operation chain an async
    /// operation identified by the specified `handle` has completed.
    void onOperationCompleted(JobHandle handle) BSLS_KEYWORD_NOEXCEPT;

    /// Execute all operations in the first link of this operation chain.
    /// Unlock the mutex associated with the specified 'lock' guard and
    /// release the guard.
    void run(LockGuard* lock) BSLS_KEYWORD_NOEXCEPT;

  private:
    // NOT IMPLEMENTED
    OperationChain(const OperationChain&) BSLS_KEYWORD_DELETED;
    OperationChain& operator=(const OperationChain&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `OperationChain` object. Optionally specify a
    /// `createStarted` flag used to create the chain in a started state. If
    /// the flag is not specified or its value is `false`, create the chain
    /// in a stopped state. Optionally specify a `basicAllocator` used to
    /// supply memory. If `basicAllocator` is 0, the default memory
    /// allocator is used.
    explicit OperationChain(bslma::Allocator* basicAllocator = 0);
    explicit OperationChain(bool              createStarted,
                            bslma::Allocator* basicAllocator = 0);

    /// Destroy this object. Call `stop()` followed by `join()`. Destroy all
    /// operation and completion callback objects left in the chain, if any.
    ~OperationChain();

  public:
    // MANIPULATORS

    /// Start executing operations in this operation chain. If the chain is
    /// already started, this function has no effect.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    void start();

    /// Stop executing operations in this operation chain without blocking
    /// the calling thread pending completion of any currently executing
    /// operation. If the chain is already stopped, this function has no
    /// effect.
    void stop() BSLS_KEYWORD_NOEXCEPT;

    /// Block the calling thread until there is no operations executing
    /// in this operation chain. If there is no operations currently
    /// executing, return immediately.
    void join() BSLS_KEYWORD_NOEXCEPT;

    /// Append the specified `link` to this operation chain. If the chain is
    /// started and the appended `link` is the first one in the chain,
    /// execute all operations in the appended `link` immediately. If the
    /// appended `link` is empty, this function has no effect. After this
    /// operation completes, `link` is left empty. The behavior is undefined
    /// unless `link` uses the same allocator as this object.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    void append(Link* link);
    void append(bslmf::MovableRef<Link> link);

#ifdef BSLS_COMPILERFEATURES_SUPPORT_GENERALIZED_INITIALIZERS
    /// Append all links from the specified `links` list to this operation
    /// chain in the order they appear in the list. If the chain is started
    /// and the first appended link is the first one in the chain, execute
    /// all operations in the first appended link immediately. If the
    /// `links` list is empty, this function has no effect. If the `links`
    /// list contains empty links, ignore them. After this operation
    /// completes, all links in the `links` list are left empty. The
    /// behavior is undefined unless all links in the `links` list use the
    /// same allocator as this object.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    void append(bsl::initializer_list<Link*> links);
#endif

    /// Append the specified `count` number of links from the specified
    /// `links` array to this operation chain in the order they appear in
    /// the array. If the chain is started and the first appended link is
    /// the first one in the chain, execute all operations in the first
    /// appended link immediately. If `count` is 0, this function has no
    /// effect. If the `links` array contains empty links, ignore them.
    /// After this operation completes, all links in the `links` array are
    /// left empty. The behavior is undefined unless all links in the
    /// `links` array use the same allocator as this object.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    void append(Link* const* links, size_t count);

    /// Append a link containing a single operation represented by the
    /// specified `opCallback` operation callback and the optionally
    /// specified `coCallback` completion callback to this operation chain.
    /// If no completion callback is specified, use `bdlf::NoOp`. If the
    /// chain is started and the appended link is the first one in the
    /// chain, execute the single operation in the appended link
    /// immediately.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    ///
    /// `bsl::decay_t<OP_CALLBACK>` and `bsl::decay_t<CO_CALLBACK>` must
    /// meet the requirements of Destructible and MoveConstructible as
    /// specified in the C++ standard. Given an object `f1` of type
    /// `bsl::decay_t<OP_CALLBACK>&&`, `f1(f2)` shall be a valid
    /// expression, where `f2` is a function object of unspecified type
    /// callable with the same arguments as `bsl::decay_t<CO_CALLBACK>&&`.
    template <class OP_CALLBACK>
    void appendInplace(BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK)
                           opCallback);
    template <class OP_CALLBACK, class CO_CALLBACK>
    void appendInplace(BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK)
                           opCallback,
                       BSLS_COMPILERFEATURES_FORWARD_REF(CO_CALLBACK)
                           coCallback);

    int popBack() BSLS_KEYWORD_NOEXCEPT;

    /// Extract and remove the last link from this operation chain and load
    /// it into the optionally specified `link`. Return 0 on success, and a
    /// non-zero value if the link can not be extracted, either because the
    /// chain is empty, or because operations in the extracted link are
    /// currently executing. On failure, this function has no effect and the
    /// contents of `link` are unmodified. The behavior is undefined unless
    /// `link` uses that same allocator as this object.
    int popBack(Link* link) BSLS_KEYWORD_NOEXCEPT;

    /// Remove all links from this operation chain. If operations in this
    /// chain are currently executing, do not remove the first link. Return
    /// the number of operations removed. If this chain is empty, this
    /// function has no effect and 0 is returned.
    size_t removeAll() BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return `true` if this operation chain is started, and `false`
    /// otherwise. Note that a stopped chain can still be executing
    /// operations.
    bool isStarted() const BSLS_KEYWORD_NOEXCEPT;

    /// Return `true` if this operation chain is currently executing at
    /// least one operation, and `false` otherwise.
    bool isRunning() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the number of links in this operation chain. A value of 0
    /// means the chain is empty.
    size_t numLinks() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the number of operations in this operation chain, including
    /// those that are currently executing (if any). A value of 0 means the
    /// chain is empty.
    size_t numOperations() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the number of operations in this operation chain that are not
    /// currently executing.
    size_t numOperationsPending() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the number of operations in this operation chain that are
    /// currently executing.
    size_t numOperationsExecuting() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the allocator used by this operation chain to supply memory.
    bslma::Allocator* allocator() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(OperationChain, bslma::UsesBslmaAllocator)
};

// ========================
// class OperationChainLink
// ========================

/// A representation of a link in the operation chain.
class OperationChainLink {
  private:
    // PRIVATE TYPES
    typedef OperationChain_Job Job;

    typedef bsl::list<Job> JobList;

  private:
    // PRIVATE DATA

    // Jobs executing async operations.
    JobList d_jobList;

    // FRIENDS
    friend class OperationChain;

  private:
    // NOT IMPLEMENTED
    OperationChainLink(const OperationChainLink&) BSLS_KEYWORD_DELETED;
    OperationChainLink&
    operator=(const OperationChainLink&) BSLS_KEYWORD_DELETED;

  public:
    // CREATORS

    /// Create a `OperationChainLink` object initialized empty. Optionally
    /// specify a `basicAllocator` used to supply memory. If
    /// `basicAllocator` is 0, the default memory allocator is used.
    explicit OperationChainLink(bslma::Allocator* basicAllocator = 0);

    /// Create a `OperationChainLink` object having the same value as the
    /// specified `original` object by moving the contents of `original` to
    /// the new link. The allocator associated with `original` is propagated
    /// for use in the newly-created link. `original` is left empty.
    OperationChainLink(bslmf::MovableRef<OperationChainLink> original);

  public:
    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object and
    /// return a reference providing modifiable access to this object. The
    /// contents of `rhs` are moved to this link. `rhs` is left empty. The
    /// behavior is undefined unless `rhs` uses that same allocator as this
    /// object.
    OperationChainLink&
    operator=(bslmf::MovableRef<OperationChainLink> rhs) BSLS_KEYWORD_NOEXCEPT;

    /// Insert an operation into this link. Specify a `opCallback` operation
    /// callback initiating the operation. Optionally specify a `coCallback`
    /// completion callback to be passed to the operation callback as a
    /// parameter. If no completion callback is specified, use `bdlf::NoOp`.
    ///
    /// This function meets the strong exception guarantee. If an exception
    /// is thrown, this function has no effect.
    ///
    /// `bsl::decay_t<OP_CALLBACK>` and `bsl::decay_t<CO_CALLBACK>` must
    /// meet the requirements of Destructible and MoveConstructible as
    /// specified in the C++ standard. Given an object `f1` of type
    /// `bsl::decay_t<OP_CALLBACK>&&`, `f1(f2)` shall be a valid
    /// expression, where `f2` is a function object of unspecified type
    /// callable with the same arguments as `bsl::decay_t<CO_CALLBACK>&&`.
    template <class OP_CALLBACK>
    void insert(BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK) opCallback);
    template <class OP_CALLBACK, class CO_CALLBACK>
    void insert(BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK) opCallback,
                BSLS_COMPILERFEATURES_FORWARD_REF(CO_CALLBACK) coCallback);

    /// Remove all operations in this link, if any. Return the number of
    /// operations removed.
    size_t removeAll() BSLS_KEYWORD_NOEXCEPT;

    /// Swap the contents of this object and the specified `other` object.
    /// The behavior is undefined unless `other` uses that same allocator
    /// as this object.
    void swap(OperationChainLink& other) BSLS_KEYWORD_NOEXCEPT;

  public:
    // ACCESSORS

    /// Return the number of operations in this link. A value of 0 means the
    /// link is empty.
    size_t numOperations() const BSLS_KEYWORD_NOEXCEPT;

    /// Return the allocator used by this link to supply memory.
    bslma::Allocator* allocator() const BSLS_KEYWORD_NOEXCEPT;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(OperationChainLink,
                                   bslma::UsesBslmaAllocator)
};

// FREE OPERATORS

/// Swap the contents of `lhs` and `rhs`. The behavior is undefined unless
/// `lhs` and `rhs` use the same allocator.
void swap(OperationChainLink& lhs,
          OperationChainLink& rhs) BSLS_KEYWORD_NOEXCEPT;

// ============================================================================
//                            INLINE DEFINITIONS
// ============================================================================

// ----------------------------------------------
// class OperationChain_CompletionCallbackWrapper
// ----------------------------------------------

// CREATORS
template <class CO_CALLBACK>
inline OperationChain_CompletionCallbackWrapper<CO_CALLBACK>::
    OperationChain_CompletionCallbackWrapper(OperationChain* chain,
                                             JobHandle       jobHandle,
                                             CO_CALLBACK*    coCallback)
        BSLS_KEYWORD_NOEXCEPT : d_chain_p(chain),
                                d_jobHandle(jobHandle),
                                d_coCallback_p(coCallback)
{
    // PRECONDITIONS
    BSLS_ASSERT(chain);
    BSLS_ASSERT(coCallback);
}

// ACCESSORS
#if !BSLS_COMPILERFEATURES_SIMULATE_CPP11_FEATURES  // $var-args=9
template <class CO_CALLBACK>
template <class... ARGS>
inline void OperationChain_CompletionCallbackWrapper<CO_CALLBACK>::operator()(
    ARGS&&... args) const
{
    try {
        // invoke completion callback
        bslmf::Util::moveIfSupported((*d_coCallback_p))(
            bslmf::Util::forward<ARGS>(args)...);
    }
    catch (...) {
        // notify the chain and rethrow the exception
        d_chain_p->onOperationCompleted(d_jobHandle);
        throw;  // THROW
    }

    // notify the chain
    d_chain_p->onOperationCompleted(d_jobHandle);
}
#endif

// ------------------------
// class OperationChain_Job
// ------------------------

// CREATORS
template <class OP_CALLBACK, class CO_CALLBACK>
inline OperationChain_Job::OperationChain_Job(
    unsigned id,
    BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK) opCallback,
    BSLS_COMPILERFEATURES_FORWARD_REF(CO_CALLBACK) coCallback,
    bslma::Allocator* allocator)
: d_id(id)
{
    // PRECONDITIONS
    BSLS_ASSERT(allocator);

    typedef Target<typename bsl::decay<OP_CALLBACK>::type,
                   typename bsl::decay<CO_CALLBACK>::type>
        Target;

    d_target.createObject<Target>(
        allocator,
        BSLS_COMPILERFEATURES_FORWARD(OP_CALLBACK, opCallback),
        BSLS_COMPILERFEATURES_FORWARD(CO_CALLBACK, coCallback),
        allocator);
}

// CREATORS
inline OperationChain_Job::~OperationChain_Job()
{
    d_target.deleteObject<TargetBase>();
}

// MANIPULATORS
inline void OperationChain_Job::execute(OperationChain* chain,
                                        JobHandle jobHandle) BSLS_NOTHROW_SPEC
{
    // NOTE: 'BSLS_NOTHROW_SPEC' specification ensures that 'bsl::terminate' is
    //       called in case of exception in C++03.

    // PRECONDITIONS
    BSLS_ASSERT(chain);

    d_target.object<TargetBase>()->execute(chain, jobHandle);
}

// ACCESSORS
inline unsigned OperationChain_Job::id() const BSLS_KEYWORD_NOEXCEPT
{
    return d_id;
}

// --------------------------------
// class OperationChain_Job::Target
// --------------------------------

// CREATORS
template <class OP_CALLBACK, class CO_CALLBACK>
template <class OP_CALLBACK_ARG, class CO_CALLBACK_ARG>
inline OperationChain_Job::Target<OP_CALLBACK, CO_CALLBACK>::Target(
    BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK_ARG) opCallback,
    BSLS_COMPILERFEATURES_FORWARD_REF(CO_CALLBACK_ARG) coCallback,
    bslma::Allocator* allocator)
: d_opCallback(BSLS_COMPILERFEATURES_FORWARD(OP_CALLBACK_ARG, opCallback),
               allocator)
, d_coCallback(BSLS_COMPILERFEATURES_FORWARD(CO_CALLBACK_ARG, coCallback),
               allocator)
{
    // PRECONDITIONS
    BSLMF_ASSERT(
        OperationChain_IsOperationCallbackCompatible<OP_CALLBACK_ARG>::value);
    BSLMF_ASSERT(
        OperationChain_IsCompletionCallbackCompatible<CO_CALLBACK_ARG>::value);
    BSLS_ASSERT(allocator);
}

// MANIPULATORS
template <class OP_CALLBACK, class CO_CALLBACK>
inline void OperationChain_Job::Target<OP_CALLBACK, CO_CALLBACK>::execute(
    OperationChain* chain,
    JobHandle       jobHandle)
{
    // PRECONDITIONS
    BSLS_ASSERT(chain);

    typedef OperationChain_CompletionCallbackWrapper<CO_CALLBACK> CoCbWrapper;

    bslmf::Util::moveIfSupported(d_opCallback.object())(
        CoCbWrapper(chain, jobHandle, &d_coCallback.object()));
}

// --------------------
// class OperationChain
// --------------------

// MANIPULATORS
inline void OperationChain::append(Link* link)
{
    append(&link, 1);
}

inline void OperationChain::append(bslmf::MovableRef<Link> link)
{
    Link* link_p = &bslmf::MovableRefUtil::access(link);
    append(&link_p, 1);
}

#ifdef BSLS_COMPILERFEATURES_SUPPORT_GENERALIZED_INITIALIZERS
inline void OperationChain::append(bsl::initializer_list<Link*> links)
{
    append(links.begin(), links.size());
}
#endif  // BSLS_COMPILERFEATURES_SUPPORT_GENERALIZED_INITIALIZERS

template <class OP_CALLBACK>
inline void
OperationChain::appendInplace(BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK)
                                  opCallback)
{
    // PRECONDITIONS
    BSLMF_ASSERT(
        OperationChain_IsOperationCallbackCompatible<OP_CALLBACK>::value);

    appendInplace(BSLS_COMPILERFEATURES_FORWARD(OP_CALLBACK, opCallback),
                  bdlf::noOp);
}

template <class OP_CALLBACK, class CO_CALLBACK>
inline void OperationChain::appendInplace(
    BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK) opCallback,
    BSLS_COMPILERFEATURES_FORWARD_REF(CO_CALLBACK) coCallback)
{
    // PRECONDITIONS
    BSLMF_ASSERT(
        OperationChain_IsOperationCallbackCompatible<OP_CALLBACK>::value);
    BSLMF_ASSERT(
        OperationChain_IsCompletionCallbackCompatible<CO_CALLBACK>::value);

    // create a link
    Link link(allocator());
    link.insert(BSLS_COMPILERFEATURES_FORWARD(OP_CALLBACK, opCallback),
                BSLS_COMPILERFEATURES_FORWARD(CO_CALLBACK, coCallback));

    // append it
    append(&link);
}

// ------------------------
// class OperationChainLink
// ------------------------

// MANIPULATORS
template <class OP_CALLBACK>
inline void
OperationChainLink::insert(BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK)
                               opCallback)
{
    // PRECONDITIONS
    BSLMF_ASSERT(
        OperationChain_IsOperationCallbackCompatible<OP_CALLBACK>::value);

    insert(BSLS_COMPILERFEATURES_FORWARD(OP_CALLBACK, opCallback), bdlf::noOp);
}

template <class OP_CALLBACK, class CO_CALLBACK>
inline void OperationChainLink::insert(
    BSLS_COMPILERFEATURES_FORWARD_REF(OP_CALLBACK) opCallback,
    BSLS_COMPILERFEATURES_FORWARD_REF(CO_CALLBACK) coCallback)
{
    // PRECONDITIONS
    BSLMF_ASSERT(
        OperationChain_IsOperationCallbackCompatible<OP_CALLBACK>::value);
    BSLMF_ASSERT(
        OperationChain_IsCompletionCallbackCompatible<CO_CALLBACK>::value);

    // create a job
    d_jobList.emplace_front(
        d_jobList.size() + 1,  // id
        BSLS_COMPILERFEATURES_FORWARD(OP_CALLBACK, opCallback),
        BSLS_COMPILERFEATURES_FORWARD(CO_CALLBACK, coCallback));
}

}  // close package namespace

// FREE OPERATORS
inline void bmqu::swap(OperationChainLink& lhs,
                       OperationChainLink& rhs) BSLS_KEYWORD_NOEXCEPT
{
    lhs.swap(rhs);
}

}  // close enterprise namespace

#endif  // End C++11 code

#endif
