// Copyright 2021-2023 Bloomberg Finance L.P.
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

// mqbs_filestoretestutil.h                                           -*-C++-*-
#ifndef INCLUDED_MQBS_FILESTORETESTUTIL
#define INCLUDED_MQBS_FILESTORETESTUTIL

//@PURPOSE: Provide files store util class used in tests.
//
//@CLASSES:
//  mqbs::FileStoreTestUtil_Record: Record type used in testing.
//
//@SEE ALSO: mqbs::FileStore
//
//@DESCRIPTION: 'mqbs::FileStoreTestUtil_Record' provides test Record.

// MQB
#include <mqbi_storage.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbu_storagekey.h>

// BDE
#include <bmqp_ctrlmsg_messages.h>
#include <bsl_memory.h>
#include <bsl_string.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {
namespace mqbs {

// ===============================
// struct FileStoreTestUtil_Record
// ===============================

struct FileStoreTestUtil_Record {
    // DATA
    bslma::Allocator* d_allocator_p;

    mqbs::RecordType::Enum d_recordType;

    mqbs::QueueOpType::Enum d_queueOpType;

    mqbs::DeletionRecordFlag::Enum d_deletionRecordFlag;

    mqbs::JournalOpType::Enum d_journalOpType;

    mqbs::SyncPointType::Enum d_syncPtType;

    bsl::string d_uri;

    mqbu::StorageKey d_queueKey;

    mqbi::StorageMessageAttributes d_msgAttributes;

    bmqt::MessageGUID d_guid;

    bsls::Types::Uint64 d_timestamp;

    bmqp_ctrlmsg::SyncPoint d_syncPoint;

    bsl::shared_ptr<bdlbb::Blob> d_appData_sp;

    bsl::shared_ptr<bdlbb::Blob> d_options_sp;

    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(FileStoreTestUtil_Record,
                                   bslma::UsesBslmaAllocator)

    // CREATORS
    FileStoreTestUtil_Record(bslma::Allocator* basicAllocator = 0)
    : d_allocator_p(bslma::Default::allocator(basicAllocator))
    , d_recordType(mqbs::RecordType::e_UNDEFINED)
    , d_queueOpType(mqbs::QueueOpType::e_UNDEFINED)
    , d_deletionRecordFlag(mqbs::DeletionRecordFlag::e_NONE)
    , d_journalOpType(mqbs::JournalOpType::e_UNDEFINED)
    , d_syncPtType(mqbs::SyncPointType::e_UNDEFINED)
    , d_uri(basicAllocator)
    , d_queueKey()
    , d_msgAttributes()
    , d_guid()
    , d_timestamp(0)
    , d_syncPoint()
    , d_appData_sp()
    , d_options_sp()
    {
    }

    FileStoreTestUtil_Record(const FileStoreTestUtil_Record& src,
                             bslma::Allocator* basicAllocator = 0)
    : d_allocator_p(bslma::Default::allocator(basicAllocator))
    , d_recordType(src.d_recordType)
    , d_queueOpType(src.d_queueOpType)
    , d_deletionRecordFlag(src.d_deletionRecordFlag)
    , d_journalOpType(src.d_journalOpType)
    , d_syncPtType(src.d_syncPtType)
    , d_uri(src.d_uri, basicAllocator)
    , d_queueKey(src.d_queueKey)
    , d_msgAttributes(src.d_msgAttributes)
    , d_guid(src.d_guid)
    , d_timestamp(src.d_timestamp)
    , d_syncPoint(src.d_syncPoint)
    , d_appData_sp(src.d_appData_sp)
    , d_options_sp(src.d_options_sp)
    {
    }

    // MANIPULATORS
    FileStoreTestUtil_Record& operator=(const FileStoreTestUtil_Record& rhs)
    {
        if (this == &rhs) {
            return *this;  // RETURN
        }

        d_recordType         = rhs.d_recordType;
        d_queueOpType        = rhs.d_queueOpType;
        d_deletionRecordFlag = rhs.d_deletionRecordFlag;
        d_journalOpType      = rhs.d_journalOpType;
        d_syncPtType         = rhs.d_syncPtType;
        d_uri                = rhs.d_uri;
        d_queueKey           = rhs.d_queueKey;
        d_msgAttributes      = rhs.d_msgAttributes;
        d_guid               = rhs.d_guid;
        d_timestamp          = rhs.d_timestamp;
        d_syncPtType         = rhs.d_syncPtType;
        d_appData_sp         = rhs.d_appData_sp;
        d_options_sp         = rhs.d_options_sp;

        return *this;
    }
};

// FREE OPERATORS

/// Return `true` if the specified `lhs` object contains the value of the
/// same type as contained in the specified `rhs` object and the value
/// itself is the same in both objects, return false otherwise.
bool operator==(const RecordHeader& lhs, const RecordHeader& rhs);

/// Return `false` if `rhs` object contains the value of the same type as
/// contained in `lhs` object and the value itself is the same in both
/// objects, return `true` otherwise.
bool operator!=(const RecordHeader& lhs, const RecordHeader& rhs);

}  // close package namespace

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// FREE OPERATORS
inline bool mqbs::operator==(const mqbs::RecordHeader& lhs,
                             const mqbs::RecordHeader& rhs)
{
    return lhs.type() == rhs.type() && lhs.flags() == rhs.flags() &&
           lhs.primaryLeaseId() == rhs.primaryLeaseId() &&
           lhs.sequenceNumber() == rhs.sequenceNumber() &&
           lhs.timestamp() == rhs.timestamp();
}

inline bool mqbs::operator!=(const mqbs::RecordHeader& lhs,
                             const mqbs::RecordHeader& rhs)
{
    return !(lhs == rhs);
}

}  // close enterprise namespace

#endif
