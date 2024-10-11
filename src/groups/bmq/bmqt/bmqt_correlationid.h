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

// bmqt_correlationid.h                                               -*-C++-*-
#ifndef INCLUDED_BMQT_CORRELATIONID
#define INCLUDED_BMQT_CORRELATIONID

/// @file bmqt_correlationid.h
///
/// @brief Provide a value-semantic type usable as an efficient identifier.
///
/// This component implements a value-semantic class,
/// @bbref{bmqt::CorrelationId}, which can be used to identify any async
/// operations.  The correlationId contains a value (64-bit integer, raw
/// pointer or sharedPtr) supplied by the application or uses an auto-assigned
/// value.  The type and the value of the correlationId can be set at
/// construction time and changed later via the `setNumeric()`, `setPointer()`
/// and `setSharedPointer()` methods.  Alternatively, an `AutoValue` can be
/// used to generate a unique correlationId from within the process.  The
/// @bbref{bmqt::CorrelationIdLess} comparison functor can be used for storing
/// @bbref{bmqt::CorrelationId} in a map as the key element; and a hash functor
/// specialization is provided in the `bsl::hash` namespace.
///
/// AutoValue                                   {#bmqt_correlationid_autovalue}
/// =========
///
/// If the application doesn't care about the actual value of the correlation,
/// `AutoValue` type can be used to create a unique correlationId.  An
/// `AutoValue` correlationId behaves exactly the same as any other type of
/// correlation, with the exception that it's value can not be retrieved (but
/// two correlationId can be still compared equal).
///
/// ```
/// bmqt::CorrelationId corrId = bmqt::CorrelationId::autoValue();
/// ```
///
/// Usage                                           {#bmqt_correlationid_usage}
/// =====
///
/// This section illustrates intended use of this component.
///
/// Example 1: Correlating Responses                  {#bmqt_correlationid_ex1}
/// --------------------------------
///
/// Suppose that we have the following asynchronous messaging interface that we
/// want to use to implement a basic request/response class.
///
/// ```
/// class Messenger {
///
///   public:
///     // TYPES
///     typedef bsl::function<
///       void(void                       *buffer,
///            int                         bufferLength,
///            const bmqt::CorrelationId&  correlationId)> MessageHandler;
///         // 'MessageHandler' is an alias for a functor that handles received
///         // messages consisting of the specified 'buffer', having the
///         // specified 'bufferLength', as well as the specified associated
///         // 'correlationId' if the message is in response to a previously
///         // transmitted message.
///
///     // MANIPULATORS
///     void sendMessage(void                       *buffer,
///                      int                         bufferLength,
///                      const bmqt::CorrelationId&  correlationId);
///         // Send a message containing the specified 'buffer' of the
///         // specified 'bufferLength', associating the specified
///         // 'correlationId' with any messages later received in response.
///
///     void setMessageHandler(const MessageHandler& handler);
///         // Set the functor to handle messages received by this object to
///         // the specified 'handler'.
/// };
/// ```
///
/// First we declare a requester class.
///
/// ```
/// class Requester {
///
///     // DATA
///     Messenger        *d_messenger_p;  // used to send messages (held)
///     bslma::Allocator *d_allocator_p;  // memory supply (held)
///
///     // PRIVATE CLASS METHODS
///     static void handleMessage(void                       *buffer,
///                               int                         bufferLength,
///                               const bmqt::CorrelationId&  correlationId);
///         // Handle the response message consisting fo the specified 'buffer'
///         // having the specified 'bufferLength', and associated
///         // 'correlationId'.
///
///   public:
///     // TYPES
///     typedef bsl::function<void(void *buffer, int bufferLength)>
///                                                            ResponseCallback;
///         // 'ResponseCallback' is an alias for a functor that is used to
///         // process received responses consisting of the specified 'buffer'
///         // having the specified 'bufferLength'.
///
///     // CREATORS
///     explicit Requester(Messenger        *messenger,
///                        bslma::Allocator *basicAllocator);
///         // Create a 'Requester' that uses the specified 'messenger' to send
///         // requests and receive responses, and the specified
///         // 'basicAllocator' to supply memory.
///
///     // MANIPULATORS
///     void sendRequest(void                    *buffer,
///                      int                      bufferLength,
///                      const ResponseCallback&  callback);
///         // Send a request consisting of the specified 'buffer' having the
///         // specified 'bufferLength', and invoke the specified 'callback'
///         // with any asynchronous response.
/// };
/// ```
///
/// Then, we implement the constructor, setting the message handler on the
/// provided 'Messenger' to our class method.
///
/// ```
/// Requester::Requester(Messenger        *messenger,
///                      bslma::Allocator *basicAllocator)
/// : d_messenger_p(messenger)
/// , d_allocator_p(basicAllocator)
/// {
///     d_messenger_p->setMessageHandler(&Requester::handleMessage);
/// }
/// ```
///
/// Now, we implement `sendRequest`, copying the given `callback` into a
/// correlationId that is provided to the messenger.
///
/// ```
/// void Requester::sendRequest(void                    *buffer,
///                             int                      bufferLength,
///                             const ResponseCallback&  callback)
/// {
///     bsl::shared_ptr<ResponseCallback> callbackCopy;
///     callbackCopy.createInplace(d_allocator_p, callback, d_allocator_p);
///     bmqt::CorrelationId correlationId(bsl::shared_ptr<void>(callbackCopy));
///     d_messenger_p->sendMessage(buffer, bufferLength, correlationId);
/// }
/// ```
///
/// Finally, we implement our message handler, extracting the response callback
/// from the correlationId, and invoking it with the received response message.
///
/// ```
/// void Requester::handleMessage(void                       *buffer,
///                               int                         bufferLength,
///                               const bmqt::CorrelationId&  correlationId)
/// {
///     assert(correlationId.isSharedPtr());
///     ResponseCallback *callback = static_cast<ResponseCallback *>(
///                                        correlationId.theSharedPtr().get());
///     (*callback)(buffer, bufferLength);
/// }
/// ```

// BMQ

// BDE
#include <bdlb_variant.h>

#include <bsl_cstdint.h>
#include <bsl_iosfwd.h>
#include <bsl_memory.h>
#include <bslh_hash.h>
#include <bsls_assert.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqt {

// ===================
// class CorrelationId
// ===================

/// This class implements an in-core value-semantic type for correlating an
/// asynchronous result with the original operation.
class CorrelationId {
    // FRIENDS
    //  Needed to access getAsAutoValue
    friend bool operator==(const CorrelationId& lhs, const CorrelationId& rhs);
    friend bool operator!=(const CorrelationId& lhs, const CorrelationId& rhs);
    friend struct CorrelationIdLess;
    friend bsl::ostream& operator<<(bsl::ostream&        stream,
                                    const CorrelationId& rhs);

    template <class HASH_ALGORITHM>
    friend void hashAppend(HASH_ALGORITHM&      hashAlgo,
                           const CorrelationId& value);

  private:
    // PRIVATE TYPES

    /// `AutoValue` is an alias for a 64-bit unsigned integer value that is
    /// used to generate automatic, opaque, correlationId values.
    typedef bsls::Types::Uint64 AutoValue;

    // DATA

    /// The variant used to hold the value of a `CorrelationId`, that may
    /// either be unset, hold a 64-bit integer, a raw pointer, a shared
    /// pointer, or an `AutoValue`.
    bdlb::Variant4<bsls::Types::Int64, void*, bsl::shared_ptr<void>, AutoValue>
        d_variant;

    // PRIVATE ACCESSORS

    /// Return the value of this correlationId as an `AutoValue`.  The
    /// behavior is undefined unless `isAutoValue`.  Note that this method
    /// is only available in order to implement the comparison and hash
    /// operations.
    AutoValue theAutoValue() const;

  public:
    // TYPES
    enum Type {
        e_NUMERIC  // the 'CorrelationId' holds a 64-bit integer
        ,
        e_POINTER  // the 'CorrelationId' holds a raw pointer
        ,
        e_SHARED_PTR  // the 'CorrelationId' holds a shared pointer
        ,
        e_AUTO_VALUE  // the 'CorrelationId' holds an auto value
        ,
        e_UNSET  // the 'CorrelationId' is not set
    };

    // CLASS METHOD

    /// Return a `CorrelationId` having a unique, automatically assigned
    /// opaque value.
    static CorrelationId autoValue();

    // CREATORS

    /// Create a `CorrelationId` having the default, unset, value.
    explicit CorrelationId();

    /// Create a `CorrelationId` object initialized with the specified
    /// `numeric` integer value.
    explicit CorrelationId(bsls::Types::Int64 numeric);

    /// Create a `CorrelationId` object initialized with the specified
    /// `pointer` value.
    explicit CorrelationId(void* pointer);

    /// Create a `CorrelationId` object initialized with the specified
    /// `sharedPtr` shared pointer value.
    explicit CorrelationId(const bsl::shared_ptr<void>& sharedPtr);

    // MANIPULATORS

    /// Unset the value of this object.
    CorrelationId& makeUnset();

    /// Set the value of this object value to the specified `numeric`.
    /// Return a reference to this object.
    CorrelationId& setNumeric(bsls::Types::Int64 numeric);

    /// Set the value of this object value to the specified `pointer`.
    /// Return a reference to this object.
    CorrelationId& setPointer(void* pointer);

    /// Set the value of this object value to the specified `sharedPtr`.
    /// Return a reference to this object.
    CorrelationId& setSharedPointer(const bsl::shared_ptr<void>& sharedPtr);

    // void setAutoValue
    // There is explicitly no setAuto, autoCorrelation should only be used
    // at creation

    // ACCESSORS

    /// Return `true` if the value of this object is unset;
    bool isUnset() const;

    /// Return `true` if the value of this object is an integer value, and
    /// otherwise return `false`.
    bool isNumeric() const;

    /// Return `true` if the value of this object is a pointer value, and
    /// otherwise return `false`.
    bool isPointer() const;

    /// Return `true` if the value of this object is a shared pointer value,
    /// and otherwise return `false`.
    bool isSharedPtr() const;

    /// Return `true` if the value of this object is an auto correlation
    /// value, and otherwise return `false`.
    bool isAutoValue() const;

    /// Return the 64-bit integer value of this object.  The behavior is
    /// undefined unless method `isNumeric` returns `true`.
    bsls::Types::Int64 theNumeric() const;

    /// Return the pointer value of this object.  The behavior is undefined
    /// unless method `isPointer` returns `true`.
    void* thePointer() const;

    /// Return the pointer value of this object.  The behavior is undefined
    /// unless method `isSharedPtr` returns `true`.
    const bsl::shared_ptr<void>& theSharedPtr() const;

    /// Return the type of the value of this object.
    Type type() const;

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

// -------------------
// class CorrelationId
// -------------------

/// Format the specified `rhs` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const CorrelationId& rhs);

/// Return `true` if `rhs` object contains the value of the same type as
/// contained in `lhs` object and the value itself is the same in both
/// objects, return false otherwise.
bool operator==(const CorrelationId& lhs, const CorrelationId& rhs);

/// Return `false` if `rhs` object contains the value of the same type as
/// contained in `lhs` object and the value itself is the same in both
/// objects, return `true` otherwise.
bool operator!=(const CorrelationId& lhs, const CorrelationId& rhs);

/// Operator used to allow comparison between the specified `lhs` and `rhs`
/// CorrelationId objects so that CorrelationId can be used as key in a map.
bool operator<(const CorrelationId& lhs, const CorrelationId& rhs);

// =======================
// class CorrelationIdLess
// =======================

struct CorrelationIdLess {
    // ACCESSORS

    /// Return `true` if the specified `lhs` should be considered as having
    /// a value less than the specified `rhs`.
    bool operator()(const CorrelationId& lhs, const CorrelationId& rhs) const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------------
// class CorrelationId
// -------------------

inline CorrelationId::AutoValue CorrelationId::theAutoValue() const
{
    // PRECONDITIONS
    BSLS_ASSERT(isAutoValue());

    return d_variant.the<AutoValue>();
}

// CREATORS
inline CorrelationId::CorrelationId()
: d_variant()
{
    // NOTHING
}

inline CorrelationId::CorrelationId(bsls::Types::Int64 numeric)
: d_variant(numeric)
{
    // NOTHING
}

inline CorrelationId::CorrelationId(void* pointer)
: d_variant(pointer)
{
    // NOTHING
}

inline CorrelationId::CorrelationId(const bsl::shared_ptr<void>& sharedPtr)
: d_variant(sharedPtr)
{
    // NOTHING
}

// MANIPULATORS
inline CorrelationId& CorrelationId::makeUnset()
{
    d_variant.reset();
    return *this;
}

inline CorrelationId& CorrelationId::setNumeric(bsls::Types::Int64 numeric)
{
    d_variant.assign(numeric);
    return *this;
}

inline CorrelationId& CorrelationId::setPointer(void* pointer)
{
    d_variant.assign(pointer);
    return *this;
}

inline CorrelationId&
CorrelationId::setSharedPointer(const bsl::shared_ptr<void>& sharedPtr)
{
    d_variant.assign(sharedPtr);
    return *this;
}

// ACCESSORS
inline bool CorrelationId::isUnset() const
{
    return d_variant.isUnset();
}

inline bool CorrelationId::isNumeric() const
{
    return d_variant.is<bsls::Types::Int64>();
}

inline bool CorrelationId::isPointer() const
{
    return d_variant.is<void*>();
}

inline bool CorrelationId::isSharedPtr() const
{
    return d_variant.is<bsl::shared_ptr<void> >();
}

inline bool CorrelationId::isAutoValue() const
{
    return d_variant.is<AutoValue>();
}

inline bsls::Types::Int64 CorrelationId::theNumeric() const
{
    // PRECONDITIONS
    BSLS_ASSERT(isNumeric());

    return d_variant.the<bsls::Types::Int64>();
}

inline void* CorrelationId::thePointer() const
{
    // PRECONDITIONS
    BSLS_ASSERT(isPointer());

    return d_variant.the<void*>();
}

inline const bsl::shared_ptr<void>& CorrelationId::theSharedPtr() const
{
    // PRECONDITIONS
    BSLS_ASSERT(isSharedPtr());

    return d_variant.the<bsl::shared_ptr<void> >();
}

inline CorrelationId::Type CorrelationId::type() const
{
    int result = isUnset()       ? e_UNSET
                 : isNumeric()   ? e_NUMERIC
                 : isPointer()   ? e_POINTER
                 : isSharedPtr() ? e_SHARED_PTR
                 : isAutoValue() ? e_AUTO_VALUE
                                 : -1;
    BSLS_ASSERT_OPT(-1 != result && "Unknown correlation type");

    return static_cast<Type>(result);
}

// FREE FUNCTIONS
template <class HASH_ALGORITHM>
void hashAppend(HASH_ALGORITHM& hashAlgo, const CorrelationId& value)
{
    using bslh::hashAppend;  // for ADL

    CorrelationId::Type type = value.type();
    hashAppend(hashAlgo, static_cast<int>(type));

    switch (type) {
    case CorrelationId::e_NUMERIC: {
        hashAppend(hashAlgo, value.theNumeric());
    } break;
    case CorrelationId::e_POINTER: {
        hashAppend(hashAlgo, value.thePointer());
    } break;
    case CorrelationId::e_SHARED_PTR: {
        hashAppend(hashAlgo, value.theSharedPtr());
    } break;
    case CorrelationId::e_AUTO_VALUE: {
        hashAppend(hashAlgo, value.theAutoValue());
    } break;
    case CorrelationId::e_UNSET: {
        hashAppend(hashAlgo, 0);
    } break;
    default: {
        BSLS_ASSERT_OPT(false && "Unknown correlationId type");
        hashAppend(hashAlgo, 0);
    }
    }
}

// ------------------------
// struct CorrelationIdLess
// ------------------------

inline bool CorrelationIdLess::operator()(const CorrelationId& lhs,
                                          const CorrelationId& rhs) const
{
    // If 'lhs' and 'rhs' have different types, they are compared by their
    // corresponding type indices.  Otherwise, they are compared by value.

    bool result;
    if (lhs.d_variant.typeIndex() != rhs.d_variant.typeIndex()) {
        result = lhs.d_variant.typeIndex() < rhs.d_variant.typeIndex();
    }
    else if (lhs.isNumeric()) {
        result = lhs.theNumeric() < rhs.theNumeric();
    }
    else if (lhs.isPointer()) {
        result = reinterpret_cast<bsl::uintptr_t>(lhs.thePointer()) <
                 reinterpret_cast<bsl::uintptr_t>(rhs.thePointer());
    }
    else if (lhs.isSharedPtr()) {
        result = lhs.theSharedPtr() < rhs.theSharedPtr();
    }
    else if (lhs.isAutoValue()) {
        result = lhs.theAutoValue() < rhs.theAutoValue();
    }
    else {
        BSLS_ASSERT_OPT(false && "Unknown correlator type");
        result = false;
    }

    return result;
}

}  // close package namespace

// -------------------
// class CorrelationId
// -------------------

// FREE OPERATORS
inline bool bmqt::operator==(const bmqt::CorrelationId& lhs,
                             const bmqt::CorrelationId& rhs)
{
    return lhs.d_variant == rhs.d_variant;
}

inline bool bmqt::operator!=(const bmqt::CorrelationId& lhs,
                             const bmqt::CorrelationId& rhs)
{
    return lhs.d_variant != rhs.d_variant;
}

inline bool bmqt::operator<(const bmqt::CorrelationId& lhs,
                            const bmqt::CorrelationId& rhs)
{
    CorrelationIdLess less;
    return less(lhs, rhs);
}

inline bsl::ostream& bmqt::operator<<(bsl::ostream&              stream,
                                      const bmqt::CorrelationId& rhs)
{
    return rhs.print(stream, 0, -1);
}

}  // close enterprise namespace

#endif
