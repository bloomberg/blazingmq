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

// bmqt_version.h                                                     -*-C++-*-
#ifndef INCLUDED_BMQT_VERSION
#define INCLUDED_BMQT_VERSION

/// @file bmqt_version.h
///
/// @brief Provide a value-semantic type representing a version (major minor).
///
/// This component implements a simple value-semantic type,
/// @bbref{bmqt::Version} representing the version of an object.  It is used in
/// particular to attach a version attribute to a @bbref{bmqa::Message}, so
/// that a consuming application receiving a message knows the version of the
/// schema that was used for publishing.
///
/// A version is represented by two numbers: a major and a minor version.  Both
/// are positive integers within the range [0-255].
///
/// Usage Example                                           {#bmqt_version_ex1}
/// =============
///
/// ```
/// bmqt::Version version(1, 3);
/// BSLS_ASSERT(version.major() == 1);
/// BSLS_ASSERT(version.minor() == 3);
/// version.setMajor(2).setMinor(4);
/// BSLS_ASSERT(version.major() == 2);
/// BSLS_ASSERT(version.minor() == 4);
/// ```

// BMQ

// BDE
#include <bsl_iosfwd.h>

namespace BloombergLP {
namespace bmqt {

// =============
// class Version
// =============

/// A version consisting of a major and minor version number.
class Version {
  private:
    // DATA
    unsigned char d_major;  // Major part of the version
    unsigned char d_minor;  // Minor part of the version

  public:
    // CREATORS

    /// Create an object of type `Version` having the default value.  Note
    /// that major is set to zero and minor is set to zero.
    Version();

    /// Create an object of type `Version` initialized with the specified
    /// `major` and `minor` versions.
    Version(unsigned char major, unsigned char minor);

    // MANIPULATORS

    /// Set the major part of the version to the specified `value` and
    /// return a reference to this object.
    Version& setMajor(unsigned char value);

    /// Set the minor part of the version to the specified `value` and
    /// return a reference to this object.
    Version& setMinor(unsigned char value);

    // ACCESSORS

    /// Format this object to the specified output `stream` at the
    /// optionally specified indentation `level` and return a reference to
    /// the modifiable `stream`.  If `level` is specified, optionally
    /// specify `spacesPerLevel`, the number of spaces per indentation level
    /// for this and all of its nested objects.  Each line is indented by
    /// the absolute value of `level * spacesPerLevel`.  If `level` is
    /// negative, suppress indentation of the first line.  If
    /// `spacesPerLevel` is negative, suppress line breaks and format the
    /// entire output on one line.  If `stream` is initially invalid, this
    /// operation has no effect.  Note that a trailing newline is provided
    /// in multiline mode only.
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;

    /// Return the major part of the version.
    unsigned char major() const;

    /// Return the minor part of the version.
    unsigned char minor() const;
};

// FREE OPERATORS

/// Format the specified `rhs` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream& stream, const Version& rhs);

/// Return `true` if the object in the specified `lhs` represents the same
/// version as the one in the specified `rhs`, return false otherwise.
bool operator==(const Version& lhs, const Version& rhs);

/// Return `true` if the object in the specified `lhs` represents a
/// different version than the one in the specified `rhs`, return false
/// otherwise.
bool operator!=(const Version& lhs, const Version& rhs);

/// Return `true` if the version represented by the specified `lhs` is less
/// than the version represented by the specified `rhs`.
bool operator<(const Version& lhs, const Version& rhs);

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -------------
// class Version
// -------------

// CREATORS
inline Version::Version()
: d_major(0)
, d_minor(0)
{
    // NOTHING
}

inline Version::Version(unsigned char major, unsigned char minor)
: d_major(major)
, d_minor(minor)
{
    // NOTHING
}

// MANIPULATORS
inline Version& Version::setMajor(unsigned char value)
{
    d_major = value;
    return *this;
}

inline Version& Version::setMinor(unsigned char value)
{
    d_minor = value;
    return *this;
}

// ACCESSORS
inline unsigned char Version::major() const
{
    return d_major;
}

inline unsigned char Version::minor() const
{
    return d_minor;
}

}  // close package namespace

// FREE OPERATORS
inline bsl::ostream& bmqt::operator<<(bsl::ostream&        stream,
                                      const bmqt::Version& rhs)
{
    return rhs.print(stream, 0, -1);
}

inline bool bmqt::operator==(const bmqt::Version& lhs,
                             const bmqt::Version& rhs)
{
    return lhs.major() == rhs.major() && lhs.minor() == rhs.minor();
}

inline bool bmqt::operator!=(const bmqt::Version& lhs,
                             const bmqt::Version& rhs)
{
    return lhs.major() != rhs.major() || lhs.minor() != rhs.minor();
}

inline bool bmqt::operator<(const bmqt::Version& lhs, const bmqt::Version& rhs)
{
    return lhs.major() < rhs.major() ||
           (lhs.major() == rhs.major() && lhs.minor() < rhs.minor());
}

}  // close enterprise namespace

#endif
