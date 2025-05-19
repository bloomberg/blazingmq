// Copyright 2023 Bloomberg Finance L.P.
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

// bmqeval_simpleevaluator.t.cpp                                      -*-C++-*-
#include <bsl_unordered_map.h>

#include <bmqeval_simpleevaluator.h>
#include <bmqeval_simpleevaluatorparser.hpp>
#include <bmqeval_simpleevaluatorscanner.h>

// BENCHMARKING LIBRARY
#ifdef BSLS_PLATFORM_OS_LINUX
#include <benchmark/benchmark.h>
#endif

// TEST DRIVER
#include <bmqtst_testhelper.h>

#include <bdlma_localsequentialallocator.h>
#include <bsl_sstream.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bmqeval;
using namespace bsl;

/// Interface for a PropertiesReader.
class MockPropertiesReader : public PropertiesReader {
  public:
    // PUBLIC DATA
    bsl::unordered_map<bsl::string, bdld::Datum> d_map;

    // CREATORS
    MockPropertiesReader(bslma::Allocator* allocator)
    : d_map(allocator)
    {
        d_map["b_true"]  = bdld::Datum::createBoolean(true);
        d_map["b_false"] = bdld::Datum::createBoolean(false);
        d_map["i_0"]     = bdld::Datum::createInteger(0);
        d_map["i_1"]     = bdld::Datum::createInteger(1);
        d_map["i_2"]     = bdld::Datum::createInteger(2);
        d_map["i_3"]     = bdld::Datum::createInteger(3);
        d_map["i_42"]    = bdld::Datum::createInteger(42);
        d_map["i64_42"]  = bdld::Datum::createInteger64(42, allocator);
        d_map["s_foo"]   = bdld::Datum::createStringRef("foo", allocator);
        d_map["exists"]  = bdld::Datum::createInteger(42);
    }
    // Destroy this object.

    // MANIPULATORS

    /// Return a `bdld::Datum` object with value for the specified `name`.
    /// The `bslma::Allocator*` argument is unused.
    bdld::Datum get(const bsl::string& name,
                    bslma::Allocator*) BSLS_KEYWORD_OVERRIDE
    {
        bsl::unordered_map<bsl::string, bdld::Datum>::const_iterator iter =
            d_map.find(name);

        if (iter == d_map.end()) {
            return bdld::Datum::createError(-1);  // RETURN
        }

        return iter->second;
    }
};

#ifdef BSLS_PLATFORM_OS_LINUX
static void testN1_SimpleEvaluator_GoogleBenchmark(benchmark::State& state)
{
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK: SimpleEvaluator");

    bdlma::LocalSequentialAllocator<2048> localAllocator;
    MockPropertiesReader                  reader(&localAllocator);
    EvaluationContext evaluationContext(&reader, &localAllocator);

    CompilationContext compilationContext(&localAllocator);
    SimpleEvaluator    evaluator;

    BMQTST_ASSERT(evaluator.compile("false || (i64_42==42 && s_foo==\"foo\")",
                                    compilationContext) == 0);

    BMQTST_ASSERT_EQ(evaluator.evaluate(evaluationContext), true);

    // <time>
    for (auto _ : state) {
        evaluator.evaluate(evaluationContext);
    }
    // </time>
}
#else
static void testN1_SimpleEvaluator()
{
    bmqtst::TestHelper::printTestName("GOOGLE BENCHMARK: SimpleEvaluator");
    PV("GoogleBenchmark is not supported on this platform, skipping...")
}
#endif

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static bsl::string makeTooManyOperators()
{
    bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());

    for (size_t i = 0; i < SimpleEvaluator::k_MAX_OPERATORS + 1; ++i) {
        os << "!";
    }

    os << "x";

    return os.str();
}

static bsl::string makeTooLongExpression()
{
    bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());

    // Note that we want to create `k_STACK_SIZE` nested NOT objects in AST,
    // and when we call destructor chain for all these objects, we'll need
    // much more memory on a stack, since each destructor address is not a
    // single byte, and also we need to call shared_ptr and Not destructors in
    // pairs. But the initial guess of `k_STACK_SIZE` is more than sufficient
    // to cause segfault if this case is not handled properly.
    const size_t k_STACK_SIZE = 1024 * 1024;
    BMQTST_ASSERT(SimpleEvaluator::k_MAX_EXPRESSION_LENGTH < k_STACK_SIZE);

    for (size_t i = 0; i < k_STACK_SIZE; i += 16) {
        // Combining `!` and `~` differently in case we want to introduce
        // "NOT" optimization one day (remove double sequential "NOTs")
        os << "!!!!~~~~!~!~!!~~";
    }

    os << "x";

    return os.str();
}

static void test1_compilationErrors()
{
    struct TestParameters {
        bsl::string expression;
        int         errorCode;
        const char* errorMessage;
    } testParameters[] = {
        {"",
         ErrorType::e_SYNTAX,
         "syntax error, unexpected end of expression at offset 0"},
        {"42 @",
         ErrorType::e_SYNTAX,
         "syntax error, unexpected invalid character at offset 3"},
        {makeTooManyOperators(),
         ErrorType::e_TOO_COMPLEX,
         "too many operators (12), max allowed operators: 10"},
        {"true && true",
         ErrorType::e_NO_PROPERTIES,
         "expression does not use any properties"},
        {makeTooLongExpression(),
         ErrorType::e_TOO_LONG,
         "expression is too long (1048577), max allowed length: 128"},

        // only C-style operator variants supported
        {"val = 42",
         ErrorType::e_SYNTAX,
         "syntax error, unexpected invalid character at offset 4"},
        {"val | true",
         ErrorType::e_SYNTAX,
         "syntax error, unexpected invalid character at offset 4"},
        {"val & true",
         ErrorType::e_SYNTAX,
         "syntax error, unexpected invalid character at offset 4"},
        {"val <> 42",
         ErrorType::e_SYNTAX,
         "syntax error, unexpected > at offset 5"},

        // unsupported_ints
        {"i_0 != 9223372036854775808",
         ErrorType::e_SYNTAX,
         "integer overflow at offset 7"},  // 2 ** 63
        {"i_0 != -170141183460469231731687303715884105728",
         ErrorType::e_SYNTAX,
         "integer overflow at offset 7"},  // -(2 ** 127)
        {"i_0 != 170141183460469231731687303715884105727",
         ErrorType::e_SYNTAX,
         "integer overflow at offset 7"},  // 2 ** 127 - 1
        {"i_0 != "
         "-5789604461865809771178549250434395392663499233282028201972879200395"
         "6564819968",
         ErrorType::e_SYNTAX,
         "integer overflow at offset 7"},  // -(2 ** 255)
        {"i_0 != "
         "57896044618658097711785492504343953926634992332820282019728792003956"
         "564819967",
         ErrorType::e_SYNTAX,
         "integer overflow at offset 7"},  // 2 ** 255 - 1
    };
    const TestParameters* testParametersEnd = testParameters +
                                              sizeof(testParameters) /
                                                  sizeof(*testParameters);

    for (const TestParameters* parameters = testParameters;
         parameters < testParametersEnd;
         ++parameters) {
        PV(bsl::string("TESTING ") + parameters->expression);

        {
            CompilationContext compilationContext(
                bmqtst::TestHelperUtil::allocator());
            SimpleEvaluator    evaluator;

            evaluator.compile(parameters->expression, compilationContext);
            PV(compilationContext.lastErrorMessage());
            BMQTST_ASSERT(!evaluator.isValid());
            BMQTST_ASSERT_EQ(compilationContext.lastErrorMessage(),
                             parameters->errorMessage);
        }
    }
}

static void test2_propertyNames()
{
    struct TestParameters {
        bsl::string expression;
        bool        valid;
    } testParameters[] = {
        // letters
        {"name > 0", true},
        {"NAME > 0", true},
        {"Name > 0", true},
        {"nameName > 0", true},
        {"NameName > 0", true},
        {"n > 0", true},
        {"N > 0", true},
        {"aBaBaBaBaBaBaBaBaBaBaBaBaBaBaB > 0", true},
        {"aaaBBBaaaBBBaaaBBBaaaBBBaaaBBB > 0", true},
        {"BaBaBaBaBaBaBaBaBaBaBaBaBaBaBa > 0", true},

        // letters + digits
        {"n1a2m3e4 > 0", true},
        {"N1A2M3E4 > 0", true},
        {"N1a2m3e4 > 0", true},
        {"n1a2m3e4N5a6m7e8 > 0", true},
        {"N1a2m3e4N5a6m7e8 > 0", true},
        {"n0 > 0", true},
        {"N0 > 0", true},
        {"aB1aB2aB3aB4aB5aB6aB7aB8aB9aB0aB > 0", true},
        {"aaa111BBBaaa222BBBaaa333BBBaaa444 > 0", true},
        {"Ba1Ba2Ba3Ba4Ba5Ba6Ba7Ba8Ba9Ba0Ba > 0", true},

        // letters + underscore
        {"name_ > 0", true},
        {"NA_ME > 0", true},
        {"Na_me_ > 0", true},
        {"name_Name > 0", true},
        {"Name_Name_ > 0", true},
        {"n_ > 0", true},
        {"N_ > 0", true},
        {"aB_aB__aB___aB____aB_____ > 0", true},
        {"aaa_BBBaaa__BBBaaa___BBBaaa > 0", true},
        {"B_a_B_a_B_a_B_a_B_a_B_a_B_a_ > 0", true},

        // letters + digits + underscore
        {"n1a2m3e4_ > 0", true},
        {"N1A2_M3E4 > 0", true},
        {"N1a2_m3e4_ > 0", true},
        {"n1a2m3e4_N5a6m7e8 > 0", true},
        {"N1a2m3e4_N5a6m7e8_ > 0", true},
        {"n0_ > 0", true},
        {"N_0 > 0", true},
        {"aB1_aB__2a___B3aB4____ > 0", true},
        {"aaa_111BBBaaa222__BBBaaa3_3_3BBB > 0", true},
        {"B_a_1_B_a_2_B_a_3_B_a_4_B_a_5_B_ > 0", true},

        // letters + dot
        {"name. > 0", true},
        {"NA.ME > 0", true},
        {"Na.me. > 0", true},
        {"name.Name > 0", true},
        {"Name.Name. > 0", true},
        {"n. > 0", true},
        {"N. > 0", true},
        {"aB.aB..aB...aB....aB..... > 0", true},
        {"aaa.BBBaaa..BBBaaa...BBBaaa > 0", true},
        {"B.a.B.a.B.a.B.a.B.a.B.a.B.a. > 0", true},

        // letters + digits + dots
        {"n1a2m3e4. > 0", true},
        {"N1A2.M3E4 > 0", true},
        {"N1a2.m3e4. > 0", true},
        {"n1a2m3e4.N5a6m7e8 > 0", true},
        {"N1a2m3e4.N5a6m7e8. > 0", true},
        {"n0. > 0", true},
        {"N.0 > 0", true},
        {"aB1.aB..2a...B3aB4.... > 0", true},
        {"aaa.111BBBaaa222..BBBaaa3.3.3BBB > 0", true},
        {"B.a.1.B.a.2.B.a.3.B.a.4.B.a.5.B. > 0", true},

        // letters + digits + dots + underscores
        {"n1a2m3e4._ > 0", true},
        {"N1A2.M3E4_ > 0", true},
        {"N1a2_m3e4. > 0", true},
        {"n1a2m3e4_N5a6m7e8. > 0", true},
        {"N1a2m3e4.N5a6m7e8_ > 0", true},
        {"n0_. > 0", true},
        {"N_0. > 0", true},
        {"aB1_aB._2a__.B3aB4..._ > 0", true},
        {"aaa.111BBBaaa222__BBBaaa3.3_3BBB > 0", true},
        {"B.a_1_B.a_2.B_a.3.B.a_4.B_a.5.B. > 0", true},

        // readable examples
        {"camelCase > 0", true},
        {"snake_case > 0", true},
        {"PascalCase > 0", true},
        {"price > 0", true},
        {"BetterLateThanNever > 0", true},
        {"firmId > 0", true},
        {"TheStandardAndPoor500 > 0", true},
        {"SPX_IND > 0", true},
        {"organization.repository > 0", true},

        // all available characters
        {"abcdefghijklmnopqrstuvwxyz_.0123456789 > 0", true},
        {"ABCDEFGHIJKLMNOPQRSTUVWXYZ_.0123456789 > 0", true},

        // negative examples
        {"0name > 0", false},
        {"1NAME > 0", false},
        {"23456789Name > 0", false},
        {"_nameName > 0", false},
        {"0_NameName > 0", false},
        {"_1n > 0", false},
        {"1_N > 0", false},
        {"_ > 0", false},
        {"_11111111111111111111111111111 > 0", false},
        {"22222222222222222222222222222_ > 0", false},
        {".nameName > 0", false},
        {"0.NameName > 0", false},
        {".1n > 0", false},
        {"1.N > 0", false},
        {". > 0", false},
        {".11111111111111111111111111111 > 0", false},
        {"22222222222222222222222222222. > 0", false},
    };
    const TestParameters* testParametersEnd = testParameters +
                                              sizeof(testParameters) /
                                                  sizeof(*testParameters);
    CompilationContext compilationContext(bmqtst::TestHelperUtil::allocator());

    for (const TestParameters* parameters = testParameters;
         parameters < testParametersEnd;
         ++parameters) {
        PV(bsl::string("TESTING ") + parameters->expression);
        {
            bool valid = SimpleEvaluator::validate(parameters->expression,
                                                   compilationContext);
            BMQTST_ASSERT_EQ(valid, parameters->valid);
        }
    }
}

static void test3_evaluation()
{
    MockPropertiesReader reader(bmqtst::TestHelperUtil::allocator());
    EvaluationContext    evaluationContext(&reader,
                                        bmqtst::TestHelperUtil::allocator());

    const bool runtimeErrorResult = false;

    struct TestParameters {
        const char* expression;
        bool        expected;
    } testParameters[] = {
        {"b_true", true},

        // integer comparisons
        {"i64_42 == 42", true},
        {"i64_42 != 42", false},

        {"i64_42 < 41", false},
        {"i64_42 < 42", false},
        {"i64_42 < 43", true},

        {"i64_42 > 41", true},
        {"i64_42 > 42", false},
        {"i64_42 > 43", false},

        {"i64_42 <= 41", false},
        {"i64_42 <= 42", true},
        {"i64_42 <= 43", true},

        {"i64_42 >= 41", true},
        {"i64_42 >= 42", true},
        {"i64_42 >= 43", false},

        // reversed
        {"42 == i64_42", true},
        {"i64_42 != 42", false},

        {"41 < i64_42", true},
        {"42 < i64_42", false},
        {"43 < i64_42", false},

        {"41 > i64_42", false},
        {"42 > i64_42", false},
        {"43 > i64_42", true},

        {"41 <= i64_42", true},
        {"42 <= i64_42", true},
        {"43 <= i64_42", false},

        {"41 >= i64_42", false},
        {"42 >= i64_42", true},
        {"43 >= i64_42", true},

        // mixed integer types
        {"i_42 == 42", true},

        // string comparisons
        {"s_foo == \"foo\"", true},
        {"s_foo != \"foo\"", false},

        {"s_foo < \"bar\"", false},
        {"s_foo < \"foo\"", false},
        {"s_foo < \"zig\"", true},

        {"s_foo > \"bar\"", true},
        {"s_foo > \"foo\"", false},
        {"s_foo > \"zig\"", false},

        {"s_foo <= \"bar\"", false},
        {"s_foo <= \"foo\"", true},
        {"s_foo <= \"zig\"", true},

        {"s_foo >= \"bar\"", true},
        {"s_foo >= \"foo\"", true},
        {"s_foo >= \"zig\"", false},

        // string comparisons, reversed
        {"\"foo\" == s_foo", true},
        {"s_foo != \"foo\"", false},

        {"\"bar\" < s_foo", true},
        {"\"foo\" < s_foo", false},
        {"\"zig\" < s_foo", false},

        {"\"bar\" > s_foo", false},
        {"\"foo\" > s_foo", false},
        {"\"zig\" > s_foo", true},

        {"\"bar\" <= s_foo", true},
        {"\"foo\" <= s_foo", true},
        {"\"zig\" <= s_foo", false},

        {"\"bar\" >= s_foo", false},
        {"\"foo\" >= s_foo", true},
        {"\"zig\" >= s_foo", true},

        // logical operators
        {"!b_true", false},
        {"~b_true", false},
        {"b_true && i64_42 == 42", true},
        {"b_true || i64_42 != 42", true},
        // precedence
        {"!(i64_42 == 42)", false},
        {"b_false && b_false || b_true", true},
        {"b_false && (b_false || b_true)", false},

        // type mismatch yields 'runtimeErrorResult' (see above)
        {"s_foo == 42", runtimeErrorResult},
        {"s_foo == 42 && b_false", runtimeErrorResult},
        {"b_false || s_foo == 42", runtimeErrorResult},
        {"!(s_foo == 42)", runtimeErrorResult},

        // unknown property yields 'runtimeErrorResult' (see above)
        {"non_existing_property", runtimeErrorResult},

        // arithmetic operators
        {"i_1 + 2 == 3", true},
        {"i_1 - 2 == -1", true},
        {"i_2 * 3 == 6", true},
        {"i_42 / 2 == 21", true},
        {"i_42 % 10 == 2", true},
        {"-i_42 == -42", true},
        // precedence
        {"i_2 * 20 + 2 == 42", true},
        {"i_2 + 20 * 2 == 42", true},
        {"-i_1 + 1 == 0", true},

        // boolean literals
        // test literals on both sides; throw in a property to fool validation
        {"true || b_true", true},
        {"(true && true) || b_true", true},
        {"(true == true) || b_true", true},
        {"(true == false) || !b_true", false},
        {"false || !b_true", false},
        {"(false && true) || !b_true", false},

        {"b_true && true", true},
        {"true && b_true", true},
        {"b_true == true", true},
        {"b_true < true", false},
        // overflows
        // supported_ints
        {"i_0 != -32768", true},                // -(2 ** 15)
        {"i_0 != 32767", true},                 // 2 ** 15 - 1
        {"i_0 != -2147483648", true},           // -(2 ** 31)
        {"i_0 != 2147483647", true},            // 2 ** 31 - 1
        {"i_0 != -9223372036854775807", true},  // -(2 ** 63) + 1
        {"i_0 != 9223372036854775807", true},   // 2 ** 63 - 1
        {"i_0 != -9223372036854775808", true},  // -(2 ** 63)

        // exists
        // Note that we allow to set a message property with name `exists`.
        // In this case the usage context defines whether it's a function call
        // or a property value.
        {"exists(i_42)", true},
        {"exists(non_existing_property)", false},
        {"!exists(non_existing_property) || non_existing_property > 41", true},
        {"exists(i_42) && i_42 > 41", true},
        {"exists(non_existing_property) && non_existing_property > 41", false},
        {"exists == 42", true},
        {"exists(exists)", true},
    };
    const TestParameters* testParametersEnd = testParameters +
                                              sizeof(testParameters) /
                                                  sizeof(*testParameters);

    for (const TestParameters* parameters = testParameters;
         parameters < testParametersEnd;
         ++parameters) {
        PV(bsl::string("TESTING ") + parameters->expression);

        CompilationContext compilationContext(
            bmqtst::TestHelperUtil::allocator());
        SimpleEvaluator    evaluator;

        BMQTST_ASSERT(!evaluator.isValid());

        if (evaluator.compile(parameters->expression, compilationContext)) {
            PV(bsl::string("UNEXPECTED: ") +
               compilationContext.lastErrorMessage());
            BMQTST_ASSERT(false);
        }
        else {
            BMQTST_ASSERT(evaluator.isValid());
            BMQTST_ASSERT_EQ(evaluator.evaluate(evaluationContext),
                             parameters->expected);
        }
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 3: test3_evaluation(); break;
    case 2: test2_propertyNames(); break;
    case 1: test1_compilationErrors(); break;
    case -1: BMQTST_BENCHMARK(testN1_SimpleEvaluator); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

#ifdef BSLS_PLATFORM_OS_LINUX
    if (_testCase < 0) {
        benchmark::Initialize(&argc, argv);
        benchmark::RunSpecifiedBenchmarks();
    }
#endif

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
