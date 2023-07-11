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

// mwcio_connectoptions.h                                             -*-C++-*-
#ifndef INCLUDED_MWCIO_CONNECTOPTIONS
#define INCLUDED_MWCIO_CONNECTOPTIONS

//@PURPOSE: Provide a type encapsulating options for 'ChannelFactory::connect'.
//
//@CLASSES:
//  mwcio::ConnectOptions: type encapsulating 'ChannelFactory::connect' options
//
//@DESCRIPTION: This component defines a value-semantic type,
// 'mwcio::ConnectOptions', which encapsulates various connection options
// passed to 'mwcio::ChannelFactory::connect'.

// MWC

#include <mwct_propertybag.h>

// BDE
#include <bsl_iosfwd.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_timeinterval.h>

namespace BloombergLP {

namespace mwcio {

// ====================
// class ConnectOptions
// ====================

/// Connection options used when calling `mwcio::ChannelFactory::connect`.
class ConnectOptions {
  private:
    // DATA
    bsl::string d_endpoint;  // implementation-defined endpoint to
                             // connect to

    int d_numAttempts;  // number of connection attempts to
                        // make before failing the connection

    bsls::TimeInterval d_attemptInterval;
    // time to wait between successive
    // connection attempts

    bool d_autoReconnect;  // should this connection be
                           // auto-reconnected when it goes down

    mwct::PropertyBag d_properties;  // additional properties

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(ConnectOptions, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a default `ConnectOptions` object with with unlimited
    /// attempts and a 10 second interval between attempts.
    explicit ConnectOptions(bslma::Allocator* basicAllocator = 0);

    ConnectOptions(const ConnectOptions& original,
                   bslma::Allocator*     basicAllocator = 0);

    // MANIPULATORS

    /// Assign to this object the value of the specified `rhs` object.
    ConnectOptions& operator=(const ConnectOptions& rhs);

    /// Set the `endpoint` attribute of this object to the specified
    /// `value` and return a reference providing modifiable access to this
    /// object.
    ConnectOptions& setEndpoint(const bslstl::StringRef& value);

    /// Set the `numAttempts` attribute of this object to the specified
    /// `value` and return a reference providing modifiable access to this
    /// object.
    ConnectOptions& setNumAttempts(int value);

    /// Set the `attemptInterval` attribute of this object to the specified
    /// `value` and return a reference providing modifiable access to this
    /// object.
    ConnectOptions& setAttemptInterval(const bsls::TimeInterval& value);

    /// Set whether this connection should be recreated automatically if it
    /// fails to be established, or if it goes down after being
    /// established.  By default, this is `false`.
    ConnectOptions& setAutoReconnect(bool value);

    /// Return a reference providing modifiable access to the `properties`
    /// property of this object.
    mwct::PropertyBag& properties();

    // ACCESSORS

    /// Return a reference providing const access to the `endpoint` property
    /// of this object.
    const bsl::string& endpoint() const;

    /// Return the `numAttempts` property of this object.
    int numAttempts() const;

    /// Return a reference providing const access to the `attemptInterval`
    /// property of this object.
    const bsls::TimeInterval& attemptInterval() const;

    /// Return the `autoReconnect` property of this object.
    bool autoReconnect() const;

    /// Return a reference providing const access to the `properties`
    /// property of this object.
    const mwct::PropertyBag& properties() const;

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

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const ConnectOptions& obj);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// class ConnectOptions
// --------------------

// CREATORS
inline ConnectOptions::ConnectOptions(bslma::Allocator* basicAllocator)
: d_endpoint(basicAllocator)
, d_numAttempts(bsl::numeric_limits<int>::max())
, d_attemptInterval(10.0)
, d_autoReconnect(false)
, d_properties(basicAllocator)
{
}

inline ConnectOptions::ConnectOptions(const ConnectOptions& original,
                                      bslma::Allocator*     basicAllocator)
: d_endpoint(original.d_endpoint, basicAllocator)
, d_numAttempts(original.d_numAttempts)
, d_attemptInterval(original.d_attemptInterval)
, d_autoReconnect(original.d_autoReconnect)
, d_properties(original.d_properties, basicAllocator)
{
}

// MANIPULATORS
inline ConnectOptions&
ConnectOptions::setEndpoint(const bslstl::StringRef& value)
{
    d_endpoint = value;
    return *this;
}

inline ConnectOptions& ConnectOptions::setNumAttempts(int value)
{
    d_numAttempts = value;
    return *this;
}

inline ConnectOptions&
ConnectOptions::setAttemptInterval(const bsls::TimeInterval& value)
{
    d_attemptInterval = value;
    return *this;
}

inline ConnectOptions& ConnectOptions::setAutoReconnect(bool value)
{
    d_autoReconnect = value;
    return *this;
}

inline mwct::PropertyBag& ConnectOptions::properties()
{
    return d_properties;
}

// ACCESSORS
inline const bsl::string& ConnectOptions::endpoint() const
{
    return d_endpoint;
}

inline int ConnectOptions::numAttempts() const
{
    return d_numAttempts;
}

inline const bsls::TimeInterval& ConnectOptions::attemptInterval() const
{
    return d_attemptInterval;
}

inline bool ConnectOptions::autoReconnect() const
{
    return d_autoReconnect;
}

inline const mwct::PropertyBag& ConnectOptions::properties() const
{
    return d_properties;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& mwcio::operator<<(bsl::ostream&                stream,
                                       const mwcio::ConnectOptions& obj)
{
    return obj.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
