// Copyright 2014-2023 Bloomberg Finance L.P.
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

// bmqt_uri.cpp                                                       -*-C++-*-
#include <bmqt_uri.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_print.h>
#include <bdlb_stringrefutil.h>
#include <bdlma_localsequentialallocator.h>
#include <bdlpcre_regex.h>
#include <bsl_cstddef.h>
#include <bsl_cstring.h>  // for 'strncmp'
#include <bsl_iostream.h>
#include <bsl_ostream.h>
#include <bsl_vector.h>
#include <bslma_default.h>
#include <bslmt_once.h>
#include <bslmt_qlock.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_objectbuffer.h>

namespace BloombergLP {
namespace bmqt {

namespace {
const char k_SCHEME[]        = "bmq";
const char k_TIER_PREFIX[]   = ".~";
const char k_QUERY_ID[]      = "id";
const int  k_QUERY_ID_LENGTH = sizeof(k_QUERY_ID) - 1;

bsls::ObjectBuffer<bdlpcre::RegEx> s_regex;
// Regular expression to validate and
// extract components of the URI.  Use an
// object buffer so we can allocate in the
// 'initialize' method, and destroy in the
// 'shutdown' instead of relying on
// 'random' ordering for creation of static
// objects by the compiler.

bsls::Types::Int64 s_initialized = 0;
// Integer to keep track of the number of
// calls to 'initialize' for the
// 'UriParser'.  If the value is non-zero,
// then it has already been initialized,
// otherwise it can be initialized.  Each
// call to 'initialize' increments the
// value of this integer by one.  Each call
// to 'shutdown' decrements the value of
// this integer by one.  If the decremented
// value is zero, then the 'UriParser' is
// destroyed.

bslmt::QLock s_initLock = BSLMT_QLOCK_INITIALIZER;
// Lock used to provide thread-safe
// protection for accessing the
// 's_initialized' counter.
}  // close unnamed namespace

// ---------
// class Uri
// ---------

Uri::Uri(bslma::Allocator* allocator)
: d_uri(allocator)
, d_wasParserInitialized(false)
{
    // NOTHING
}

Uri::Uri(const Uri& original, bslma::Allocator* allocator)
: d_uri(allocator)
, d_wasParserInitialized(false)
{
    copyImpl(original);
}

Uri::Uri(const bslstl::StringRef& uri, bslma::Allocator* allocator)
: d_uri(allocator)
, d_wasParserInitialized(false)
{
    UriParser::initialize();
    d_wasParserInitialized = true;
    UriParser::parse(this, 0, uri);
}

Uri::Uri(const char* uri, bslma::Allocator* allocator)
: d_uri(allocator)
, d_wasParserInitialized(false)
{
    UriParser::initialize();
    d_wasParserInitialized = true;
    UriParser::parse(this, 0, uri);
}

Uri::Uri(const bsl::string& uri, bslma::Allocator* allocator)
: d_uri(allocator)
, d_wasParserInitialized(false)
{
    UriParser::initialize();
    d_wasParserInitialized = true;
    UriParser::parse(this, 0, uri);
}

Uri::~Uri()
{
    if (d_wasParserInitialized) {
        UriParser::shutdown();
    }
}

void Uri::reset()
{
    d_uri.clear();

    d_scheme.reset();
    d_authority.reset();
    d_domain.reset();
    d_tier.reset();
    d_path.reset();
    d_query_id.reset();
}

void Uri::copyImpl(const Uri& src)
{
    reset();

    d_uri = src.d_uri;

    d_scheme.assign(k_SCHEME);  // The scheme is 'special', constant

    // Adjust all stringRef: keep their offset in the src.d_uri, but adjust it
    // to the address of this.d_uri
#define BMQT_FIX_STRINGREF(FIELD)                                             \
    if (!src.FIELD.isEmpty()) {                                               \
        FIELD.assign(d_uri.data() + (src.FIELD.data() - src.d_uri.data()),    \
                     src.FIELD.length());                                     \
    }

    BMQT_FIX_STRINGREF(d_authority)
    BMQT_FIX_STRINGREF(d_domain)
    BMQT_FIX_STRINGREF(d_tier)
    BMQT_FIX_STRINGREF(d_path)
    BMQT_FIX_STRINGREF(d_query_id)

#undef BMQT_FIX_STRINGREF
}

bsl::ostream&
Uri::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << d_uri;

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

// ----------------
// struct UriParser
// ----------------

void UriParser::initialize(bslma::Allocator* allocator)
{
    bslmt::QLockGuard qlockGuard(&s_initLock);

    // NOTE: We pre-increment here instead of post-incrementing inside the
    //       conditional check below because the post-increment of an int does
    //       not work correctly with versions of IBM xlc12 released following
    //       the 'Dec 2015 PTF'.
    BSLS_ASSERT(s_initialized <
                bsl::numeric_limits<bsls::Types::Int64>::max());
    ++s_initialized;
    if (s_initialized > 1) {
        return;  // RETURN
    }

    const char k_PATTERN[] = "^bmq:\\/\\/"
                             "(?P<authority>"
                             "(?P<domain>[-a-zA-Z0-9\\._]*)"
                             "(?P<tier>\\.~[-a-zA-Z0-9]*)?"
                             ")/"
                             "(?P<path>[-a-zA-Z0-9_\\.]*)"
                             "(?P<q1>\\?(id=)[-a-zA-Z0-9_\\.]+)?$";
    // NOTE: '~' in the authority is a reserved character, used by domain
    //       resolver to insert the optional resolved tier (if the last segment
    //       of a 'domain' starts by '~', this means it represents the tier and
    //       was automatically added by the domain resolver).
    // NOTE: we use '*' and not '+' for the authority and path parts so that
    //       when parsing the URI, we can return more detailed error
    //       (i.e. empty 'authority', instead of a generic regular expression
    //       mismatch).

    bsl::string error;
    size_t      errorOffset;

    new (s_regex.buffer())
        bdlpcre::RegEx(bslma::Default::globalAllocator(allocator));
    // Enable JIT compilation, unless running under MemorySanitizer.
    // Low-level assembler instructions used by sljit causes sanitizer issues.
    int regexOptions = bdlpcre::RegEx::k_FLAG_JIT;
#if defined(__has_feature)
#if __has_feature(memory_sanitizer)
    regexOptions &= ~bdlpcre::RegEx::k_FLAG_JIT;
#endif
#endif
    int rc = s_regex.object().prepare(&error,
                                      &errorOffset,
                                      k_PATTERN,
                                      regexOptions);
    if (rc != 0) {
        BALL_LOG_ERROR << "#URI_REGEXP "
                       << "Failed to compile URI regular expression [error: '"
                       << error << "', offset: " << errorOffset << "]";
    }
    BSLS_ASSERT_OPT(rc == 0 && "Failed to compile URI regular expression");
}

void UriParser::shutdown()
{
    bslmt::QLockGuard qlockGuard(&s_initLock);

    BSLS_ASSERT(s_initialized > 0);
    if (--s_initialized != 0) {
        return;  // RETURN
    }

    s_regex.object().~RegEx();
}

int UriParser::parse(Uri*                     result,
                     bsl::string*             errorDescription,
                     const bslstl::StringRef& uriString)
{
    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS        = 0,
        rc_INVALID_FORMAT = -1,
        rc_BAD_QUERY      = -2,
        rc_MISSING_DOMAIN = -3,
        rc_MISSING_QUEUE  = -4,
        rc_MISSING_TIER   = -5
    };

    enum {
        // Enum representing the index of the matching parts of the URI regexp
        k_AUTHORITY_IDX = 1,
        k_DOMAIN_IDX,
        k_TIER_IDX,
        k_PATH_IDX,
        k_Q1_IDX
    };

    BSLS_ASSERT_SAFE(s_regex.object().isPrepared() &&
                     "'initialize' was not called");

    bdlma::LocalSequentialAllocator<2048> localAllocator(
        bslma::Default::allocator());

    result->reset();
    result->d_uri.assign(uriString.data(), uriString.length());

    // RegExp matching
    // ---------------
    bsl::vector<bsl::pair<size_t, size_t> > matches(&localAllocator);

    int rc = s_regex.object().match(&matches,
                                    result->d_uri.data(),
                                    result->d_uri.length());
    if (rc != 0) {
        if (errorDescription) {
            bmqu::MemOutStream os(&localAllocator);
            os << "invalid format (" << rc << ")";
            errorDescription->assign(os.str().data(), os.str().length());
        }
        result->reset();
        return rc_INVALID_FORMAT;  // RETURN
    }

    result->d_scheme.assign(k_SCHEME);

    // Populate from captured groups
    // -----------------------------
    typedef bsl::pair<size_t, size_t> RegExpCapture;

    // Authority
    const RegExpCapture& capturedAuthority = matches[k_AUTHORITY_IDX];
    result->d_authority.assign(result->d_uri.data() + capturedAuthority.first,
                               capturedAuthority.second);

    // Domain
    const RegExpCapture& capturedDomain = matches[k_DOMAIN_IDX];
    result->d_domain.assign(result->d_uri.data() + capturedDomain.first,
                            capturedDomain.second);

    // Tier
    const RegExpCapture& capturedTier = matches[k_TIER_IDX];
    if (capturedTier.second != 0) {
        // Dropping the .~ prefix from the tier
        result->d_tier.assign(result->d_uri.data() + capturedTier.first + 2,
                              capturedTier.second - 2);
    }

    // Path
    const RegExpCapture& capturedPath = matches[k_PATH_IDX];
    result->d_path.assign(result->d_uri.data() + capturedPath.first,
                          capturedPath.second);

    // Query
    const RegExpCapture& query1 = matches[k_Q1_IDX];
    if (query1.second > 0) {
        // Query1 is present
        bslstl::StringRef q1(result->d_uri.data() + query1.first + 1,
                             query1.second - 1);  // + -1 to skip '?'

        if (0 == bsl::strncmp(q1.data(), k_QUERY_ID, k_QUERY_ID_LENGTH)) {
            result->d_query_id.assign(q1.data() + k_QUERY_ID_LENGTH + 1,
                                      q1.length() - k_QUERY_ID_LENGTH - 1);
            // +1 -1 to skip '='
        }
        else {
            if (errorDescription) {
                *errorDescription = "bad query";
            }
            result->reset();
            return rc_BAD_QUERY;  // RETURN
        }
    }

    // Validate mandatory fields
    // -------------------------
    if (result->d_authority.isEmpty()) {
        if (errorDescription) {
            *errorDescription = "missing domain";
        }
        result->reset();
        return rc_MISSING_DOMAIN;  // RETURN
    }

    if (capturedTier.second > 0 && result->d_tier.isEmpty()) {
        if (errorDescription) {
            *errorDescription = "missing tier";
        }
        result->reset();
        return rc_MISSING_TIER;  // RETURN
    }

    if (result->d_path.isEmpty()) {
        if (errorDescription) {
            *errorDescription = "missing queue";
        }
        result->reset();
        return rc_MISSING_QUEUE;  // RETURN
    }

    if (result->d_path.length() > Uri::k_QUEUENAME_MAX_LENGTH) {
        // TBD: Convert to a real error once certified all active queues are
        //      within the limit.  When converting to an error, add test cases
        //      in the test driver.
        BSLMT_ONCE_DO
        {
            bmqu::MemOutStream os;
            os << "The queue name part of '" << uriString << "' is exceeding "
               << "the maximum size limit of " << Uri::k_QUEUENAME_MAX_LENGTH
               << ".  This is only a warning at the moment, but this limit "
               << "will soon be enforced and  queue URI will be rejected!";
            bsl::cerr << "BMQALARM [INVALID_QUEUE_NAME]: " << os.str() << '\n'
                      << bsl::flush;
            BALL_LOG_ERROR << os.str();
        }
    }

    // Success
    return rc_SUCCESS;
}

// ----------------
// class UriBuilder
// ----------------

UriBuilder::UriBuilder(bslma::Allocator* allocator)
: d_uri(allocator)
{
    reset();
}

UriBuilder::UriBuilder(const bmqt::Uri& uri, bslma::Allocator* allocator)
: d_uri(uri, allocator)
{
    // NOTHING
}

UriBuilder& UriBuilder::setQualifiedDomain(const bslstl::StringRef& value)
{
    bslstl::StringRef tier = bdlb::StringRefUtil::strrstr(value,
                                                          k_TIER_PREFIX);

    if (tier.length() > 0) {
        d_uri.d_domain = bslstl::StringRef(value.begin(), tier.begin());
        d_uri.d_tier   = bslstl::StringRef(tier.end(), value.end());
    }
    else {
        d_uri.d_domain = value;
        d_uri.d_tier.reset();
    }

    return *this;
}

void UriBuilder::reset()
{
    d_uri.reset();

    // Reset scheme to the default value
    d_uri.d_scheme = k_SCHEME;
}

int UriBuilder::uri(Uri* result, bsl::string* errorDescription) const
{
    bdlma::LocalSequentialAllocator<1024> localAllocator(
        bslma::Default::allocator());

    // Build the string
    bmqu::MemOutStream os(&localAllocator);
    os << d_uri.d_scheme << "://" << d_uri.d_domain;
    if (!d_uri.d_tier.empty()) {
        os << k_TIER_PREFIX << d_uri.d_tier;
    }
    os << "/" << d_uri.d_path;
    if (!d_uri.d_query_id.isEmpty()) {
        os << "?" << k_QUERY_ID << "=" << d_uri.d_query_id;
    }

    // Parse and populate the result
    return UriParser::parse(result, errorDescription, os.str());
}

}  // close package namespace
}  // close enterprise namespace
