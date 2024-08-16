// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqp_optionutil.h                                                  -*-C++-*-
#ifndef INCLUDED_BMQP_OPTIONUTIL
#define INCLUDED_BMQP_OPTIONUTIL

//@PURPOSE: Provide utilities for builders and iterators that use options.
//
//@CLASSES:
//  bmqp::OptionUtil: Utilities for builders and iterators that use options.
//
//@DESCRIPTION: 'bmqp::OptionUtil' provides a set of utility methods to be
// used by the builders and iterators.

// BMQ

#include <bmqp_protocol.h>
#include <bmqt_resultcode.h>

// MWC
#include <mwcu_blob.h>

// BDE
#include <bdlbb_blob.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqp {

/// Utilities for builders and iterators that use options.
struct OptionUtil {
    // PUBLIC TYPES

    /// Class that holds metadata for an option
    class OptionMeta {
      private:
        // DATA
        bmqp::OptionType::Enum d_type;
        // The type for this option

        int d_payloadSize;
        // The total size of the payload
        // (excluding padding)

        int d_padding;
        // The total size of padding for an
        // option.  Could be '0' if no padding

        bool d_packed;
        // If set, the option will be "packed"
        // as a single option header with no
        // payload.  Instead of a payload, a
        // single type-specific value will be
        // included in the header (in the
        // "words" field)

        int d_packedValue;
        // If 'packed' is set, this field will
        // contain a single type-specific value
        // that will be populated into the
        // header

        unsigned char d_typeSpecific;
        // Type-specific value describing an
        // option's payload, only relevant if
        // 'packed' is false.

      public:
        // CLASS METHODS

        /// Return an `OptionMeta` for an option with the specified `type`,
        /// `size`, `packed`, `packedValue`, and `typeSpecific`.  It's
        /// undefined behavior if the `size` isn't a multiple of
        /// `Protocol::k_WORD_SIZE` when `packed` is false.
        static OptionMeta forOption(const bmqp::OptionType::Enum type,
                                    const int                    size,
                                    bool          packed       = false,
                                    int           packedValue  = 0,
                                    unsigned char typeSpecific = 0);

        /// Return an `OptionMeta` for an option with the specified
        /// `optionType` and `size` taking into account additional padding
        /// that will have to be added to make this option's size a multiple
        /// of `Protocol::k_WORD_SIZE`.  Behavior is undefined if `type` is
        /// `bmqp::OptionType::e_UNDEFINED`.
        static OptionMeta
        forOptionWithPadding(const bmqp::OptionType::Enum type,
                             const int                    size);

        /// This is for monadic completeness.  It is undefined behavior to
        /// use any method other than `isNull()` on this option.
        static OptionMeta forNullOption();

      private:
        // CREATORS

        /// Create an `OptionMeta` using the specified `type`, `payloadSize`
        /// `padding`, `packed`, `packedValue`, and `typeSpecific`.
        OptionMeta(const bmqp::OptionType::Enum type,
                   const int                    payloadSize,
                   const int                    padding,
                   bool                         packed       = false,
                   int                          packedValue  = 0,
                   unsigned char                typeSpecific = 0);

      public:
        // ACCESSORS

        /// Return the payload size (excluding optional padding)
        int payloadSize() const;

        /// Return the payload size (including optional padding)
        int payloadEffectiveSize() const;

        /// Return the total size of the option (including header)
        int size() const;

        /// Return the (optional) padding of this option
        int padding() const;

        /// Return the `OptionType` for this option.
        bmqp::OptionType::Enum type() const;

        /// Return `true` if this option is `null`.  If it's `null`, it's
        /// undefined behavior to use any other method on this.
        bool isNull() const;

        /// Return whether this option is packed.
        bool packed() const;

        /// Return the packed option's single value.  Note that the value is
        /// valid only if `packed` is set to true.
        int packedValue() const;

        /// Return the `typeSpecific` attribute value.
        unsigned char typeSpecific() const;
    };

    /// Class that enables easy addition of options on a message blob
    class OptionsBox {
      private:
        // DATA
        int d_optionsSize;
        // Size in bytes of the options area associated with the current
        // (to-be-packed) message.

        int d_optionsCount;
        // The count of distinct option blocks already added to the
        // current (to-be-packed) message.

      public:
        // CREATORS

        /// Create an empty `OptionsBox`
        OptionsBox();

        // MANIPULATORS

        /// Appends the specified `option` and `payload` to the specified
        /// `blob`.  The behavior is undefined unless `canAdd()` returns
        /// `true` for the `option` and the `currentSize` representing the
        /// size of data on the `blob`.
        void
        add(bdlbb::Blob* blob, const char* payload, const OptionMeta& option);

        /// Resets the size of the options area associated with the current
        /// (to-be-packed) message.
        void reset();

        // ACCESSORS

        /// Returns `bmqt::EventBuilderResult::e_SUCCESS` if the option
        /// described with the specified `option` can be successfully added
        /// to the current (to-be-packed) message, for which the specified
        /// `currentSize` bytes of data are expected to be added or have
        /// already been added.  Returns appropriate error code otherwise.
        /// `currentSize` is expected to be at least 4 i.e.  represent the
        /// fact that there will be at least something more than a header on
        /// the blob.  This isn't a functional requirement but just a sanity
        /// test.
        bmqt::EventBuilderResult::Enum canAdd(const int         currentSize,
                                              const OptionMeta& option) const;

        /// Return the total size of options area associated with the
        /// current (to-be-packed) message.
        int size() const;

        /// Return the count of distinct option blocks already added to the
        /// current (to-be-packed) message.
        int optionsCount() const;
    };

  private:
    // PRIVATE CLASS METHODS

    /// Set the specified `optionsSize` and `optionsPosition` to the
    /// appropriate values, so to describe the options segment of the
    /// message on the specified `blob`.  The header has the specified
    /// `headerWords` size and the options segment has the specified
    /// `optionsWords` size - both in words and both found in message's
    /// header.  The options segment starts at the specified `startPosition`
    static bool loadOptionsPosition(int*                      optionsSize,
                                    mwcu::BlobPosition*       optionsPosition,
                                    const bdlbb::Blob&        blob,
                                    const int                 headerWords,
                                    const int                 optionsWords,
                                    const mwcu::BlobPosition& startPosition);

  public:
    /// Returns `bmqt::EventBuilderResult::e_SUCCESS` if the specified
    /// `length` is valid for a Group Id or appropriate error code
    /// otherwise.
    static bmqt::EventBuilderResult::Enum
    isValidMsgGroupId(const Protocol::MsgGroupId& msgGroupId);

    /// Set the specified `optionsSize` and `optionsPosition` to the
    /// appropriate values, so to describe the options segment of the
    /// message on the specified `blob`.  The specified `header` is the
    /// `[Push/Put]Header` for the message and the specified
    /// `headerPosition` is the position within the `blob`.
    template <class HEADER>
    static bool loadOptionsPosition(int*                     optionsSize,
                                    mwcu::BlobPosition*      optionsPosition,
                                    const bdlbb::Blob&       blob,
                                    const mwcu::BlobPosition headerPosition,
                                    const HEADER&            header);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// -----------------------------
// struct OptionUtil::OptionMeta
// -----------------------------
// ACCESSORS
inline int OptionUtil::OptionMeta::payloadSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!isNull());

    return d_payloadSize;
}

inline int OptionUtil::OptionMeta::payloadEffectiveSize() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!isNull());

    return d_payloadSize + d_padding;
}

inline int OptionUtil::OptionMeta::size() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!isNull());

    return static_cast<int>(sizeof(OptionHeader)) + d_payloadSize + d_padding;
}

inline int OptionUtil::OptionMeta::padding() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!isNull());

    return d_padding;
}

inline bmqp::OptionType::Enum OptionUtil::OptionMeta::type() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(!isNull());

    return d_type;
}

inline bool OptionUtil::OptionMeta::isNull() const
{
    return (d_type == bmqp::OptionType::e_UNDEFINED);
}

inline bool OptionUtil::OptionMeta::packed() const
{
    return d_packed;
}

inline int OptionUtil::OptionMeta::packedValue() const
{
    return d_packedValue;
}

inline unsigned char OptionUtil::OptionMeta::typeSpecific() const
{
    return d_typeSpecific;
}

// -----------------------------
// struct OptionUtil::OptionsBox
// -----------------------------

inline void OptionUtil::OptionsBox::reset()
{
    d_optionsSize  = 0;
    d_optionsCount = 0;
}

inline int OptionUtil::OptionsBox::size() const
{
    return d_optionsSize;
}

inline int OptionUtil::OptionsBox::optionsCount() const
{
    return d_optionsCount;
}

template <class HEADER>
inline bool
OptionUtil::loadOptionsPosition(int*                     optionsSize,
                                mwcu::BlobPosition*      optionsPosition,
                                const bdlbb::Blob&       blob,
                                const mwcu::BlobPosition headerPosition,
                                const HEADER&            header)
{
    return loadOptionsPosition(optionsSize,
                               optionsPosition,
                               blob,
                               header.headerWords(),
                               header.optionsWords(),
                               headerPosition);
}

}  // close package namespace
}  // close enterprise namespace

#endif
