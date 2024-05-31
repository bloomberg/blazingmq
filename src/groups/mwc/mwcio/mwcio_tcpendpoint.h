// Copyright 2019-2023 Bloomberg Finance L.P.
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

// mwcio_tcpendpoint.h                                                -*-C++-*-
#ifndef INCLUDED_MWCIO_TCPENDPOINT
#define INCLUDED_MWCIO_TCPENDPOINT

//@PURPOSE: Value-semantic type representing endpoints for 'mwcio' channels
//
//@CLASSES:
//  mwcio::TCPEndpoint
//
//@SEE_ALSO:
//
//@DESCRIPTION: This component represents a value-semantic type,
// 'mwcio::TCPEndpoint', to convert between a URI ('tcp://host:port')
// representation and a { host, port } pair, to use with the 'mwcio' channels.
// This is simply a convenient utility object to create a URI from an host and
// port.  Note that the URI format must be of the form 'tcp://:int' (the host
// is optional, but the port is not; host can either be an IP address or a host
// name).  Creating an 'TCPEndpoint' by URI with should be followed by a call
// to operator bool() to check if the endpoint is valid.
//
// This component should not be used (directly) by client code.  This component
// is an implementation detail of 'dmp' and 'mbs' and is *not* intended for
// direct client use.  It is subject to change without notice.

// MWC

// BDE
#include <bsl_iosfwd.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {

namespace mwcio {

// =================
// class TCPEndpoint
// =================

/// TCPEndpoint representation
class TCPEndpoint {
  private:
    bsl::string d_uri;
    int         d_port;
    bsl::string d_host;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TCPEndpoint, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an empty `TCPEndpoint`.
    explicit TCPEndpoint(bslma::Allocator* allocator = 0);

    /// Create an `TCPEndpoint` initialized with the specified `uri`.
    explicit TCPEndpoint(const bsl::string& uri,
                         bslma::Allocator*  allocator = 0);

    /// Create an `TCPEndpoint` initialized with the specified `host` and
    /// `port`.
    TCPEndpoint(const bslstl::StringRef& host,
                int                      port,
                bslma::Allocator*        allocator = 0);

    // MANIPULATORS

    /// Set the uri of this endpoint to `uri`. Return true on success, or
    /// false if the uri failed to parse.
    bool fromUri(const bsl::string& uri);

    /// DEPRECATED: use `fromUri` instead.
    /// Set the uri of this endpoint to `uri`.  Expects uri to be valid
    /// format.
    void fromUriRaw(const bsl::string& uri);

    /// Set the host and port value of this endpoint to the specified `host`
    /// and `port`.
    TCPEndpoint& assign(const bslstl::StringRef& host, int port);

    // ACCESSORS

    /// Return true if the endpoint is valid.
    operator bool() const;

    /// Return the `port` part of the endpoint.
    int port() const;

    /// Return the `host` part of the endpoint.
    const bsl::string& host() const;

    /// Return the uri of the endpoint (tcp://...) or empty string if the
    /// endpoint is invalid.
    const bsl::string& uri() const;

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
bsl::ostream& operator<<(bsl::ostream& stream, const TCPEndpoint& value);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------
// class TCPEndpoint
// -----------------

// ACCESSORS
inline TCPEndpoint::operator bool() const
{
    return !d_uri.empty();
}

inline int TCPEndpoint::port() const
{
    return d_port;
}

inline const bsl::string& TCPEndpoint::host() const
{
    return d_host;
}

inline const bsl::string& TCPEndpoint::uri() const
{
    return d_uri;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mwcio::operator<<(bsl::ostream&             stream,
                                       const mwcio::TCPEndpoint& endpoint)
{
    return endpoint.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
