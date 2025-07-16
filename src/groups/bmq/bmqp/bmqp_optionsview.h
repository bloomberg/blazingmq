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

// bmqp_optionsview.h                                                 -*-C++-*-

#ifndef INCLUDED_BMQP_OPTIONSVIEW
#define INCLUDED_BMQP_OPTIONSVIEW

//@PURPOSE: Provide a mechanism to access options of a message.
//
//@CLASSES:
//  bmqp::OptionsView: read-only sequential view on options
//
//@DESCRIPTION: 'bmqp::OptionsView' is a mechanism providing read-only
// sequential access to options of messages of type
// 'PUT'/'PUSH'/'ACK'/'CONFIRM'.
//
/// Usage
///-----
// Typical usage of this view should follow the following pattern:
//..
//  int rc = 0;
//  if (optionsView.isValid()) {
//      if (   optionsView.find(bmqp::OptionType::e_SUB_QUEUE_INFOS)
//          != optionsView.end()) {
//          bmqp::Protocol::SubQueueInfosArray subQueueInfos;
//          rc = optionsView.loadSubQueueInfosOption(&subQueueInfos);
//          if (rc != 0) {
//              // Invalid SubQueueInfo option
//              BALL_LOG_ERROR_BLOCK {
//                  optionsView.dumpBlob(BALL_LOG_OUTPUT_STREAM);
//              }
//          }
//      }
//  }
//..

// BMQ

#include <bmqp_protocol.h>
#include <bmqu_blob.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlbb_blob.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_iterator.h>
#include <bsl_ostream.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_issame.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <bsls_assert.h>

namespace BloombergLP {

namespace bmqp {

// =================
// class OptionsView
// =================

/// A mechanism that provides read-only sequential access to options of
/// messages of type `PUT`/`PUSH`/`ACK`/`CONFIRM`.
class OptionsView {
  private:
    // PRIVATE TYPES

    /// Sequential stack allocator for the first template-specified bytes
    /// (falls back to other allocator afterwards)
    typedef bdlma::LocalSequentialAllocator<(OptionHeader::k_MAX_TYPE + 1) *
                                            sizeof(bmqu::BlobPosition)>
        LocalSequentialAllocator;

    /// Mapping of OptionType (index) to BlobPosition
    typedef bsl::vector<bmqu::BlobPosition> OptionPositions;

    /// Enables iteration on `bmqp::OptionsView`
    class Iterator {
      private:
        // DATA
        const OptionsView* d_optionsView_p;
        // Pointer to the source container
        unsigned int d_offset;
        // Current offset
        bmqp::OptionType::Enum d_value;
        // Copy of the current value

      public:
        // CREATORS

        /// Create an invalid instance
        Iterator();

        /// Copy-construct an instance from the specified `rhs` value
        Iterator(const Iterator& rhs);

        /// Create an iterator for the specified `optionsView` container
        /// pointing on the specified `offset`.
        Iterator(const OptionsView* optionsView, const unsigned int offset);

        // MANIPULATORS

        /// Assign with the value of the specified `rhs`
        Iterator& operator=(const Iterator& rhs);

        /// Advance by one
        void operator++();

        // ACCESSORS

        /// Get the OptionType for current offset
        const bmqp::OptionType::Enum& operator*() const;

        /// Return `true` if the specified `rhs` object contains the value
        /// of the same type as contained in `this` object and the value
        /// itself is the same in both objects, return `false` otherwise.
        bool operator==(const Iterator& rhs) const;
    };

  public:
    // PUBLIC TYPES

    /// Declares that `OptionsView` provides a const_iterator
    typedef bslstl::ForwardIterator<const bmqp::OptionType::Enum, Iterator>
        const_iterator;

    // FRIENDS
    friend class Iterator;
    // To be able to access 'advance()'

  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    // Pointer to allocator

    LocalSequentialAllocator d_bufferAllocator;
    // Sequential stack allocator for
    // BlobPositions of possibly every type
    // (at most OptionHeader::k_MAX_TYPE +
    // 1 of them)

    OptionPositions d_optionPositions;
    // Mapping of OptionType to
    // BlobPosition.  Note that the size of
    // this instance must always be
    // OptionHeader::k_MAX_TYPE + 1

    const bdlbb::Blob* d_blob_p;  // Pointer to a blob holding the
                                  // options (if any).

    bool d_isValid;
    // Flag indicating if this view is
    // initialized and valid.

    const bmqu::BlobPosition d_NullBlobPosition;
    // Value indicating no blob position
    // (member just to avoid repeated
    // instantiations elsewhere)

  private:
    // PRIVATE MANIPULATORS

    /// Reset this instance using the specified `blob`, `optionsAreaPos`,
    /// and `optionsAreaSize`, without first clearing the instance.  An
    /// attempt will be made to detect invalid option headers in the options
    /// area pointed to by this blob, and if detected then the instance
    /// becomes invalid.  Behavior is undefined if the `blob` pointer is
    /// null, or `optionsAreaSize` is negative, or if `blob` does not have
    /// `optionsAreaSize` bytes from `optionsAreaPos` offset.  Return 0 on
    /// success, and non-zero on error.
    int resetImpl(const bdlbb::Blob*        blob,
                  const bmqu::BlobPosition& optionsAreaPos,
                  int                       optionsAreaSize);

    // PRIVATE ACCESSORS

    /// Return an iterator pointing to the next available
    /// `bmqp::OptionType`, starting after the optionally specified
    /// `initialOffset`.
    const_iterator advance(const int initialOffset = -1) const;

    /// Load the specified `payloadPosition` and `payloadSizeBytes` with the
    /// position and the size respectively of the specified `optionType`
    /// inside the blob.  Return 0 on success or a non-zero value if the
    /// range for `optionType` doesn't fall within the blob.  If the
    /// specified `hasPadding` is set, then `payloadSizeBytes` takes into
    /// account (and excludes) the padding this option has.  Behavior is
    /// undefined unless `isValid()` returns `true` and `find(optionType)`
    /// returns a valid iterator.  Note that if the option blob is *packed*,
    /// `payloadSizeBytes` will be loaded as zero.
    int loadOptionPositionAndSize(bmqu::BlobPosition* payloadPosition,
                                  int*                payloadSizeBytes,
                                  const bmqp::OptionType::Enum optionType,
                                  const bool hasPadding) const;

    /// Load into the specified `out` the sub-queue infos associated with
    /// the options pointed to by this view stored using the specified
    /// `optionType`.  Each item in the sub-queue infos must have the
    /// specified `itemSize`.  Return zero on success, and a non-zero value
    /// otherwise.  Behavior is undefined unless `out` is empty, `isValid()`
    /// returns true and `optionType` is either e_SUB_QUEUE_INFOS or
    /// e_SUB_QUEUE_IDS_OLD.
    template <typename SUB_QUEUE_INFO>
    int loadSubQueueInfosOptionHelper(bsl::vector<SUB_QUEUE_INFO>* out,
                                      int                          itemSize,
                                      OptionType::Enum optionType) const;

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(OptionsView, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create an invalid instance using the specified `allocator`.  Only
    /// valid operations on an invalid instance are, `reset` and `isValid`.
    explicit OptionsView(bslma::Allocator* allocator);

    /// Copy constructor from the specified `src` using the specified
    /// `allocator`.  Needed because LocalSequentialAllocator does not
    /// authorize copy semantic.
    OptionsView(const OptionsView& src, bslma::Allocator* allocator);

    /// Initialize a new instance using the specified `blob`,
    /// `optionsAreaPos`, `optionsAreaSize`, and `allocator`.  An attempt
    /// will be made to detect invalid option headers in the options area
    /// pointed to by this blob, and if detected  then the instance becomes
    /// invalid.  Behavior is undefined if the `blob` pointer is null, or
    /// `optionsAreaSize` is negative, of if `blob` does not have
    /// `optionsAreaSize` bytes from `optionsAreaPos` offset.  Return 0 on
    /// success, and non-zero on error.
    OptionsView(const bdlbb::Blob*        blob,
                const bmqu::BlobPosition& optionsAreaPos,
                int                       optionsAreaSize,
                bslma::Allocator*         allocator);

    // MANIPULATORS

    /// Reset this instance using the specified `blob`, `optionsAreaPos`,
    /// and `optionsAreaSize`.  An attempt will be made to detect invalid
    /// option headers in the options area pointed to by this blob, and if
    /// detected then the instance becomes invalid.  Behavior is undefined
    /// if the `blob` pointer is null, or `optionsAreaSize` is negative, or
    /// if `blob` does not have `optionsAreaSize` bytes from
    /// `optionsAreaPos` offset.  Return 0 on success, and non-zero on
    /// error.
    int reset(const bdlbb::Blob*        blob,
              const bmqu::BlobPosition& optionsAreaPos,
              int                       optionsAreaSize);

    /// Set the internal state of this instance to be same as default
    /// constructed, i.e., invalid.
    void clear();

    /// Dump the beginning of the blob associated to this OptionsView to the
    /// specified `stream`.
    void dumpBlob(bsl::ostream& stream);

    // ACCESSORS

    /// Return true if this view is initialized and valid (and `find()` and
    /// the various `load(...)` methods can be called on this instance), or
    /// return false in all other cases.
    bool isValid() const;

    /// Return true if the options pointed to by this view have an
    /// OptionHeader with the specified `optionType` , and false otherwise.
    /// Behavior is undefined unless `isValid` returns `true`.
    const_iterator find(bmqp::OptionType::Enum optionType) const;

    /// Load into the specified `subQueueIdsOld` the sub-queue ids
    /// associated with the options pointed to by this view.  Return zero on
    /// success, and a non-zero value otherwise.  Behavior is undefined
    /// unless `subQueueIdsOld` is empty, `isValid()` returns true and
    /// `find(bmqp::OptionType::Enum::e_SUB_QUEUE_IDS_OLD)` returns a valid
    /// iterator.
    int
    loadSubQueueIdsOption(Protocol::SubQueueIdsArrayOld* subQueueIdsOld) const;

    /// Load into the specified `subQueueInfos` the sub-queue infos
    /// associated with the options pointed to by this view.  Return zero on
    /// success, and a non-zero value otherwise.  Behavior is undefined
    /// unless `subQueueInfos` is empty and `isValid()` returns true.  If
    /// `find(bmqp::OptionType::Enum::e_SUB_QUEUE_INFOS)` returns a valid
    /// iterator, the sub-queue infos will be loaded as is.  Else, if
    /// `find(bmqp::OptionType::Enum::e_SUB_QUEUE_IDS_OLD)` returns a valid
    /// iterator, the sub-queue ids will be loaded as is but the rest of the
    /// infos will be set to zeros.  Else, behavior is undefined.
    int
    loadSubQueueInfosOption(Protocol::SubQueueInfosArray* subQueueInfos) const;

    /// Load into the specified `msgGroupId` the Group Id associated with
    /// the options pointed to by this view.  Return zero on success, and a
    /// non-zero value otherwise.  Behavior is undefined unless `isValid()`
    /// returns `true`, `find(bmqp::OptionType::Enum::e_MSG_GROUP_ID)`
    /// returns a valid iterator and `msgGroupId` is a pointer to a valid
    /// empty `Protocol::MsgGroupId`.
    int loadMsgGroupIdOption(Protocol::MsgGroupId* msgGroupId) const;

    /// Return an iterator pointing to the beginning of the available
    /// `bmqp::OptionType::Enum` for this container
    const_iterator begin() const;

    /// Return an iterator pointing past the end of the available
    /// `bmqp::OptionType::Enum` for this container
    const_iterator end() const;
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// ---------------------------
// class OptionsView::Iterator
// ---------------------------

// CREATORS
inline OptionsView::Iterator::Iterator()
: d_optionsView_p(0)
, d_offset(0)
, d_value(OptionType::e_UNDEFINED)
{
}

inline OptionsView::Iterator::Iterator(const Iterator& rhs)
: d_optionsView_p(rhs.d_optionsView_p)
, d_offset(rhs.d_offset)
, d_value(rhs.d_value)
{
}

inline OptionsView::Iterator::Iterator(const OptionsView* optionsView,
                                       const unsigned int offset)
: d_optionsView_p(optionsView)
, d_offset(offset)
, d_value(static_cast<bmqp::OptionType::Enum>(d_offset))
{
}

inline OptionsView::Iterator&
OptionsView::Iterator::operator=(const Iterator& rhs)
{
    if (&rhs != this) {
        d_optionsView_p = rhs.d_optionsView_p;
        d_offset        = rhs.d_offset;
        d_value         = rhs.d_value;
    }
    return *this;
}

// MANIPULATORS
inline void OptionsView::Iterator::operator++()
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_optionsView_p);

    *this = d_optionsView_p->advance(d_offset).imp();
}

// ACCESSORS
inline const bmqp::OptionType::Enum& OptionsView::Iterator::operator*() const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_optionsView_p);

    return d_value;
}

inline bool
OptionsView::Iterator::operator==(const OptionsView::Iterator& rhs) const
{
    // This expression is ok even for the case when an iterator is invalid.
    return (d_optionsView_p == rhs.d_optionsView_p) &&
           (d_offset == rhs.d_offset) && (d_value == rhs.d_value);
}

// -----------------
// class OptionsView
// -----------------

// PRIVATE ACCESSORS
inline OptionsView::const_iterator
OptionsView::advance(const int initialOffset) const
{
    const int startLookingAt = initialOffset + 1;
    BSLS_ASSERT_SAFE(startLookingAt >= 0);
    for (unsigned int i = static_cast<unsigned int>(startLookingAt);
         i < d_optionPositions.size();
         ++i) {
        if (d_optionPositions[i] != d_NullBlobPosition) {
            return Iterator(this, i);  // RETURN
        }
    }
    return const_iterator();
}

template <typename SUB_QUEUE_INFO>
int OptionsView::loadSubQueueInfosOptionHelper(
    bsl::vector<SUB_QUEUE_INFO>* out,
    int                          itemSize,
    OptionType::Enum             optionType) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(out && out->empty());
    BSLS_ASSERT_SAFE(isValid());
    BSLS_ASSERT_SAFE(
        (optionType == OptionType::e_SUB_QUEUE_INFOS &&
         bsl::is_same<SUB_QUEUE_INFO, SubQueueInfo>::value) ||
        (optionType == OptionType::e_SUB_QUEUE_IDS_OLD &&
         bsl::is_same<SUB_QUEUE_INFO, bdlb::BigEndianUint32>::value));

    enum RcEnum {
        // Value for the various RC error categories
        rc_SUCCESS      = 0,
        rc_INVALID_BLOB = -1
    };

    bmqu::BlobPosition firstSubQueueInfoPos;
    int                subQueueInfosSize;

    int rc = loadOptionPositionAndSize(&firstSubQueueInfoPos,
                                       &subQueueInfosSize,
                                       optionType,
                                       false);  // hasPadding
    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // Will happen if the range
        // [headerPos, headerPos + sizeof(OptionHeader)] does not fall within
        // the blob.
        return rc_INVALID_BLOB;  // RETURN
    }

    const int numSubQueueInfos = subQueueInfosSize / itemSize;
    out->resize(numSubQueueInfos);

    // Read option payload
    rc = bmqu::BlobUtil::readNBytes(reinterpret_cast<char*>(out->data()),
                                    *d_blob_p,
                                    firstSubQueueInfoPos,
                                    subQueueInfosSize);

    if (BSLS_PERFORMANCEHINT_PREDICT_UNLIKELY(rc != 0)) {
        BSLS_PERFORMANCEHINT_UNLIKELY_HINT;
        // The range
        // [firstSubQueueInfoPos, firstSubQueueInfoPos + subQueueInfosSize]
        // doesn't fall within the blob.
        return rc_INVALID_BLOB;  // RETURN
    }

    return rc_SUCCESS;
}

// CREATORS
inline OptionsView::OptionsView(bslma::Allocator* allocator)
: d_allocator_p(allocator)
, d_bufferAllocator(allocator)
, d_optionPositions(OptionHeader::k_MAX_TYPE + 1, &d_bufferAllocator)
, d_blob_p(0)
, d_isValid(false)
, d_NullBlobPosition(bmqu::BlobPosition(-1, 0))
{
    // NOTHING
}

inline OptionsView::OptionsView(const OptionsView& src,
                                bslma::Allocator*  allocator)
: d_allocator_p(allocator)
, d_bufferAllocator(allocator)
, d_optionPositions(src.d_optionPositions, &d_bufferAllocator)
, d_blob_p(src.d_blob_p)
, d_isValid(src.d_isValid)
, d_NullBlobPosition(bmqu::BlobPosition(-1, 0))
{
    // NOTHING
}

inline OptionsView::OptionsView(const bdlbb::Blob*        blob,
                                const bmqu::BlobPosition& optionsAreaPos,
                                int                       optionsAreaSize,
                                bslma::Allocator*         allocator)
: d_allocator_p(allocator)
, d_bufferAllocator(allocator)
, d_optionPositions(OptionHeader::k_MAX_TYPE + 1,
                    bmqu::BlobPosition(-1, 0),
                    &d_bufferAllocator)
, d_blob_p(blob)
, d_isValid(false)
, d_NullBlobPosition(bmqu::BlobPosition(-1, 0))
{
    resetImpl(blob, optionsAreaPos, optionsAreaSize);
}

// MANIPULATORS
inline int OptionsView::reset(const bdlbb::Blob*        blob,
                              const bmqu::BlobPosition& optionsAreaPos,
                              int                       optionsAreaSize)
{
    clear();
    return resetImpl(blob, optionsAreaPos, optionsAreaSize);
}

inline void OptionsView::clear()
{
    // We already allocated the maximum memory we are going to need at
    // construction, so we simply set each BlobPosition to "null"
    for (unsigned int i = 0; i < d_optionPositions.size(); ++i) {
        d_optionPositions[i] = d_NullBlobPosition;
    }

    d_blob_p  = 0;
    d_isValid = false;
}

// ACCESSORS
inline bool OptionsView::isValid() const
{
    return d_isValid;
}

inline OptionsView::const_iterator OptionsView::begin() const
{
    return advance();
}

inline OptionsView::const_iterator OptionsView::end() const
{
    return const_iterator();
}

inline OptionsView::const_iterator
OptionsView::find(bmqp::OptionType::Enum optionType) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isValid());

    return d_optionPositions[optionType] == d_NullBlobPosition
               ? end()
               : Iterator(this, optionType);
}

}  // close package namespace
}  // close enterprise namespace

#endif
