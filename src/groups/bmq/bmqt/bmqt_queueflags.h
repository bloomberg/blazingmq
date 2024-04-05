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

// bmqt_queueflags.h                                                  -*-C++-*-
#ifndef INCLUDED_BMQT_QUEUEFLAGS
#define INCLUDED_BMQT_QUEUEFLAGS

/// @file bmqt_queueflags.h
///
/// @brief Provide enumerators for flags to use at Queue open.
///
/// This file contains an enum, @bbref{bmqt::QueueFlags} of all the flags that
/// can be used at Queue open.  Each value of the enum correspond to a bit of a
/// bit-mask integer.  It also exposes a set of utilities, in the
/// @bbref{bmqt::QueueFlagsUtil} namespace to manipulate such bit-mask value.

// BMQ

// BDE
#include <bsl_iosfwd.h>
#include <bsl_string.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bmqt {

// =================
// struct QueueFlags
// =================

/// This enum represents queue flags
struct QueueFlags {
    // TYPES
    enum Enum {
        e_ADMIN = (1 << 0)  // The queue is opened in admin mode (Valid only
                            // for BlazingMQ admin tasks)
        ,
        e_READ = (1 << 1)  // The queue is opened for consuming messages
        ,
        e_WRITE = (1 << 2)  // The queue is opened for posting messages
        ,
        e_ACK = (1 << 3)  // Set to indicate interested in receiving
                          // 'ACK' events for all message posted
    };

    // PUBLIC CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that a
    /// QueueFlags field is a supported type.
    static const int k_LOWEST_SUPPORTED_QUEUE_FLAG = e_ADMIN;

    /// NOTE: This value must always be equal to the highest *supported*
    /// type in the enum because it is being used to verify a QueueFlags
    /// field is a supported type.
    static const int k_HIGHEST_SUPPORTED_QUEUE_FLAG = e_ACK;

    // CLASS METHODS

    /// Write the string representation of the specified enumeration `value`
    /// to the specified output `stream`, and return a reference to
    /// `stream`.  Optionally specify an initial indentation `level`, whose
    /// absolute value is incremented recursively for nested objects.  If
    /// `level` is specified, optionally specify `spacesPerLevel`, whose
    /// absolute value indicates the number of spaces per indentation level
    /// for this and all of its nested objects.  If `level` is negative,
    /// suppress indentation of the first line.  If `spacesPerLevel` is
    /// negative, format the entire output on one line, suppressing all but
    /// the initial indentation (as governed by `level`).  See `toAscii` for
    /// what constitutes the string representation of a
    /// `GenericResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&    stream,
                               QueueFlags::Enum value,
                               int              level          = 0,
                               int              spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(QueueFlags::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(QueueFlags::Enum* out, const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, QueueFlags::Enum value);

// =====================
// struct QueueFlagsUtil
// =====================

struct QueueFlagsUtil {
  public:
    // CLASS METHODS

    /// Return true if the bit-mask in the specified `flags` has the
    /// specified `flag` set, or false if not.
    ///
    /// DEPRECATED: This method is deprecated in favor of the below more
    ///             specific accessors; and should be made private once all
    ///             clients have been updated to the new APIs.
    static bool isSet(bsls::Types::Uint64 flags, QueueFlags::Enum flag);

  public:
    // CLASS METHODS

    /// Check whether the specified `flags` represent a valid combination of
    /// flags to use for opening a queue.  Return true if it does, or false
    /// if some exclusive flags are both set, and populate the specified
    /// `errorDescription` with the reason of the failure.
    static bool isValid(bsl::ostream&       errorDescription,
                        bsls::Types::Uint64 flags);

    /// The `empty` value for flags.
    static bsls::Types::Uint64 empty();

    /// Returns `true` if the specified `flags` have the `empty` value.
    static bool isEmpty(bsls::Types::Uint64 flags);

    /// Returns `true` if the specified `flags` represent a reader.
    static bool isReader(bsls::Types::Uint64 flags);

    /// Returns `true` if the specified `flags` represent a writer.
    static bool isWriter(bsls::Types::Uint64 flags);

    /// Returns `true` if the specified `flags` represent an admin.
    static bool isAdmin(bsls::Types::Uint64 flags);

    /// Returns `true` if the specified `flags` represent ack required.
    static bool isAck(bsls::Types::Uint64 flags);

    /// Sets the specified `flags` representation for reader.
    static void setReader(bsls::Types::Uint64* flags);

    /// Sets the specified `flags` representation for admin.
    static void setAdmin(bsls::Types::Uint64* flags);

    /// Sets the specified `flags` representation for writer.
    static void setWriter(bsls::Types::Uint64* flags);

    /// Sets the specified `flags` representation for ack.
    static void setAck(bsls::Types::Uint64* flags);

    /// Resets the specified `flags` representation for reader.
    static void unsetReader(bsls::Types::Uint64* flags);

    /// Resets the specified `flags` representation for admin.
    static void unsetAdmin(bsls::Types::Uint64* flags);

    /// Resets the specified `flags` representation for writer.
    static void unsetWriter(bsls::Types::Uint64* flags);

    /// Resets the specified `flags` representation for ack.
    static void unsetAck(bsls::Types::Uint64* flags);

    /// Return a new flag mask that only set bits are the ones that are in
    /// the specified `newFlags` and not in the specified `oldFlags`.
    static bsls::Types::Uint64 additions(bsls::Types::Uint64 oldFlags,
                                         bsls::Types::Uint64 newFlags);

    /// Return a new flag mask that only set bits are the ones that are in
    /// the specified `oldFlags` and not in the specified `newFlags`.
    static bsls::Types::Uint64 removals(bsls::Types::Uint64 oldFlags,
                                        bsls::Types::Uint64 newFlags);

    /// Print the ascii-representation of all the values set in the
    /// specified `flags` to the specified `stream`.  Each value is `,`
    /// separated.
    static bsl::ostream& prettyPrint(bsl::ostream&       stream,
                                     bsls::Types::Uint64 flags);

    /// Convert the string representation of the enum bit mask from the
    /// specified `str` (which format corresponds to the one of the
    /// `prettyPrint` method) and populate the specified `out` with the
    /// result on success returning 0, or return a non-zero error code on
    /// error, populating the specified `errorDescription` with a
    /// description of the error.
    static int fromString(bsl::ostream&        errorDescription,
                          bsls::Types::Uint64* out,
                          const bsl::string&   str);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------
// struct QueueFlagsUtil
// ---------------------

inline bool QueueFlagsUtil::isSet(bsls::Types::Uint64 flags,
                                  QueueFlags::Enum    flag)
{
    return ((flags & flag) != 0);
}

inline void QueueFlagsUtil::setReader(bsls::Types::Uint64* flags)
{
    *flags |= bmqt::QueueFlags::e_READ;
}

inline void QueueFlagsUtil::setAdmin(bsls::Types::Uint64* flags)
{
    *flags |= bmqt::QueueFlags::e_ADMIN;
}

inline void QueueFlagsUtil::setWriter(bsls::Types::Uint64* flags)
{
    *flags |= bmqt::QueueFlags::e_WRITE;
}

inline void QueueFlagsUtil::setAck(bsls::Types::Uint64* flags)
{
    *flags |= bmqt::QueueFlags::e_ACK;
}

inline void QueueFlagsUtil::unsetReader(bsls::Types::Uint64* flags)
{
    *flags &= ~bmqt::QueueFlags::e_READ;
}

inline void QueueFlagsUtil::unsetAdmin(bsls::Types::Uint64* flags)
{
    *flags &= ~bmqt::QueueFlags::e_ADMIN;
}

inline void QueueFlagsUtil::unsetWriter(bsls::Types::Uint64* flags)
{
    *flags &= ~bmqt::QueueFlags::e_WRITE;
}

inline void QueueFlagsUtil::unsetAck(bsls::Types::Uint64* flags)
{
    *flags &= ~bmqt::QueueFlags::e_ACK;
}

inline bool QueueFlagsUtil::isReader(bsls::Types::Uint64 flags)
{
    return isSet(flags, bmqt::QueueFlags::e_READ);
}

inline bool QueueFlagsUtil::isWriter(bsls::Types::Uint64 flags)
{
    return isSet(flags, bmqt::QueueFlags::e_WRITE);
}

inline bool QueueFlagsUtil::isAdmin(bsls::Types::Uint64 flags)
{
    return isSet(flags, bmqt::QueueFlags::e_ADMIN);
}

inline bool QueueFlagsUtil::isAck(bsls::Types::Uint64 flags)
{
    return isSet(flags, bmqt::QueueFlags::e_ACK);
}

inline bool QueueFlagsUtil::isEmpty(bsls::Types::Uint64 flags)
{
    return flags == empty();
}

inline bsls::Types::Uint64 QueueFlagsUtil::empty()
{
    return 0;
}

}  // close package namespace

// -----------------
// struct QueueFlags
// -----------------

inline bsl::ostream& bmqt::operator<<(bsl::ostream&          stream,
                                      bmqt::QueueFlags::Enum value)
{
    return bmqt::QueueFlags::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
