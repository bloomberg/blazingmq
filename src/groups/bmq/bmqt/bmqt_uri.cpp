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
#include <bsl_cstddef.h>
#include <bsl_cstring.h>  // for 'strncmp'
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_ostream.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bsla_annotations.h>
#include <bslma_default.h>
#include <bslmt_once.h>
#include <bslmt_qlock.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_objectbuffer.h>

namespace BloombergLP {
namespace bmqt {

namespace {
const bsl::string_view k_SCHEME           = "bmq";
const bsl::string_view k_SCHEME_SEPARATED = "bmq://";
const bsl::string_view k_QUERY_ID         = "id";
const bsl::string_view k_TIER_PREFIX      = ".~";

#define BMQT_RETURN_WITH_ERROR(RC, ERROR_VAR, ERROR_DESC)                     \
    if (ERROR_VAR) {                                                          \
        bdlma::LocalSequentialAllocator<256> local;                           \
        bmqu::MemOutStream                   os(&local);                      \
        os << ERROR_DESC;                                                     \
        (ERROR_VAR)->assign(os.str().data(), os.str().length());              \
    }                                                                         \
    return RC;

#define BMQT_VALIDATE_AND_RETURN(LENGTH, ERROR_RC, ERROR_VAR, ERROR_DESC)     \
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(0 == (LENGTH))) {               \
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;                                   \
        BMQT_RETURN_WITH_ERROR(ERROR_RC, ERROR_VAR, ERROR_DESC);              \
    }                                                                         \
    return UriParser::UriParseResult::e_SUCCESS;

struct UriParsingContext {
    const bslstl::StringRef d_uri;
    bool                    d_hasTier;
    bool                    d_hasQuery;

    struct QueryType {
        enum Enum { e_ID = 0 };
    };

    /// @brief Check if the provided char is alphanumeric.
    /// @param c input char.
    /// @return true if the provided char is alphamumeric, false otherwiese.
    /// NOTE: this is a fast inline equivalent of `bsl::isalnum`.
    static inline bool isalnum_fast(char c)
    {
        return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
               ('0' <= c && c <= '9');
    }

    explicit UriParsingContext(bslstl::StringRef uriString)
    : d_uri(uriString)
    , d_hasTier(false)
    , d_hasQuery(false)
    {
        // NOTHING
    }

    inline bool hasTier() const { return d_hasTier; }

    inline bool hasQuery() const { return d_hasQuery; }

    /// @brief Check if the stored uri has the supported schema.
    /// @param errorDescription optional output for error description.
    /// @return 0 return code on success, non-zero return code on failure.
    inline int validateScheme(bsl::string* errorDescription) const
    {
        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(
                !d_uri.starts_with(k_SCHEME_SEPARATED))) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;

            BMQT_RETURN_WITH_ERROR(UriParser::UriParseResult::e_INVALID_SCHEME,
                                   errorDescription,
                                   "Invalid scheme: uri must start with \""
                                       << k_SCHEME_SEPARATED
                                       << "\"");  // RETURN
        }
        return UriParser::UriParseResult::e_SUCCESS;
    }

    /// @brief Parse the domain part of the stored uri.
    /// @param errorDescription optional output for error description.
    /// @param length output domain length after parsing, if rc == 0.
    /// @param start domain parse start position.
    /// @return 0 return code on success, non-zero return code on failure.
    /// NOTE: domain might be followed by optional tier, and we set `d_hasTier`
    ///       flag if we encounter it for later parsing.
    inline int
    parseDomain(bsl::string* errorDescription, size_t* length, size_t start)
    {
        // bmq://my-domain.~dv/queue?id=foo
        //       ^        ^
        //       start    (start + (*length))
        // Allowed characters: [-a-zA-Z0-9\\._]
        for (size_t pos = start; pos < d_uri.length(); ++pos) {
            if (isalnum_fast(d_uri[pos])) {
                continue;
            }

            switch (d_uri[pos]) {
            case '-': BSLA_FALLTHROUGH;
            case '_': {
                continue;
            }
            case '.': {
                if (pos + 1 < d_uri.length() && d_uri[pos + 1] == '~') {
                    d_hasTier = true;
                    *length   = pos - start;
                    BMQT_VALIDATE_AND_RETURN(
                        *length,
                        UriParser::UriParseResult::e_MISSING_DOMAIN,
                        errorDescription,
                        "Missing domain");  // RETURN
                }
                continue;
            }
            case '/': {
                // The end of the domain with no tier specified
                *length = pos - start;
                BMQT_VALIDATE_AND_RETURN(
                    *length,
                    UriParser::UriParseResult::e_MISSING_DOMAIN,
                    errorDescription,
                    "Missing domain");  // RETURN
            }
            default: {
                BMQT_RETURN_WITH_ERROR(
                    UriParser::UriParseResult::e_UNSUPPORTED_CHAR,
                    errorDescription,
                    "Domain parsing failed: unsupported char (int)"
                        << static_cast<int>(d_uri[pos]));  // RETURN
            }
            }
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_uri.length() <= start)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // We tried to parse the domain from the position out of `d_uri`
            // length.  Nothing to parse: empty domain.
            BMQT_RETURN_WITH_ERROR(UriParser::UriParseResult::e_MISSING_DOMAIN,
                                   errorDescription,
                                   "Missing domain");  // RETURN
        }

        // If we are here, we have fully iterated over `d_uri` in the loop.
        // We are sure that there is no tier or queue path after this,
        // and we can fast-forward the error.
        BMQT_RETURN_WITH_ERROR(UriParser::UriParseResult::e_MISSING_QUEUE,
                               errorDescription,
                               "Missing queue");  // RETURN
    }

    /// @brief Parse the tier part of the stored uri.
    /// @param errorDescription optional output for error description.
    /// @param length output tier length after parsing, if rc == 0.
    /// @param start tier parse start position.
    /// @return 0 return code on success, non-zero return code on failure.
    inline int
    parseTier(bsl::string* errorDescription, size_t* length, size_t start)
    {
        // bmq://my-domain.~development/queue?id=foo
        //                  ^          ^
        //                  start      (start + (*length))
        // Allowed characters: [-a-zA-Z0-9]
        for (size_t pos = start; pos < d_uri.length(); ++pos) {
            if (isalnum_fast(d_uri[pos])) {
                continue;
            }

            switch (d_uri[pos]) {
            case '-': {
                continue;
            }
            case '/': {
                // The end of the tier
                *length = pos - start;
                BMQT_VALIDATE_AND_RETURN(
                    *length,
                    UriParser::UriParseResult::e_EMPTY_TIER,
                    errorDescription,
                    "Empty tier");  // RETURN
            }
            default: {
                BMQT_RETURN_WITH_ERROR(
                    UriParser::UriParseResult::e_UNSUPPORTED_CHAR,
                    errorDescription,
                    "Tier parsing failed: unsupported char (int)"
                        << static_cast<int>(d_uri[pos]));  // RETURN
            }
            }
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_uri.length() <= start)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // We tried to parse the tier from the position out of `d_uri`
            // length.  Nothing to parse: empty tier.
            BMQT_RETURN_WITH_ERROR(UriParser::UriParseResult::e_EMPTY_TIER,
                                   errorDescription,
                                   "Empty tier");  // RETURN
        }

        // If we are here, we have fully iterated over `d_uri` in the loop.
        // We are sure that there is no queue path after this,
        // and we can fast-forward the error.
        BMQT_RETURN_WITH_ERROR(UriParser::UriParseResult::e_MISSING_QUEUE,
                               errorDescription,
                               "Missing queue");  // RETURN
    }

    /// @brief Parse the path part of the stored uri.
    /// @param errorDescription optional output for error description.
    /// @param length output path length after parsing, if rc == 0.
    /// @param start path parse start position.
    /// @return 0 return code on success, non-zero return code on failure.
    /// NOTE: path might be followed by optional query, and we set `d_hasQuery`
    ///       flag if we encounter it for later parsing.
    inline int
    parsePath(bsl::string* errorDescription, size_t* length, size_t start)
    {
        // bmq://my-domain.~dv/queue_abcdef?id=foo
        //                     ^           ^
        //                     start       (start + (*length))
        // Allowed characters: [-a-zA-Z0-9_\\.]
        for (size_t pos = start; pos < d_uri.length(); ++pos) {
            if (isalnum_fast(d_uri[pos])) {
                continue;
            }

            switch (d_uri[pos]) {
            case '-': BSLA_FALLTHROUGH;
            case '_': BSLA_FALLTHROUGH;
            case '.': {
                continue;
            }
            case '?': {
                // The end of the path, and start of a query
                d_hasQuery = true;
                *length    = pos - start;
                BMQT_VALIDATE_AND_RETURN(
                    *length,
                    UriParser::UriParseResult::e_MISSING_QUEUE,
                    errorDescription,
                    "Missing queue");  // RETURN
            }
            default: {
                BMQT_RETURN_WITH_ERROR(
                    UriParser::UriParseResult::e_UNSUPPORTED_CHAR,
                    errorDescription,
                    "Path parsing failed: unsupported char (int)"
                        << static_cast<int>(d_uri[pos]));  // RETURN
            }
            }
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_uri.length() <= start)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            // We tried to parse the path from the position out of `d_uri`
            // length.  Nothing to parse: empty path.
            BMQT_RETURN_WITH_ERROR(UriParser::UriParseResult::e_MISSING_QUEUE,
                                   errorDescription,
                                   "Missing queue");  // RETURN
        }

        // If we are here, `d_uri` already ended.
        // This is a valid scenario if there is no query.
        *length = d_uri.length() - start;
        return UriParser::UriParseResult::e_SUCCESS;
    }

    /// @brief Parse the query part of the stored uri.
    /// @param errorDescription optional output for error description.
    /// @param qtype output query type, if rc == 0.
    /// @param value_start output query value start, if rc == 0.
    /// @param value_length output query value length, if rc == 0.
    /// @param start query parse start position.
    /// @return 0 return code on success, non-zero return code on failure.
    /// NOTE: currently we don't support multiple queries.
    // Allowed characters:
    //     key       = any
    //     separator = [=]
    //     value     = [-a-zA-Z0-9_\\.]
    inline int parseQuery(bsl::string*     errorDescription,
                          QueryType::Enum* qtype,
                          size_t*          value_start,
                          size_t*          value_length,
                          size_t           start)
    {
        // bmq://my-domain.~dv/queue_abcdef?id=foo
        //                                  ^     ^
        //                                  start (start + (*length))

        // Might re-set this flag if another query encountered during parsing
        d_hasQuery = false;

        // 1. Find query separator
        size_t separatorPos = start;
        for (; separatorPos < d_uri.length(); ++separatorPos) {
            if (d_uri[separatorPos] == '=')
                break;
        }

        if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(d_uri.length() <=
                                                  separatorPos + 1)) {
            BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
            if (d_uri.length() == separatorPos + 1) {
                BMQT_RETURN_WITH_ERROR(UriParser::UriParseResult::e_BAD_QUERY,
                                       errorDescription,
                                       "Query parsing failed: missing value "
                                       "after \"=\"");  // RETURN
            }
            else {
                BMQT_RETURN_WITH_ERROR(
                    UriParser::UriParseResult::e_BAD_QUERY,
                    errorDescription,
                    "Query parsing failed: no separator found");  // RETURN
            }
        }

        // Currently we only support one query type "id".
        // The following code can be generalized to check a known set of
        // queries:
        const bsl::string_view query = d_uri.substr(start,
                                                    separatorPos - start);
        if (query != k_QUERY_ID) {
            BMQT_RETURN_WITH_ERROR(UriParser::UriParseResult::e_BAD_QUERY,
                                   errorDescription,
                                   "Query parsing failed: unsupported query \""
                                       << query << "\"");  // RETURN
        }

        for (size_t pos = separatorPos + 1; pos < d_uri.length(); ++pos) {
            if (isalnum_fast(d_uri[pos])) {
                continue;
            }

            // The following code can be generalized to support multiple
            // queries by handling case '&' (set d_hasQuery = true)
            switch (d_uri[pos]) {
            case '-': BSLA_FALLTHROUGH;
            case '_': BSLA_FALLTHROUGH;
            case '.': {
                continue;
            }
            default: {
                BMQT_RETURN_WITH_ERROR(
                    UriParser::UriParseResult::e_UNSUPPORTED_CHAR,
                    errorDescription,
                    "Query parsing failed: unsupported char (int)"
                        << static_cast<int>(d_uri[pos]));  // RETURN
            }
            }
        }

        // If we are here, `d_uri` already ended.
        // This is the only supported scenario now for a single query.
        *qtype        = QueryType::e_ID;
        *value_start  = separatorPos + 1;
        *value_length = d_uri.length() - (*value_start);
        return UriParser::UriParseResult::e_SUCCESS;
    }

    inline int validateResult(bsl::string*     errorDescription,
                              const bmqt::Uri& result)
    {
        if (result.domain().isEmpty() || result.authority().isEmpty()) {
            BMQT_RETURN_WITH_ERROR(UriParser::UriParseResult::e_MISSING_DOMAIN,
                                   errorDescription,
                                   "Missing domain");  // RETURN
        }

        if (hasTier() && result.tier().isEmpty()) {
            BMQT_RETURN_WITH_ERROR(UriParser::UriParseResult::e_EMPTY_TIER,
                                   errorDescription,
                                   "Empty tier");  // RETURN
        }

        if (result.path().isEmpty()) {
            BMQT_RETURN_WITH_ERROR(UriParser::UriParseResult::e_MISSING_QUEUE,
                                   errorDescription,
                                   "Missing queue");  // RETURN
        }

        if (Uri::k_QUEUENAME_MAX_LENGTH < result.path().length()) {
            BMQT_RETURN_WITH_ERROR(
                UriParser::UriParseResult::e_QUEUE_NAME_TOO_LONG,
                errorDescription,
                "Queue name exceeds " << Uri::k_QUEUENAME_MAX_LENGTH
                                      << " characters");  // RETURN
        }

        return UriParser::UriParseResult::e_SUCCESS;
    }
};

#undef BMQT_VALIDATE_AND_RETURN
#undef BMQT_RETURN_WITH_ERROR

}  // close unnamed namespace

// ---------
// class Uri
// ---------

Uri::Uri(bslma::Allocator* allocator)
: d_uri(allocator)
{
    // NOTHING
}

Uri::Uri(const Uri& original, bslma::Allocator* allocator)
: d_uri(allocator)
{
    copyImpl(original);
}

Uri::Uri(const bslstl::StringRef& uri, bslma::Allocator* allocator)
: d_uri(allocator)
{
    UriParser::parse(this, 0, uri);
}

Uri::Uri(const char* uri, bslma::Allocator* allocator)
: d_uri(allocator)
{
    UriParser::parse(this, 0, uri);
}

Uri::Uri(const bsl::string& uri, bslma::Allocator* allocator)
: d_uri(allocator)
{
    UriParser::parse(this, 0, uri);
}

Uri::~Uri()
{
    // NOTHING
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

void UriParser::initialize(BSLA_UNUSED bslma::Allocator* allocator)
{
    // NOTHING
}

void UriParser::shutdown()
{
    // NOTHING
}

int UriParser::parse(Uri*                     result,
                     bsl::string*             errorDescription,
                     const bslstl::StringRef& uriString)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(result);

#define BMQT_RETURN_ON_BAD_RC(RC, RES)                                        \
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(                                \
            UriParser::UriParseResult::e_SUCCESS != (RC))) {                  \
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;                                   \
        (RES)->reset();                                                       \
        return (RC);                                                          \
    }

    UriParsingContext ctx(uriString);

    result->reset();
    result->d_uri.assign(uriString.data(), uriString.length());

    // Step 1: verify BlazingMQ uri scheme
    // bmq://my-domain.~dv/queue_abcdef?id=foo
    //[bmq]    - scheme
    //[bmq://] - scheme with separator
    int rc = ctx.validateScheme(errorDescription);
    BMQT_RETURN_ON_BAD_RC(rc, result);

    result->d_scheme.assign(k_SCHEME);

    // Step 2: parse domain
    // bmq://my-domain.~dv/queue_abcdef?id=foo
    //       ^
    //       domainStart = k_SCHEME_SEPARATED.length()
    // The current parse position: skip the scheme we just verified.
    const size_t domainStart  = k_SCHEME_SEPARATED.length();
    size_t       domainLength = 0;
    rc = ctx.parseDomain(errorDescription, &domainLength, domainStart);
    BMQT_RETURN_ON_BAD_RC(rc, result);

    result->d_domain.assign(result->d_uri.data() + domainStart, domainLength);

    // Step 3: parse tier (optional)
    // bmq://my-domain.~dv/queue_abcdef?id=foo
    //                 [dv] - tier
    if (ctx.hasTier()) {
        // Do not include ".~" in tier (+2):
        const size_t tierStart  = domainStart + domainLength + 2;
        size_t       tierLength = 0;
        rc = ctx.parseTier(errorDescription, &tierLength, tierStart);
        BMQT_RETURN_ON_BAD_RC(rc, result);

        result->d_tier.assign(result->d_uri.data() + tierStart, tierLength);
    }

    // Step 4: set up authority
    // bmq://my-domain.~dv/queue_abcdef?id=foo
    //      [my-domain.~dv] - authority
    // Note that we include both domain and optional tier continuously:
    // tier separator ".~" is also included (+2).
    const size_t authorityLength =
        domainLength + (ctx.hasTier() ? (2 + result->d_tier.length()) : 0);
    result->d_authority.assign(result->d_uri.data() + domainStart,
                               authorityLength);

    // Step 5: parse path
    // bmq://my-domain.~dv/queue_abcdef?id=foo
    //                    [queue_abcdef] - path
    // Skip "/" separator (+1) between authority and path:
    const size_t pathStart  = domainStart + authorityLength + 1;
    size_t       pathLength = 0;
    rc = ctx.parsePath(errorDescription, &pathLength, pathStart);
    BMQT_RETURN_ON_BAD_RC(rc, result);

    result->d_path.assign(result->d_uri.data() + pathStart, pathLength);

    // Step 6: parse optional queries
    // bmq://my-domain.~dv/queue_abcdef?id=foo
    //                                 [id=foo] - query
    //                                 [id]     - query key
    //                                    [foo] - query value
    size_t pos = pathStart + pathLength + 1;
    while (ctx.hasQuery()) {
        UriParsingContext::QueryType::Enum qtype =
            UriParsingContext::QueryType::e_ID;
        size_t value_start  = 0;
        size_t value_length = 0;

        rc = ctx.parseQuery(errorDescription,
                            &qtype,
                            &value_start,
                            &value_length,
                            pos);
        BMQT_RETURN_ON_BAD_RC(rc, result);

        switch (qtype) {
        case UriParsingContext::QueryType::e_ID: {
            result->d_query_id.assign(result->d_uri.data() + value_start,
                                      value_length);
        } break;

        default: {
            BSLS_ASSERT_OPT(false && "Unsupported value");
        } break;
        }

        // Skip next query separator '&' (+1):
        pos = value_start + value_length + 1;

        // Currently we guarantee that there is at most one query.
        // The following check might be removed if we support multiple.
        BSLS_ASSERT_SAFE(uriString.length() <= pos);
    }

    // Step 7: validate mandatory fields
    rc = ctx.validateResult(errorDescription, *result);
    BMQT_RETURN_ON_BAD_RC(rc, result);

#undef BMQT_RETURN_ON_BAD_RC

    return bmqt::UriParser::UriParseResult::e_SUCCESS;
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
