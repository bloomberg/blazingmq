// Copyright 2025 Bloomberg Finance L.P.
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

// bmqu_managedcallback.t.cpp                                         -*-C++-*-
#include <bmqu_managedcallback.h>

// BDE
#include <bsl_cstdlib.h>
#include <bsl_ctime.h>
#include <bsl_functional.h>
#include <bsl_string.h>
#include <bsl_unordered_set.h>
#include <bsl_utility.h>
#include <bsla_annotations.h>
#include <bsls_timeutil.h>

// BMQ
#include <bmqu_printutil.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------
namespace {

static void
printSummary(bslstl::StringRef desc, bsls::Types::Int64 dt, size_t iters)
{
    bsl::cout << desc << ":" << bsl::endl;
    bsl::cout << "       total: " << bmqu::PrintUtil::prettyTimeInterval(dt)
              << " (" << iters << " iterations)" << bsl::endl;
    bsl::cout << "    per call: "
              << bmqu::PrintUtil::prettyTimeInterval((dt) / iters)
              << bsl::endl;
    bsl::cout << bsl::endl;
}

void increment(size_t* calls)
{
    ++(*calls);
}

struct IncrementCallback : public bmqu::ManagedCallback::CallbackFunctor {
    size_t* d_calls_p;

    explicit IncrementCallback(size_t* calls)
    : d_calls_p(calls)
    {
        BSLS_ASSERT_SAFE(calls);
    }

    ~IncrementCallback() BSLS_KEYWORD_OVERRIDE
    {
        // NOTHING
    }

    void operator()() const BSLS_KEYWORD_OVERRIDE { ++(*d_calls_p); }
};

void complexFunction(size_t*     calls,
                     BSLA_UNUSED bsls::Types::Uint64 arg1,
                     BSLA_UNUSED bsls::Types::Uint64 arg2,
                     BSLA_UNUSED bsls::Types::Uint64 arg3,
                     BSLA_UNUSED bsls::Types::Uint64 arg4,
                     BSLA_UNUSED bsls::Types::Uint64 arg5,
                     BSLA_UNUSED bsls::Types::Uint64 arg6,
                     BSLA_UNUSED bsls::Types::Uint64 arg7,
                     BSLA_UNUSED bsls::Types::Uint64 arg8)
{
    ++(*calls);
}

struct ComplexCallback : bmqu::ManagedCallback::CallbackFunctor {
    size_t*             d_calls_p;
    bsls::Types::Uint64 d_arg1;
    bsls::Types::Uint64 d_arg2;
    bsls::Types::Uint64 d_arg3;
    bsls::Types::Uint64 d_arg4;
    bsls::Types::Uint64 d_arg5;
    bsls::Types::Uint64 d_arg6;
    bsls::Types::Uint64 d_arg7;
    bsls::Types::Uint64 d_arg8;

    explicit ComplexCallback(size_t*             calls,
                             bsls::Types::Uint64 arg1,
                             bsls::Types::Uint64 arg2,
                             bsls::Types::Uint64 arg3,
                             bsls::Types::Uint64 arg4,
                             bsls::Types::Uint64 arg5,
                             bsls::Types::Uint64 arg6,
                             bsls::Types::Uint64 arg7,
                             bsls::Types::Uint64 arg8)
    : d_calls_p(calls)
    , d_arg1(arg1)
    , d_arg2(arg2)
    , d_arg3(arg3)
    , d_arg4(arg4)
    , d_arg5(arg5)
    , d_arg6(arg6)
    , d_arg7(arg7)
    , d_arg8(arg8)
    {
        BSLS_ASSERT_SAFE(calls);
    }

    ~ComplexCallback() BSLS_KEYWORD_OVERRIDE
    {
        // NOTHING
    }

    void operator()() const BSLS_KEYWORD_OVERRIDE { ++(*d_calls_p); }
};

struct BigCallback : public bmqu::ManagedCallback::CallbackFunctor {
    size_t* d_calls_p;
    char    d_data[4096];

    explicit BigCallback(size_t* calls)
    : d_calls_p(calls)
    , d_data()
    {
        BSLS_ASSERT_SAFE(calls);
    }

    ~BigCallback() BSLS_KEYWORD_OVERRIDE
    {
        // NOTHING
    }

    void operator()() const BSLS_KEYWORD_OVERRIDE { ++(*d_calls_p); }
};

struct DestructorChecker : public bmqu::ManagedCallback::CallbackFunctor {
    bool* d_wasDestructorCalled_p;

    explicit DestructorChecker(bool* wasDestructorCalled)
    : d_wasDestructorCalled_p(wasDestructorCalled)
    {
        BSLS_ASSERT_SAFE(wasDestructorCalled);

        *d_wasDestructorCalled_p = false;
    }

    ~DestructorChecker() BSLS_KEYWORD_OVERRIDE
    {
        *d_wasDestructorCalled_p = true;
    }

    void operator()() const BSLS_KEYWORD_OVERRIDE
    {
        // NOTHING
    }
};

class ManagedCallbackBuffer BSLS_KEYWORD_FINAL {
  private:
    // DATA
    /// Reusable buffer holding the stored callback.
    char d_callbackBuffer[256];

    /// The flag indicating if `d_callbackBuffer` is empty now.
    bool d_empty;

  public:
    // TRAITS
    BSLA_MAYBE_UNUSED
    BSLMF_NESTED_TRAIT_DECLARATION(ManagedCallbackBuffer,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    inline ManagedCallbackBuffer()
    : d_callbackBuffer()
    , d_empty(true)
    {
        // NOTHING
    }

    inline ~ManagedCallbackBuffer() { reset(); }

    // MANIPULATORS
    inline void reset()
    {
        if (!d_empty) {
            // Not necessary to resize the vector or memset its elements to 0,
            // we just call the virtual destructor, and `d_empty` flag
            // prevents us from calling outdated callback.
            reinterpret_cast<bmqu::ManagedCallback::CallbackFunctor*>(
                d_callbackBuffer)
                ->~CallbackFunctor();
            d_empty = true;
        }
    }

    /// Note: this class exists for evaluation only, and we don't define
    ///       `createInplace` for it, because it requires generation of C++03
    ///       compatible code.  We don't want to make UTs more complicated with
    ///       it, so we only define a simpler function `place` that is used
    ///       with "placement new".
    template <class CALLBACK_TYPE>
    inline char* place()
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(d_empty);
        /// The compilation will fail here on the outer `static_cast` if we
        /// don't provide a type that is inherited from the base
        /// `CallbackFunctor` type.
        /// TODO: replace by static_assert on C++ standard update
        BSLS_ASSERT_SAFE(
            0 ==
            static_cast<CALLBACK_TYPE*>(
                reinterpret_cast<bmqu::ManagedCallback::CallbackFunctor*>(0)));
        d_empty = false;
        return d_callbackBuffer;
    }

    // ACCESSORS

    BSLA_MAYBE_UNUSED
    inline bool empty() const { return d_empty; }

    inline void operator()() const
    {
        // PRECONDITIONS
        BSLS_ASSERT_SAFE(!d_empty);

        (*reinterpret_cast<const bmqu::ManagedCallback::CallbackFunctor*>(
            d_callbackBuffer))();
    }
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------
static void test1_ManagedCallback()
{
    bmqtst::TestHelper::printTestName("MANAGED CALLBACK");

    bsl::vector<bsls::Types::Uint64> args(bmqtst::TestHelperUtil::allocator());
    args.resize(8, 0);

    bmqu::ManagedCallback callback(bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT(callback.empty());

    // 1. Basic create, call, reset
    size_t calls1 = 0;
    callback.createInplace<IncrementCallback>(&calls1);
    BMQTST_ASSERT(!callback.empty());

    callback();
    BMQTST_ASSERT_EQ(1U, calls1);

    callback.reset();
    BMQTST_ASSERT(callback.empty());

    // 2. Reuse callback and make multiple calls
    size_t calls2 = 0;
    callback.createInplace<IncrementCallback>(&calls2);
    BMQTST_ASSERT(!callback.empty());

    for (size_t i = 1; i <= 5; ++i) {
        callback();
        BMQTST_ASSERT_EQ(i, calls2);
    }
    BMQTST_ASSERT_EQ(1U, calls1);

    callback.reset();
    BMQTST_ASSERT(callback.empty());

    // 3. Compatibility with `bsl::function` using `set(...)`
    size_t calls3 = 0;
    callback.set(bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                       increment,
                                       &calls3));
    BMQTST_ASSERT(!callback.empty());

    callback();
    BMQTST_ASSERT_EQ(1U, calls3);

    callback.reset();
    BMQTST_ASSERT(callback.empty());

    // 4. Big functor object support
    size_t calls4 = 0;
    callback.createInplace<BigCallback>(&calls4);
    BMQTST_ASSERT(!callback.empty());

    callback();
    BMQTST_ASSERT_EQ(1U, calls4);

    callback.reset();
    BMQTST_ASSERT(callback.empty());

    // 5. Multiple arguments support
    size_t calls5 = 0;
    callback.createInplace<ComplexCallback>(&calls5,
                                            args[0],
                                            args[1],
                                            args[2],
                                            args[3],
                                            args[4],
                                            args[5],
                                            args[6],
                                            args[7]);
    BMQTST_ASSERT(!callback.empty());

    callback();
    BMQTST_ASSERT_EQ(1U, calls5);

    callback.reset();
    BMQTST_ASSERT(callback.empty());

    // 6. Multiple reuse
    size_t calls6 = 0;
    for (size_t i = 1; i <= 100000; i++) {
        if (i % 3 == 0) {
            callback.createInplace<IncrementCallback>(&calls6);
        }
        else if (i % 3 == 1) {
            callback.createInplace<BigCallback>(&calls6);
        }
        else {
            callback.createInplace<ComplexCallback>(&calls6,
                                                    args[0],
                                                    args[1],
                                                    args[2],
                                                    args[3],
                                                    args[4],
                                                    args[5],
                                                    args[6],
                                                    args[7]);
        }
        BMQTST_ASSERT(!callback.empty());

        callback();
        BMQTST_ASSERT_EQ(i, calls6);

        callback.reset();
        BMQTST_ASSERT(callback.empty());
    }

    // 7. ManagedCallback destructor destructs the stored callback object
    bool wasDestructorCalled = false;
    {
        bmqu::ManagedCallback scopedCallback(
            bmqtst::TestHelperUtil::allocator());
        scopedCallback.createInplace<DestructorChecker>(&wasDestructorCalled);
        BMQTST_ASSERT(!scopedCallback.empty());

        // `scopedCallback` is destructed on exiting the scope
    }
    BMQTST_ASSERT(wasDestructorCalled);
}

static void testN1_ManagedCallbackPerformance()
{
    const size_t k_ITERS_NUM = 10000000;

    // `calls` is used everywhere to prevent optimizing out anything.
    size_t                           calls = 0;
    bsl::vector<bsls::Types::Uint64> args(bmqtst::TestHelperUtil::allocator());
    args.resize(8, 0);

    // Warmup
    for (size_t i = 0; i < k_ITERS_NUM; i++) {
        bmqu::ManagedCallback callback(bmqtst::TestHelperUtil::allocator());
        callback.createInplace<IncrementCallback>(&calls);
        callback.reset();
    }

    bsl::cout << "========= Benchmark 1: function call ==========" << bsl::endl
              << "Build a functor once and call it multiple times" << bsl::endl
              << "===============================================" << bsl::endl
              << bsl::endl;
    {
        bsl::function<void(void)> callback;
        callback = bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                         increment,
                                         &calls);
        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            callback();
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        printSummary("bsl::function<...>()", end - begin, k_ITERS_NUM);
    }
    {
        bmqu::ManagedCallback callback(bmqtst::TestHelperUtil::allocator());
        callback.createInplace<IncrementCallback>(&calls);

        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            callback();
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        printSummary("bmqu::ManagedCallback(vector)",
                     end - begin,
                     k_ITERS_NUM);
    }
    {
        ManagedCallbackBuffer callback;
        new (callback.place<IncrementCallback>()) IncrementCallback(&calls);

        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            callback();
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        printSummary("bmqu::ManagedCallback(char[])",
                     end - begin,
                     k_ITERS_NUM);
    }

    bsl::cout << "========= Benchmark 2: new construct ==========" << bsl::endl
              << "Build a functor multiple times without calling" << bsl::endl
              << "===============================================" << bsl::endl
              << bsl::endl;
    {
        bsl::vector<bsl::function<void(void)> > cbs(
            bmqtst::TestHelperUtil::allocator());
        cbs.resize(k_ITERS_NUM);

        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            cbs[i] = bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                           increment,
                                           &calls);
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        printSummary("bsl::function<...>()", end - begin, k_ITERS_NUM);
    }
    {
        bsl::vector<bmqu::ManagedCallback*> cbs(
            bmqtst::TestHelperUtil::allocator());
        cbs.resize(k_ITERS_NUM);

        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            cbs[i] = new bmqu::ManagedCallback(
                bmqtst::TestHelperUtil::allocator());
            cbs[i]->createInplace<IncrementCallback>(&calls);
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            cbs[i]->reset();
            delete cbs[i];
        }

        printSummary("bmqu::ManagedCallback(vector)",
                     end - begin,
                     k_ITERS_NUM);
    }
    {
        bsl::vector<ManagedCallbackBuffer*> cbs(
            bmqtst::TestHelperUtil::allocator());
        cbs.resize(k_ITERS_NUM);

        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            cbs[i] = new ManagedCallbackBuffer();
            new (cbs[i]->place<IncrementCallback>()) IncrementCallback(&calls);
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            cbs[i]->reset();
            delete cbs[i];
        }

        printSummary("bmqu::ManagedCallback(char[])",
                     end - begin,
                     k_ITERS_NUM);
    }

    bsl::cout << "========= Benchmark 3: reuse functor ==========" << bsl::endl
              << "Reset and call a functor multiple times" << bsl::endl
              << "===============================================" << bsl::endl
              << bsl::endl;
    {
        bsl::function<void(void)> callback;

        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            callback = bdlf::BindUtil::bindS(
                bmqtst::TestHelperUtil::allocator(),
                increment,
                &calls);
            callback();
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        printSummary("bsl::function<...>()", end - begin, k_ITERS_NUM);
    }
    {
        bmqu::ManagedCallback callback(bmqtst::TestHelperUtil::allocator());

        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            callback.createInplace<IncrementCallback>(&calls);
            callback();
            callback.reset();
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        printSummary("bmqu::ManagedCallback(vector)",
                     end - begin,
                     k_ITERS_NUM);
    }
    {
        ManagedCallbackBuffer callback;

        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            new (callback.place<IncrementCallback>())
                IncrementCallback(&calls);
            callback();
            callback.reset();
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        printSummary("bmqu::ManagedCallback(char[])",
                     end - begin,
                     k_ITERS_NUM);
    }

    bsl::cout << "===== Benchmark 4: reuse complex functor ======" << bsl::endl
              << "Reset and call a complex functor multiple times" << bsl::endl
              << "===============================================" << bsl::endl
              << bsl::endl;
    {
        bsl::function<void(void)> callback;

        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            callback = bdlf::BindUtil::bindS(
                bmqtst::TestHelperUtil::allocator(),
                complexFunction,
                &calls,
                args[0],
                args[1],
                args[2],
                args[3],
                args[4],
                args[5],
                args[6],
                args[7]);
            callback();
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        printSummary("bsl::function<...>()", end - begin, k_ITERS_NUM);
    }
    {
        bmqu::ManagedCallback callback(bmqtst::TestHelperUtil::allocator());

        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            callback.createInplace<ComplexCallback>(&calls,
                                                    args[0],
                                                    args[1],
                                                    args[2],
                                                    args[3],
                                                    args[4],
                                                    args[5],
                                                    args[6],
                                                    args[7]);
            callback();
            callback.reset();
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        printSummary("bmqu::ManagedCallback(vector)",
                     end - begin,
                     k_ITERS_NUM);
    }
    {
        ManagedCallbackBuffer callback;

        const bsls::Types::Int64 begin = bsls::TimeUtil::getTimer();
        for (size_t i = 0; i < k_ITERS_NUM; i++) {
            new (callback.place<ComplexCallback>()) ComplexCallback(&calls,
                                                                    args[0],
                                                                    args[1],
                                                                    args[2],
                                                                    args[3],
                                                                    args[4],
                                                                    args[5],
                                                                    args[6],
                                                                    args[7]);
            callback();
            callback.reset();
        }
        const bsls::Types::Int64 end = bsls::TimeUtil::getTimer();

        printSummary("bmqu::ManagedCallback(char[])",
                     end - begin,
                     k_ITERS_NUM);
    }

    if (calls < 100000) {
        bsl::cout << calls << bsl::endl;
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // To be called only once per process instantiation.
    bsls::TimeUtil::initialize();

    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_ManagedCallback(); break;
    case -1: testN1_ManagedCallbackPerformance(); break;
    default: {
        bsl::cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND."
                  << bsl::endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
