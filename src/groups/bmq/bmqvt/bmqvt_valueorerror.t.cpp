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

// bmqvt_valueorerror.t.cpp -*-C++-*-
#include <bmqvt_valueorerror.h>

// BMQ
#include <bmqu_memoutstream.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>
#include <bsl_cstdlib.h>
#include <bslma_constructionutil.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

//=============================================================================
//                          GLOBAL TYPES FOR TESTING
//-----------------------------------------------------------------------------

namespace {

// A long string to trigger allocation.
const char* k_LONG_STRING = "12345678901234567890123456789001234567890";

class CustomValueType {
  public:
    CustomValueType()
    : d_foo(k_LONG_STRING, bmqtst::TestHelperUtil::allocator())
    , d_bar(k_LONG_STRING, bmqtst::TestHelperUtil::allocator())
    {
        PVV("CustomValueType(bslma::Allocator *)");
    }

    CustomValueType(const bsl::string& src)
    : d_foo(k_LONG_STRING, bmqtst::TestHelperUtil::allocator())
    , d_bar(k_LONG_STRING, bmqtst::TestHelperUtil::allocator())
    {
        PVV("CustomValueType(const bsl::string&)");

        d_foo += src;
        d_bar += src;
    }

    CustomValueType(const CustomValueType& rhs)
    : d_foo(rhs.d_foo, bmqtst::TestHelperUtil::allocator())
    , d_bar(rhs.d_bar, bmqtst::TestHelperUtil::allocator())
    {
        PVV("CustomValueType(const CustomValueType& , bslma::Allocator *)");
    }

    bsl::string d_foo;
    bsl::string d_bar;
};

bsl::ostream& operator<<(bsl::ostream& os, const CustomValueType& value)
{
    os << "(" << value.d_foo << ", " << value.d_bar << ")";
    return os;
}

bool operator==(const CustomValueType& lhs, const CustomValueType& rhs)
{
    return (lhs.d_foo == rhs.d_foo) && (lhs.d_bar == rhs.d_bar);
}

class CustomAllocValueType {
  public:
    CustomAllocValueType(bslma::Allocator* basicAllocator)
    : d_foo(k_LONG_STRING, basicAllocator)
    , d_bar(k_LONG_STRING, basicAllocator)
    {
    }

    CustomAllocValueType(const bsl::string& src,
                         bslma::Allocator*  basicAllocator)
    : d_foo(k_LONG_STRING, basicAllocator)
    , d_bar(k_LONG_STRING, basicAllocator)
    {
        d_foo += src;
        d_bar += src;
    }

    CustomAllocValueType(const CustomAllocValueType& rhs,
                         bslma::Allocator*           basicAllocator)
    : d_foo(rhs.d_foo, basicAllocator)
    , d_bar(rhs.d_bar, basicAllocator)
    {
    }

    BSLMF_NESTED_TRAIT_DECLARATION(CustomAllocValueType,
                                   bslma::UsesBslmaAllocator)

    bsl::string d_foo;
    bsl::string d_bar;
};

bsl::ostream& operator<<(bsl::ostream& os, const CustomAllocValueType& value)
{
    os << "(" << value.d_foo << ", " << value.d_bar << ")";
    return os;
}

bool operator==(const CustomAllocValueType& lhs,
                const CustomAllocValueType& rhs)
{
    return (lhs.d_foo == rhs.d_foo) && (lhs.d_bar == rhs.d_bar);
}

bsl::string formatError(const int rc)
{
    bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
    os << "error: " << rc << "(" << k_LONG_STRING << ")";
    return bsl::string(os.str(), bmqtst::TestHelperUtil::allocator());
}

class CustomErrorType {
  public:
    CustomErrorType()
    : d_error(k_LONG_STRING, bmqtst::TestHelperUtil::allocator())
    , d_rc(-2)
    {
    }

    CustomErrorType(const int rc)
    : d_error(formatError(rc), bmqtst::TestHelperUtil::allocator())
    , d_rc(rc)
    {
    }

    CustomErrorType(const CustomErrorType& rhs)
    : d_error(rhs.d_error, bmqtst::TestHelperUtil::allocator())
    , d_rc(rhs.d_rc)
    {
    }

    bsl::string d_error;
    int         d_rc;
};

bsl::ostream& operator<<(bsl::ostream& os, const CustomErrorType& error)
{
    os << "rc = " << error.d_rc << ": " << error.d_error;
    return os;
}

bool operator==(const CustomErrorType& lhs, const CustomErrorType& rhs)
{
    return (lhs.d_error == rhs.d_error) && (lhs.d_rc == rhs.d_rc);
}

class CustomAllocErrorType {
  public:
    CustomAllocErrorType(bslma::Allocator* basicAllocator)
    : d_error(k_LONG_STRING, basicAllocator)
    , d_rc(-2)
    {
    }

    CustomAllocErrorType(const int rc, bslma::Allocator* basicAllocator)
    : d_error(formatError(rc), basicAllocator)
    , d_rc(rc)
    {
    }

    CustomAllocErrorType(const CustomAllocErrorType& rhs,
                         bslma::Allocator*           basicAllocator)
    : d_error(rhs.d_error, basicAllocator)
    , d_rc(rhs.d_rc)
    {
    }

    BSLMF_NESTED_TRAIT_DECLARATION(CustomAllocErrorType,
                                   bslma::UsesBslmaAllocator)

    bsl::string d_error;
    int         d_rc;
};

bsl::ostream& operator<<(bsl::ostream& os, const CustomAllocErrorType& error)
{
    os << "rc = " << error.d_rc << ": " << error.d_error;
    return os;
}

bool operator==(const CustomAllocErrorType& lhs,
                const CustomAllocErrorType& rhs)
{
    return (lhs.d_error == rhs.d_error) && (lhs.d_rc == rhs.d_rc);
}

template <typename T, typename V>
T makeObjectCase(const V&               value,
                 BSLS_ANNOTATION_UNUSED bsl::true_type uses_allocator)
{
    return T(value, bmqtst::TestHelperUtil::allocator());
}

template <typename T, typename V>
T makeObjectCase(const V&               value,
                 BSLS_ANNOTATION_UNUSED bsl::false_type uses_allocator)
{
    return T(value);
}

template <typename T, typename V>
T makeObject(const V& value)
{
    return makeObjectCase<T, V>(value,
                                typename bslma::UsesBslmaAllocator<T>::type());
}

template <typename T>
T makeObjectCase(BSLS_ANNOTATION_UNUSED bsl::true_type uses_allocator)
{
    return T(bmqtst::TestHelperUtil::allocator());
}

template <typename T>
T makeObjectCase(BSLS_ANNOTATION_UNUSED bsl::false_type uses_allocator)
{
    return T();
}

template <typename T>
T makeObject()
{
    return makeObjectCase<T>(typename bslma::UsesBslmaAllocator<T>::type());
}

int atoiNoThrowSpec(const char* str)
{
    return bsl::atoi(str);
}

inline unsigned toUnsigned(const double in)
{
    return static_cast<unsigned>(in);
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

template <typename T>
static void test_ops_generic()
{
    typedef typename T::ValueType Value;
    typedef typename T::ErrorType Error;

    PVV("Test ValueOrError(bslma::Allocator *) and operator<<");
    T value(bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT(value.isUndefined());
    BMQTST_ASSERT(!value.isValue());
    BMQTST_ASSERT(!value.isError());
    {
        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
        os << value;
        BMQTST_ASSERT_EQ(os.str(), "[ UNDEFINED ]");
    }

    PVV("Test makeValue(const VALUE&)");

    Value refValue(makeObject<Value>("value"));

    value.makeValue(refValue);
    BMQTST_ASSERT(!value.isUndefined());
    BMQTST_ASSERT(value.isValue());
    BMQTST_ASSERT(!value.isError());
    {
        bmqu::MemOutStream expected(bmqtst::TestHelperUtil::allocator()),
            actual(bmqtst::TestHelperUtil::allocator());

        expected << "[ value = " << refValue << " ]";
        actual << value;

        BMQTST_ASSERT_EQ(actual.str(), expected.str());
    }
    BMQTST_ASSERT_EQ(value.value(), refValue);

    PVV("Test makeError(const ERROR&)");

    Error refError(makeObject<Error>(-5));

    value.makeError(refError);
    BMQTST_ASSERT(!value.isUndefined());
    BMQTST_ASSERT(!value.isValue());
    BMQTST_ASSERT(value.isError());
    {
        bmqu::MemOutStream expected(bmqtst::TestHelperUtil::allocator()),
            actual(bmqtst::TestHelperUtil::allocator());

        expected << "[ error = " << refError << " ]";
        actual << value;

        BMQTST_ASSERT_EQ(actual.str(), expected.str());
    }
    BMQTST_ASSERT_EQ(value.error(), refError);

    PVV("Test makeValue()");
    value.makeValue();
    BMQTST_ASSERT_EQ(value.value(), makeObject<Value>());

    PVV("Test makeError()");
    value.makeError();
    BMQTST_ASSERT_EQ(value.error(), makeObject<Error>());

    PVV("Test reset()");
    BMQTST_ASSERT(!value.isUndefined());
    value.reset();
    BMQTST_ASSERT(value.isUndefined());

    PVV("Test operator=");

    Value srcValue(makeObject<Value>("src"));
    {
        T src(bmqtst::TestHelperUtil::allocator());
        src.makeValue(srcValue);
        value = src;
    }
    BMQTST_ASSERT(!value.isUndefined());
    BMQTST_ASSERT(value.isValue());
    BMQTST_ASSERT(!value.isError());
    BMQTST_ASSERT_EQ(value.value(), srcValue);

    PVV("Test copy constructor");
    T copy(value, bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(copy.value(), value.value());

    // Temporary workaround to suppress the 'unused operator
    // NestedTraitDeclaration' warning/error generated by clang.
    //
    // TBD: figure out the right way to fix this.

    CustomAllocValueType dummy1(bmqtst::TestHelperUtil::allocator());
    static_cast<void>(
        static_cast<bslmf::NestedTraitDeclaration<CustomAllocValueType,
                                                  bslma::UsesBslmaAllocator> >(
            dummy1));

    CustomAllocErrorType dummy2(bmqtst::TestHelperUtil::allocator());
    static_cast<void>(
        static_cast<bslmf::NestedTraitDeclaration<CustomAllocErrorType,
                                                  bslma::UsesBslmaAllocator> >(
            dummy2));
}

static void test1_ops_on_string_int()
// ------------------------------------------------------------------------
// Basic Operations on ValueOrError<string, int>
//
// Concerns:
//   - ensure that all the basic 'ValueOrError' operations function in
//     accordance with the usual semantics. Note that there's no need for
//     extreme testing, since this class was based on bas_codegen'ed code.
//
// Plan:
//   Execute all basic operations and check basic assertions.
//
// Testing:
//   Proper behavior of the 'ValueOrError' class.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ops_on_string_int");

    test_ops_generic<bmqvt::ValueOrError<bsl::string, int> >();
}

static void test2_ops_on_custom_value_error_types()
// ------------------------------------------------------------------------
// Basic Operations on ValueOrError<CustomValueType, CustomErrorType>
//
// Concerns:
//   - ensure that all the basic 'ValueOrError' operations function in
//     accordance with the usual semantics. Note that there's no need for
//     extreme testing, since this class was based on bas_codegen'ed code.
//
// Plan:
//   Execute all basic operations and check basic assertions.
//
// Testing:
//   Proper behavior of the 'ValueOrError' class.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ops_on_custom_value_error_types");

    test_ops_generic<bmqvt::ValueOrError<CustomValueType, CustomErrorType> >();
}

static void test3_ops_on_customalloc_value_error_types()
// ------------------------------------------------------------------------
// Basic Operations on ValueOrError<CustomValueType, CustomErrorType>
//
// Concerns:
//   - ensure that all the basic 'ValueOrError' operations function in
//     accordance with the usual semantics. Note that there's no need for
//     extreme testing, since this class was based on bas_codegen'ed code.
//
// Plan:
//   Execute all basic operations and check basic assertions.
//
// Testing:
//   Proper behavior of the 'ValueOrError' class.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ops_on_customalloc_value_error_types");

    test_ops_generic<
        bmqvt::ValueOrError<CustomAllocValueType, CustomErrorType> >();
}

static void test4_ops_on_custom_value_erroralloc_types()
// ------------------------------------------------------------------------
// Basic Operations on ValueOrError<CustomValueType, CustomErrorType>
//
// Concerns:
//   - ensure that all the basic 'ValueOrError' operations function in
//     accordance with the usual semantics. Note that there's no need for
//     extreme testing, since this class was based on bas_codegen'ed code.
//
// Plan:
//   Execute all basic operations and check basic assertions.
//
// Testing:
//   Proper behavior of the 'ValueOrError' class.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("ops_on_custom_value_erroralloc_types");

    test_ops_generic<
        bmqvt::ValueOrError<CustomValueType, CustomAllocErrorType> >();
}

static void test5_ops_on_customalloc_value_erroralloc_types()
// ------------------------------------------------------------------------
// Basic Operations on ValueOrError<CustomValueType, CustomErrorType>
//
// Concerns:
//   - ensure that all the basic 'ValueOrError' operations function in
//     accordance with the usual semantics. Note that there's no need for
//     extreme testing, since this class was based on bas_codegen'ed code.
//
// Plan:
//   Execute all basic operations and check basic assertions.
//
// Testing:
//   Proper behavior of the 'ValueOrError' class.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "ops_on_customalloc_value_erroralloc_types");

    test_ops_generic<
        bmqvt::ValueOrError<CustomAllocValueType, CustomAllocErrorType> >();
}

static void test6_converters()
// ------------------------------------------------------------------------
// Mapping Operations on ValueOrError<CustomValueType, CustomErrorType>
//
// Concerns:
//   - ensure that all mapping 'ValueOrError' operations function in
//     accordance with the usual semantics.
//
// Plan:
//   Execute all mapping operations and check basic assertions.
//
// Testing:
//   Proper behavior of the 'ValueOrError' class.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("converters");

    // Use 'mapValue()' to transform values (Value case)
    {
        typedef bmqvt::ValueOrError<const char*, char> SourceType;
        typedef bmqvt::ValueOrError<int, char>         TargetType;

        SourceType st;
        st.makeValue("12");
        TargetType tt;

        st.mapValue(&tt, &atoiNoThrowSpec);
        BMQTST_ASSERT(tt.isValue());
        BMQTST_ASSERT_EQ(tt.value(), 12);

        BMQTST_ASSERT_EQ(tt.valueOr(15), 12);
    }

    // Use 'mapValue()' to transform values (Error case)
    {
        typedef bmqvt::ValueOrError<const char*, char> SourceType;
        typedef bmqvt::ValueOrError<int, char>         TargetType;

        SourceType st;
        st.makeError('a');
        TargetType tt;

        st.mapValue(&tt, &atoiNoThrowSpec);
        BMQTST_ASSERT(tt.isError());
        BMQTST_ASSERT_EQ(tt.error(), 'a');

        BMQTST_ASSERT_EQ(tt.valueOr(15), 15);
    }

    // Same thing with 'mapError()' for errors (Value case)
    {
        typedef bmqvt::ValueOrError<char, const char*> SourceType;
        typedef bmqvt::ValueOrError<char, int>         TargetType;

        SourceType st;
        st.makeValue('a');
        TargetType tt;

        st.mapError(&tt, &atoiNoThrowSpec);
        BMQTST_ASSERT(tt.isValue());
        BMQTST_ASSERT_EQ(tt.value(), 'a');

        BMQTST_ASSERT_EQ(tt.valueOr('i'), 'a');
    }

    // Same thing with 'mapError()' for errors (Error case)
    {
        typedef bmqvt::ValueOrError<char, const char*> SourceType;
        typedef bmqvt::ValueOrError<char, int>         TargetType;

        SourceType st;
        st.makeError("12");
        TargetType tt;

        st.mapError(&tt, &atoiNoThrowSpec);
        BMQTST_ASSERT(tt.isError());
        BMQTST_ASSERT_EQ(tt.error(), 12);

        BMQTST_ASSERT_EQ(tt.valueOr('i'), 'i');
    }

    // Use map() to transform both.
    {
        typedef bmqvt::ValueOrError<const char*, double> SourceType;
        typedef bmqvt::ValueOrError<int, unsigned>       TargetType;

        SourceType st1, st2;

        st1.makeValue("12");
        st2.makeError(3.14);

        TargetType tt1, tt2;
        st1.map(&tt1, &atoiNoThrowSpec, &toUnsigned);
        st2.map(&tt2, &atoiNoThrowSpec, &toUnsigned);

        BMQTST_ASSERT(tt1.isValue());
        BMQTST_ASSERT_EQ(tt1.value(), 12);

        BMQTST_ASSERT(tt2.isError());
        BMQTST_ASSERT_EQ(tt2.error(), 3u);
    }
}

static void test7_explicit_print()
// ------------------------------------------------------------------------
// Explicit Printing on ValueOrError
//
// Concerns:
//   - ensure that explicit call of the 'static' 'print()' method of
//     'ValueOrError' produces the expected outcome.
//
// Plan:
//   Execute a 'print()' call and check results.
//
// Testing:
//   Proper behavior of the 'ValueOrError' class.
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("explicit_print");

    typedef bmqvt::ValueOrError<int, char> TypeUnderTest;

    TypeUnderTest value;
    value.makeValue(12);

    bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
    TypeUnderTest::print(os, value, 0, 0);
    BMQTST_ASSERT_EQ(os.str(), "[ value = 12 ]\n");
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 7: test7_explicit_print(); break;
    case 6: test6_converters(); break;
    case 5: test5_ops_on_customalloc_value_erroralloc_types(); break;
    case 4: test4_ops_on_custom_value_erroralloc_types(); break;
    case 3: test3_ops_on_customalloc_value_error_types(); break;
    case 2: test2_ops_on_custom_value_error_types(); break;
    case 1: test1_ops_on_string_int(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
