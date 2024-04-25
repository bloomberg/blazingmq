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

// bmqt_resultcode.h                                                  -*-C++-*-
#ifndef INCLUDED_BMQT_RESULTCODE
#define INCLUDED_BMQT_RESULTCODE

/// @file bmqt_resultcode.h
///
/// @brief Provide enums for various publicly exposed result code.
///
/// This file contains a list of Enums (@bbref{bmqt::GenericResult},
/// @bbref{bmqt::AckResult}, @bbref{bmqt::CloseQueueResult},
/// @bbref{bmqt::EventBuilderResult}, @bbref{bmqt::OpenQueueResult},
/// @bbref{bmqt::ConfigureQueueResult}, and @bbref{bmqt::PostResult}) that are
/// publicly exposed to Application (via `bmqa`), but whose value comes from
/// the internal implementation (`bmqimp`).  Having them defined in `bmqt`
/// allows `bmqa` to return the enum returned by `bmqimp` with no
/// transformation.
///
/// All enums are using the convention that < 0 values are errors, while > 0
/// are warnings.
///
/// The @bbref{bmqt::GenericResult} enum contains values that are common for
/// most or all of the other status enums, such as `success`, `timeout`, ...
/// The values of its members can range from -99 to 99.
///
/// Each other enum should duplicate any values from the
/// @bbref{bmqt::GenericResult} one that it intends to be able to represent
/// (aliasing the values to the ones from the @bbref{bmqt::GenericResult} enum)
/// and extend with specialized values in the `]..., -99[` or `]99, ...[`
/// ranges.

// BMQ

// BDE
#include <bsl_iosfwd.h>
#include <bsl_string.h>

namespace BloombergLP {
namespace bmqt {

// ====================
// struct GenericResult
// ====================

/// This enum represents generic common status
struct GenericResult {
    // TYPES
    enum Enum {
        e_SUCCESS = 0  // Operation was success
        ,
        e_UNKNOWN = -1  // Operation failed for unknown reason
        ,
        e_TIMEOUT = -2  // Operation timedout
        ,
        e_NOT_CONNECTED = -3  // Cant process, not connected to the broker
        ,
        e_CANCELED = -4  // Operation was canceled
        ,
        e_NOT_SUPPORTED = -5  // Operation is not supported
        ,
        e_REFUSED = -6  // Operation was refused
        ,
        e_INVALID_ARGUMENT = -7  // An invalid argument was provided
        ,
        e_NOT_READY = -8  // Not ready to process the request
        ,
        e_LAST = e_NOT_READY
        // Used in test driver only, to validate
        // consistency between this enum and the
        // 'bmqp_ctrlmsg.xsd::StatusCategory' one
    };

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
    static bsl::ostream& print(bsl::ostream&       stream,
                               GenericResult::Enum value,
                               int                 level          = 0,
                               int                 spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(GenericResult::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(GenericResult::Enum*     out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, GenericResult::Enum value);

// ======================
// struct OpenQueueResult
// ======================

/// This enum represents the result of an openQueue operation
struct OpenQueueResult {
    // TYPES
    enum Enum {
        // GENERIC
        e_SUCCESS          = GenericResult::e_SUCCESS,
        e_UNKNOWN          = GenericResult::e_UNKNOWN,
        e_TIMEOUT          = GenericResult::e_TIMEOUT,
        e_NOT_CONNECTED    = GenericResult::e_NOT_CONNECTED,
        e_CANCELED         = GenericResult::e_CANCELED,
        e_NOT_SUPPORTED    = GenericResult::e_NOT_SUPPORTED,
        e_REFUSED          = GenericResult::e_REFUSED,
        e_INVALID_ARGUMENT = GenericResult::e_INVALID_ARGUMENT,
        e_NOT_READY        = GenericResult::e_NOT_READY

        // SPECIALIZED
        // WARNINGS
        ,
        e_ALREADY_OPENED = 100  // The queue is already opened
        ,
        e_ALREADY_IN_PROGRESS = 101  // The queue is already being opened

        // ERRORS
        ,
        e_INVALID_URI = -100  // The queue uri is invalid
        ,
        e_INVALID_FLAGS = -101  // The flags provided are invalid
        ,
        e_CORRELATIONID_NOT_UNIQUE = -102  // The correlationdId is not unique
    };

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
    /// `OpenQueueResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&         stream,
                               OpenQueueResult::Enum value,
                               int                   level          = 0,
                               int                   spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(OpenQueueResult::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(OpenQueueResult::Enum*   out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, OpenQueueResult::Enum value);

// ===========================
// struct ConfigureQueueResult
// ===========================

/// This enum represents the result of a configureQueue operation
struct ConfigureQueueResult {
    // TYPES
    enum Enum {
        // GENERIC
        e_SUCCESS          = GenericResult::e_SUCCESS,
        e_UNKNOWN          = GenericResult::e_UNKNOWN,
        e_TIMEOUT          = GenericResult::e_TIMEOUT,
        e_NOT_CONNECTED    = GenericResult::e_NOT_CONNECTED,
        e_CANCELED         = GenericResult::e_CANCELED,
        e_NOT_SUPPORTED    = GenericResult::e_NOT_SUPPORTED,
        e_REFUSED          = GenericResult::e_REFUSED,
        e_INVALID_ARGUMENT = GenericResult::e_INVALID_ARGUMENT,
        e_NOT_READY        = GenericResult::e_NOT_READY

        // SPECIALIZED WARNINGS
        ,
        e_ALREADY_IN_PROGRESS = 100  // The queue is already being configured

        // ERRORS
        ,
        e_INVALID_QUEUE = -101
    };

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
    /// `ConfigureQueueResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&              stream,
                               ConfigureQueueResult::Enum value,
                               int                        level          = 0,
                               int                        spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(ConfigureQueueResult::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(ConfigureQueueResult::Enum* out,
                          const bslstl::StringRef&    str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&              stream,
                         ConfigureQueueResult::Enum value);

// =======================
// struct CloseQueueResult
// =======================

/// This enum represents the result of a closeQueue operation
struct CloseQueueResult {
    // TYPES
    enum Enum {
        // GENERIC
        e_SUCCESS          = GenericResult::e_SUCCESS,
        e_UNKNOWN          = GenericResult::e_UNKNOWN,
        e_TIMEOUT          = GenericResult::e_TIMEOUT,
        e_NOT_CONNECTED    = GenericResult::e_NOT_CONNECTED,
        e_CANCELED         = GenericResult::e_CANCELED,
        e_NOT_SUPPORTED    = GenericResult::e_NOT_SUPPORTED,
        e_REFUSED          = GenericResult::e_REFUSED,
        e_INVALID_ARGUMENT = GenericResult::e_INVALID_ARGUMENT,
        e_NOT_READY        = GenericResult::e_NOT_READY

        // SPECIALIZED
        // WARNINGS
        ,
        e_ALREADY_CLOSED = 100  // The queue is already closed
        ,
        e_ALREADY_IN_PROGRESS = 101  // The queue is already being closed

        // ERRORS
        ,
        e_UNKNOWN_QUEUE = -100  // The queue doesn't exist
        ,
        e_INVALID_QUEUE = -101  // The queue provided is invalid
    };

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
    /// `PublishResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&          stream,
                               CloseQueueResult::Enum value,
                               int                    level          = 0,
                               int                    spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(CloseQueueResult::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(CloseQueueResult::Enum*  out,
                          const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, CloseQueueResult::Enum value);

// =========================
// struct EventBuilderResult
// =========================

/// This enum represents the result of an EventBuilder's packMessage()
/// operation
struct EventBuilderResult {
    // TYPES
    enum Enum {
        // GENERIC
        e_SUCCESS = GenericResult::e_SUCCESS,
        e_UNKNOWN = GenericResult::e_UNKNOWN

        // SPECIALIZED
        // ERRORS
        ,
        e_QUEUE_INVALID          = -100,
        e_QUEUE_READONLY         = -101,
        e_MISSING_CORRELATION_ID = -102,
        e_EVENT_TOO_BIG          = -103,
        e_PAYLOAD_TOO_BIG        = -104,
        e_PAYLOAD_EMPTY          = -105,
        e_OPTION_TOO_BIG         = -106
#ifdef BMQ_ENABLE_MSG_GROUPID
        ,
        e_INVALID_MSG_GROUP_ID = -107
#endif
        ,
        e_QUEUE_SUSPENDED = -108
    };

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
    /// `EventBuilderResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&            stream,
                               EventBuilderResult::Enum value,
                               int                      level          = 0,
                               int                      spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(EventBuilderResult::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(EventBuilderResult::Enum* out,
                          const bslstl::StringRef&  str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, EventBuilderResult::Enum value);

// ================
// struct AckResult
// ================

/// This enum represents the result code status of an ack message
struct AckResult {
    // TYPES
    enum Enum {
        // GENERIC
        e_SUCCESS          = GenericResult::e_SUCCESS,
        e_UNKNOWN          = GenericResult::e_UNKNOWN,
        e_TIMEOUT          = GenericResult::e_TIMEOUT,
        e_NOT_CONNECTED    = GenericResult::e_NOT_CONNECTED,
        e_CANCELED         = GenericResult::e_CANCELED,
        e_NOT_SUPPORTED    = GenericResult::e_NOT_SUPPORTED,
        e_REFUSED          = GenericResult::e_REFUSED,
        e_INVALID_ARGUMENT = GenericResult::e_INVALID_ARGUMENT,
        e_NOT_READY        = GenericResult::e_NOT_READY

        // SPECIALIZED
        // ERRORS
        ,
        e_LIMIT_MESSAGES = -100  // Messages limit reached
        ,
        e_LIMIT_BYTES = -101  // Bytes limit reached

        // TBD:DEPRECATED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        // >libbmq-1.3.5
        // The below 4 values are deprecated in favor of the above two ones
        ,
        e_LIMIT_DOMAIN_MESSAGES = -100  // The domain is full (messages)
        ,
        e_LIMIT_DOMAIN_BYTES = -101  // The domain is full (bytes)
        ,
        e_LIMIT_QUEUE_MESSAGES = -102  // The queue is full (messages)
        ,
        e_LIMIT_QUEUE_BYTES = -103  // The queue is full (bytes)
        // TBD:DEPRECATED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        // >libbmq-1.3.5
        ,
        e_STORAGE_FAILURE = -104  // The storage (on disk) is full
    };

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
    /// `PublishResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&   stream,
                               AckResult::Enum value,
                               int             level          = 0,
                               int             spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `e_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(AckResult::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(AckResult::Enum* out, const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, AckResult::Enum value);

// =================
// struct PostResult
// =================

/// This enum represents the result code status of a post message
struct PostResult {
    // TYPES
    enum Enum {
        // GENERIC
        e_SUCCESS          = GenericResult::e_SUCCESS,
        e_UNKNOWN          = GenericResult::e_UNKNOWN,
        e_TIMEOUT          = GenericResult::e_TIMEOUT,
        e_NOT_CONNECTED    = GenericResult::e_NOT_CONNECTED,
        e_CANCELED         = GenericResult::e_CANCELED,
        e_NOT_SUPPORTED    = GenericResult::e_NOT_SUPPORTED,
        e_REFUSED          = GenericResult::e_REFUSED,
        e_INVALID_ARGUMENT = GenericResult::e_INVALID_ARGUMENT,
        e_NOT_READY        = GenericResult::e_NOT_READY

        // SPECIALIZED
        // WARNINGS
        ,
        e_BW_LIMIT = 100  // The application has been posting too much
                          // data, and the IO or broker are temporarily
                          // rejecting new messages.
    };

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
    /// `PublishResult::Enum` value.
    static bsl::ostream& print(bsl::ostream&    stream,
                               PostResult::Enum value,
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
    static const char* toAscii(PostResult::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(PostResult::Enum* out, const bslstl::StringRef& str);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, PostResult::Enum value);

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// struct GenericResult
// --------------------

inline bsl::ostream& bmqt::operator<<(bsl::ostream&             stream,
                                      bmqt::GenericResult::Enum value)
{
    return bmqt::GenericResult::print(stream, value, 0, -1);
}

// ----------------------
// struct OpenQueueResult
// ----------------------

inline bsl::ostream& bmqt::operator<<(bsl::ostream&               stream,
                                      bmqt::OpenQueueResult::Enum value)
{
    return bmqt::OpenQueueResult::print(stream, value, 0, -1);
}

// ---------------------------
// struct ConfigureQueueResult
// ---------------------------

inline bsl::ostream& bmqt::operator<<(bsl::ostream&                    stream,
                                      bmqt::ConfigureQueueResult::Enum value)
{
    return bmqt::ConfigureQueueResult::print(stream, value, 0, -1);
}

// -----------------------
// struct CloseQueueResult
// -----------------------

inline bsl::ostream& bmqt::operator<<(bsl::ostream&                stream,
                                      bmqt::CloseQueueResult::Enum value)
{
    return bmqt::CloseQueueResult::print(stream, value, 0, -1);
}

// -------------------------
// struct EventBuilderResult
// -------------------------

inline bsl::ostream& bmqt::operator<<(bsl::ostream&                  stream,
                                      bmqt::EventBuilderResult::Enum value)
{
    return bmqt::EventBuilderResult::print(stream, value, 0, -1);
}

// ----------------
// struct AckResult
// ----------------

inline bsl::ostream& bmqt::operator<<(bsl::ostream&         stream,
                                      bmqt::AckResult::Enum value)
{
    return bmqt::AckResult::print(stream, value, 0, -1);
}

// -----------------
// struct PostResult
// -----------------

inline bsl::ostream& bmqt::operator<<(bsl::ostream&          stream,
                                      bmqt::PostResult::Enum value)
{
    return bmqt::PostResult::print(stream, value, 0, -1);
}

}  // close enterprise namespace

#endif
