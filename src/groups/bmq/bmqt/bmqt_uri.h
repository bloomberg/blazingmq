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

// bmqt_uri.h                                                         -*-C++-*-
#ifndef INCLUDED_BMQT_URI
#define INCLUDED_BMQT_URI

//@PURPOSE: Provide value-semantic type and utilities for a BlazingMQ queue
// URI.
//
//@CLASSES:
//  bmqt::Uri        : value-semantic type representing a URI
//  bmqt::UriParser  : utility to parse a string into a URI
//  bmqt::UriBuilder : builder mechanism to create a URI
//
//@DESCRIPTION: This component provides a value-semantic type, 'bmqt::Uri'
// representing a URI for a BlazingMQ queue.  A 'bmqt:Uri' can be built by
// parsing from a string representation, using the 'bmqt::UriParser', or built
// using the 'bmqt::UriBuilder'.
//
//@SEE_ALSO:
//----------
// https://tools.ietf.org/html/rfc3986
//
/// URI format
///----------
// In a nutshell, a URI representing an application queue managed by a
// BlazingMQ broker on a given machine looks like one of the following:
//..
//  bmq://ts.trades.myapp/my.queue
//  bmq://ts.trades.myapp.~bt/my.queue
//  bmq://ts.trades.myapp/my.queue?id=foo
//..
// where:
//
//: o The URI scheme is always "bmq".
//
//: o The URI authority is the name of BlazingMQ domain (such as
//    "ts.trades.myapp")
//:   as registered with the BlazingMQ infrastructure.  The domain name may
//:   contain alphanumeric characters, dots and dashes (it has to match the
//:   following regular expression: '[-a-zA-Z0-9\\._]+').  The domain may be
//:   followed by an optional tier, introduced by the ".~" sequence and
//:   consisting of alphanumeric characters and dashes.  The ".~" sequence is
//:   not part of the tier.
//
//: o The URI path is the name of the queue ("my.queue" above) and may contain
//:   alphanumeric characters, dashes, underscores and tild (it has to match
//:   the following regular expression: '[-a-zA-Z0-9_~\\.]+').  The queue name
//:   must be less than 'bmqt::Uri::k_QUEUENAME_MAX_LENGTH' characters long.
//
//: o The name of the queue ("my.queue" above) may contain alphanumeric
//:   characters and dots.
//
//: o The URI may contain an optional query with a key-value pair.  Currently
//:   supported keys are:
//:   o !id!: the corresponding value represents a name that will be used by
//:     BlazingMQ broker to uniquely identify the client.
//
//: o The URI fragment part is currently unused.
//
/// Usage Example 1
///---------------
// First, call the 'initialize' method of the 'UriParser'.  This call is only
// needed one time; you can call it when your task starts.
//
// Note that the 'bmq' library takes care of that, so users of 'bmq' don't
// have to explicitly do it themselves.
//..
//  bmqt::UriParser::initialize();
//..
// Then, parse a URI string created on the stack to populate a 'Uri' object.
// The parse function takes an optional error string which is populated with a
// short error message if the URI is not formatted correctly.
//..
//   bsl::string input = "bmq://my.domain/queue";
//   bmqt::Uri   uri(allocator);
//   bsl::string errorDescription;
//
//   int         rc = bmqt::UriParser::parse(&uri, &errorDescription, input);
//   if (rc != 0) {
//      BALL_LOG_ERROR << "Invalid URI [error: " << errorDescription << "]";
//   }
//   assert(rc == 0);
//   assert(error == "");
//   assert(uri.scheme()    == "bmq");
//   assert(uri.domain()    == "my.domain");
//   assert(uri.queue()     == "queue");
//..
//
/// Usage Example 2
///----------------
// Instantiate a 'Uri' object with a string representation of the URI and an
// allocator.
//..
//  bmqt::Uri uri("bmq://my.domain/queue", allocator);
//  assert(uri.scheme() == "bmq");
//  assert(uri.domain() == "my.domain");
//  assert(uri.queue()  == "queue");
//..

/// Thread Safety
///-------------
//: o 'bmqt::UriBuilder' is NOT thread safe
//: o 'bmqt::UriParser' should be thread safe: the component depends on
//:   'bdepcre_regex' that is a wrapper around "pcre.h".  See
//:    http://www.pcre.org/pcre.txt.

// BMQ

// BDE
#include <ball_log.h>
#include <bsl_cstddef.h>
#include <bsl_iosfwd.h>
#include <bsl_string.h>
#include <bslh_hash.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

// FORWARD DECLARATION
namespace bmqt {
class UriBuilder;
}
namespace bmqt {
struct UriParser;
}

namespace bmqt {

// =========
// class Uri
// =========

/// Value semantic type representing a URI
class Uri {
  public:
    // PUBLIC CONSTANTS

    /// The maximum authorized length for the queue name part of the URI.
    static const int k_QUEUENAME_MAX_LENGTH = 64;

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQT.URI");

  private:
    // FRIENDS
    friend struct UriParser;
    friend class UriBuilder;
    template <class HASH_ALGORITHM>
    friend void hashAppend(HASH_ALGORITHM& hashAlgo, const Uri& uri);

  private:
    // DATA
    bsl::string d_uri;  // The full URI

    bslstl::StringRef d_scheme;  // URI scheme (must be "bmq").

    bslstl::StringRef d_authority;  // URI authority (domain + optional
                                    // tier)

    bslstl::StringRef d_domain;  // URI domain

    bslstl::StringRef d_tier;  // URI tier

    bslstl::StringRef d_path;  // URI path (i.e queue name).

    bslstl::StringRef d_query_id;  // Optional application id, part of the
                                   // URI query if present.

    bool d_wasParserInitialized;
    // Flag indicating whether the URI
    // parser was initialized (and whether
    // shutdown should be called on it at
    // destruction)

  private:
    // PRIVATE MANIPULATORS

    /// Reset this object to the default value.
    void reset();

    /// Implementation of the copy of the specified `src` URI into this
    /// object.
    void copyImpl(const Uri& src);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(Uri, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Constructor of an invalid Uri with all fields empty, using the
    /// optionally specified `allocator`.
    explicit Uri(bslma::Allocator* allocator = 0);

    /// Copy constructor, create a new Uri having the same values as the
    /// specified `original`, and using the optionally specified
    /// `allocator`.
    Uri(const Uri&        original,
        bslma::Allocator* allocator = 0);  // IMPLICIT

    /// Implicit constructor of this object from the specified `uri` string
    /// using the optionally specified `allocator`.  If the `uri` input
    /// string doesn't not represent a valid URI, this object is left in an
    /// invalid state (isValid() will return false).
    Uri(const bsl::string& uri,
        bslma::Allocator*  allocator = 0);  // IMPLICIT
    Uri(const bslstl::StringRef& uri,
        bslma::Allocator*        allocator = 0);  // IMPLICIT
    Uri(const char*       uri,
        bslma::Allocator* allocator = 0);  // IMPLICIT

    /// Destructor.
    ~Uri();

    // MANIPULATORS

    /// Set the value of this object to the specified `rhs`.
    Uri& operator=(const Uri& rhs);

    // ACCESSORS

    /// Return the string representation of this URI.
    const bsl::string& asString() const;

    /// Return true if this object represents a valid URI.
    bool isValid() const;

    /// Return true if this object represents a canonical URI.
    bool isCanonical() const;

    const bslstl::StringRef& scheme() const;
    const bslstl::StringRef& authority() const;

    /// Return the corresponding (raw) part of the URI, matching to the URI
    /// RFC terminology.
    const bslstl::StringRef& path() const;

    const bslstl::StringRef& qualifiedDomain() const;
    const bslstl::StringRef& domain() const;
    const bslstl::StringRef& tier() const;
    const bslstl::StringRef& queue() const;

    /// Return the corresponding (extracted) part of the URI, provided as
    /// convenient accessors using the BlazingMQ terminology.
    const bslstl::StringRef& id() const;

    /// Return the canonical form of the URI. Note that canonical form
    /// includes everything except the query part of the URI.
    bslstl::StringRef canonical() const;

    /// Format this object to the specified output `stream` at the (absolute
    /// value of) the optionally specified indentation `level` and return a
    /// reference to `stream`.  If `level` is specified, optionally specify
    /// `spacesPerLevel`, the number of spaces per indentation level for
    /// this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  If `stream` is
    /// not valid on entry, this operation has no effect.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
};

// FREE OPERATORS

/// Return `true` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return false otherwise.
bool operator==(const Uri& lhs, const Uri& rhs);

/// Return `false` if the specified `rhs` object contains the value of the
/// same type as contained in the specified `lhs` object and the value
/// itself is the same in both objects, return `true` otherwise.
bool operator!=(const Uri& lhs, const Uri& rhs);

/// Return `true` if the specified `lhs` object compares less than the
/// specified `rhs` object.
bool operator<(const Uri& lhs, const Uri& rhs);

/// Format the specified `rhs` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const Uri& rhs);

// ================
// struct UriParser
// ================

/// Utility namespace of methods for parsing URI strings into `Uri` objects.
struct UriParser {
    // CLASS METHODS

    /// Initialize the `UriParser`.  Note that this will compile the regular
    /// expression used by `parseUri`.  This method only needs to be called
    /// once before any other method, but can be called multiple times
    /// provided that for each call to `initialize` there is a corresponding
    /// call to `shutdown`.  Use the optionally specified `allocator` for
    /// any memory allocation, or the `global` allocator if none is
    /// provided.  Note that specifying the allocator is provided for test
    /// drivers only, and therefore users should let it default to the
    /// global allocator.
    static void initialize(bslma::Allocator* allocator = 0);

    /// Pendant operation of the `initialize` one.  Note that behaviour
    /// after calling the `.parse()` method of the `UriParser` after
    /// `shutdown` has been called is undefined.  The number of calls to
    /// `shutdown` must equal the number of calls to `initialize`, without
    /// corresponding `shutdown` calls, to fully destroy the parser.  It is
    /// safe to call `initialize` after calling `shutdown`.  Behaviour is
    /// undefined if `shutdown` is called without `initialize` first being
    /// called.
    static void shutdown();

    /// Parse the specified `uriString` into the specified `result` object
    /// if `uriString` is a valid URI, otherwise load the specified
    /// `errorDescription` with a description of the syntax error present in
    /// `uriString`.  Return 0 on success and non-zero if `uriString` does
    /// not have a valid syntax.  Note that `errorDescription` may be null
    /// if the caller does not care about getting error messages.  The
    /// behavior is undefined unless `initialize` has been called
    /// previously.
    static int parse(Uri*                     result,
                     bsl::string*             errorDescription,
                     const bslstl::StringRef& uriString);

  private:
    // CLASS-SCOPE CATEGORY
    BALL_LOG_SET_CLASS_CATEGORY("BMQT.URI");
};

// ================
// class UriBuilder
// ================

/// This component implements a mechanism, `bmqt::UriBuilder`, that can be
/// used for creating queue URI for BMQ.
class UriBuilder {
  private:
    // DATA
    Uri d_uri;  // Placeholder object for URI object being built.

  private:
    // NOT IMPLEMENTED

    /// Copy constructor and assignment operator not implemented.
    UriBuilder(const UriBuilder&);
    UriBuilder& operator=(const UriBuilder&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(UriBuilder, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new UriBuilder using the optionally specified `allocator`.
    explicit UriBuilder(bslma::Allocator* allocator = 0);

    /// Create a new UriBuilder initialized with the specified `uri` and
    /// using the optionally specified `allocator`.
    explicit UriBuilder(const bmqt::Uri& uri, bslma::Allocator* allocator = 0);

    // MANIPULATORS
    UriBuilder& setDomain(const bslstl::StringRef& value);
    UriBuilder& setTier(const bslstl::StringRef& value);
    UriBuilder& setQualifiedDomain(const bslstl::StringRef& value);
    UriBuilder& setQueue(const bslstl::StringRef& value);

    /// Set the corresponding field of the URI to the specified `value`.
    /// The behavior is undefined unless `value` remains valid until URI has
    /// been built by invoking `uri()`.  `setDomain()` and `setTier()`
    /// should be preferred over `setQualifiedDomain()` whenever possible.
    UriBuilder& setId(const bslstl::StringRef& value);

    /// Reset all fields of this builder.
    void reset();

    // ACCESSORS

    /// Build and populate the specified `result` with the built URI,
    /// returning 0 on success, or return non-zero on error, populating the
    /// optionally specified `errorDescription` if provided.
    int uri(Uri* result, bsl::string* errorDescription = 0) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------
// class Uri
// ---------

inline Uri& Uri::operator=(const Uri& rhs)
{
    copyImpl(rhs);

    return *this;
}

inline const bsl::string& Uri::asString() const
{
    return d_uri;
}

inline bool Uri::isValid() const
{
    // URI can only be set using either 'UriBuilder' or 'UriParser', which are
    // resetting the d_uri on failure.
    return !d_uri.empty();
}

inline bool Uri::isCanonical() const
{
    return d_query_id.isEmpty();
}

inline const bslstl::StringRef& Uri::scheme() const
{
    return d_scheme;
}

inline const bslstl::StringRef& Uri::authority() const
{
    return d_authority;
}

inline const bslstl::StringRef& Uri::path() const
{
    return d_path;
}

inline const bslstl::StringRef& Uri::domain() const
{
    return d_domain;
}

inline const bslstl::StringRef& Uri::qualifiedDomain() const
{
    return d_authority;
}

inline const bslstl::StringRef& Uri::tier() const
{
    return d_tier;
}

inline const bslstl::StringRef& Uri::queue() const
{
    return d_path;
}

inline const bslstl::StringRef& Uri::id() const
{
    return d_query_id;
}

inline bslstl::StringRef Uri::canonical() const
{
    size_t queryBeginPos = d_uri.find_first_of('?');
    if (bsl::string::npos == queryBeginPos) {
        return d_uri;  // RETURN
    }

    return bslstl::StringRef(d_uri.c_str(), queryBeginPos);
}

// FREE FUNCTIONS
template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const Uri& uri)
{
    using bslh::hashAppend;           // for ADL
    hashAppend(hashAlgo, uri.d_uri);  // hashes full uri string
}

// ----------------
// class UriBuilder
// ----------------

inline UriBuilder& UriBuilder::setDomain(const bslstl::StringRef& value)
{
    d_uri.d_domain = value;
    return *this;
}

inline UriBuilder& UriBuilder::setTier(const bslstl::StringRef& value)
{
    d_uri.d_tier = value;
    return *this;
}

inline UriBuilder& UriBuilder::setQueue(const bslstl::StringRef& value)
{
    d_uri.d_path = value;
    return *this;
}

inline UriBuilder& UriBuilder::setId(const bslstl::StringRef& value)
{
    d_uri.d_query_id = value;
    return *this;
}

}  // close package namespace

// FREE OPERATORS

inline bool bmqt::operator==(const bmqt::Uri& lhs, const bmqt::Uri& rhs)
{
    return (lhs.asString() == rhs.asString());
}

inline bool bmqt::operator!=(const bmqt::Uri& lhs, const bmqt::Uri& rhs)
{
    return (lhs.asString() != rhs.asString());
}

inline bool bmqt::operator<(const bmqt::Uri& lhs, const bmqt::Uri& rhs)
{
    return (lhs.asString() < rhs.asString());
}

inline bsl::ostream& bmqt::operator<<(bsl::ostream&    stream,
                                      const bmqt::Uri& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
