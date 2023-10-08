// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mwcst_testutil.cpp -*-C++-*-
#include <mwcst_testutil.h>

#include <bdlb_tokenizer.h>
#include <bdlbb_blobutil.h>
#include <bsl_iostream.h>
#include <mwcscm_version.h>
#include <stdlib.h>

namespace BloombergLP {
namespace mwcu {

// ---------------
// struct TestUtil
// ---------------

// CLASS METHODS
bsl::vector<int> TestUtil::intVector(const char* nums)
{
    bsl::vector<int> ret;
    bdlb::Tokenizer  tokenizer(nums, " ");
    for (bdlb::TokenizerIterator iter = tokenizer.begin();
         iter != tokenizer.end();
         ++iter) {
        if (*iter == "--") {
            ret.push_back(INT_MIN);
        }
        else if (*iter == "++") {
            ret.push_back(INT_MAX);
        }
        else {
            char buf[32] = {0};
            bsl::memcpy(buf, iter->begin(), iter->length());
            buf[iter->length()] = '\0';

            ret.push_back(bsl::atoi(buf));
        }
    }

    return ret;
}

bsl::vector<bsls::Types::Int64> TestUtil::int64Vector(const char* nums)
{
    bsl::vector<bsls::Types::Int64> ret;
    bdlb::Tokenizer                 tokenizer(nums, " ");
    for (bdlb::TokenizerIterator iter = tokenizer.begin();
         iter != tokenizer.end();
         ++iter) {
        if (*iter == "--") {
            ret.push_back(LLONG_MIN);
        }
        else if (*iter == "++") {
            ret.push_back(LLONG_MAX);
        }
        else {
            char buf[32] = {0};
            bsl::memcpy(buf, iter->begin(), iter->length());
            buf[iter->length()] = '\0';

            ret.push_back(bsl::atol(buf));
        }
    }

    return ret;
}

bsl::vector<bsl::string> TestUtil::stringVector(const char* s)
{
    bsl::vector<bsl::string> ret;
    bdlb::Tokenizer          tokenizer(s, " ");
    for (bdlb::TokenizerIterator iter = tokenizer.begin();
         iter != tokenizer.end();
         ++iter) {
        if (*iter == "-=") {
            ret.push_back("");
        }
        else {
            ret.push_back(*iter);
        }
    }

    return ret;
}

void TestUtil::hexToString(bsl::string* output, const bslstl::StringRef& input)
{
    static const char* HEX_DIGITS = "0123456789ABCDEF";

    BSLS_ASSERT_OPT(!(input.length() & 1) && "Odd length");
    output->reserve(input.length() / 2);

    for (size_t i = 0; i < input.length(); i += 2) {
        char        a = input[i];
        const char* p = bsl::lower_bound(HEX_DIGITS, HEX_DIGITS + 16, a);
        BSLS_ASSERT_OPT(*p == a && "Bad hex input");

        char        b = input[i + 1];
        const char* q = bsl::lower_bound(HEX_DIGITS, HEX_DIGITS + 16, b);
        BSLS_ASSERT_OPT(*q == b && "Bad hex input");

        output->push_back(static_cast<char>((p - HEX_DIGITS) << 4) | static_cast<char>(q - HEX_DIGITS));
    }
}

bool TestUtil::testRunningInJenkins()
{
    char* envVar    = getenv("JENKINS_URL");
    bool  isJenkins = (envVar != 0);
    if (isJenkins) {
        bsl::cerr << "WARNING: RUNNING IN JENKINS. "
                  << "WILL SKIP SOME TEST CASES IF THEY FAIL." << bsl::endl;
    }
    return isJenkins;
}

void TestUtil::printTestStatus(int testStatus, int verbose)
{
    if (testStatus == 254) {
        bsl::cerr << "Test skipped." << bsl::endl;
    }
    else if (testStatus > 0) {
        bsl::cerr << "Error, non-zero test status = " << testStatus << "."
                  << bsl::endl;
    }
    else if (verbose && testStatus == 0) {
        bsl::cout << "Test Passed!" << bsl::endl;
    }
}

// ----------------------
// class BlobDataComparer
// ----------------------

// CREATORS
BlobDataComparer::BlobDataComparer(const bdlbb::Blob* blob,
                                   bslma::Allocator*  allocator)
: d_blob_p(blob)
, d_offset(0)
, d_allocator_p(allocator)
{
}

BlobDataComparer::BlobDataComparer(const bdlbb::Blob*  blob,
                                   const BlobPosition& start,
                                   bslma::Allocator*   allocator)
: d_blob_p(blob)
, d_offset()
, d_allocator_p(allocator)
{
    BlobUtil::positionToOffset(&d_offset, *blob, start);
}

// ACCESSORS
const bdlbb::Blob* BlobDataComparer::blob() const
{
    return d_blob_p;
}

int BlobDataComparer::offset() const
{
    return d_offset;
}

bslma::Allocator* BlobDataComparer::allocator() const
{
    return d_allocator_p;
}

}  // close package namespace

// FREE FUNCTIONS
bool mwcu::operator==(const mwcu::BlobDataComparer& lhs,
                      const mwcu::BlobDataComparer& rhs)
{
    bdlbb::Blob lhsCopy(lhs.allocator());
    bdlbb::BlobUtil::append(&lhsCopy, *lhs.blob(), lhs.offset());

    bdlbb::Blob rhsCopy(rhs.allocator());
    bdlbb::BlobUtil::append(&rhsCopy, *rhs.blob(), rhs.offset());

    return bdlbb::BlobUtil::compare(lhsCopy, rhsCopy) == 0;
}

bsl::ostream& mwcu::operator<<(bsl::ostream&                 stream,
                               const mwcu::BlobDataComparer& val)
{
    int length = val.blob()->length() - val.offset();
    return stream << "\n"
                  << bdlbb::BlobUtilHexDumper(val.blob(),
                                              val.offset(),
                                              length);
}

}  // close enterprise namespace
