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

#ifndef INCLUDED_M_BMQSTORAGETOOL_RECORDPRINTER
#define INCLUDED_M_BMQSTORAGETOOL_RECORDPRINTER

//@PURPOSE: Provide utilities for printing journal file records.
//
//@CLASSES:
//
//@DESCRIPTION: 'm_bmqstoragetool::RecordPrinter' namespace provides methods
// for printing journal file records details. These methods are enhanced
// versions of
// mqbs::FileStoreProtocolPrinter::printRecord() methods.

// BDE
#include <bslma_allocator.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =======================
// namespace RecordPrinter
// =======================

namespace RecordPrinter {

// FREE FUNCTIONS

/// Print the specified message record `rec` and QueueInfo pointed by the
/// specified `queueInfo_p` to the specified `stream`, using the specified
/// `allocator` for memory allocation.
void printRecord(bsl::ostream&                  stream,
                 const mqbs::MessageRecord&     rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                 bslma::Allocator*              allocator);

/// Print the specified confirm record `rec` and QueueInfo pointed by the
/// specified `queueInfo_p` to the specified `stream`, using the specified
/// `allocator` for memory allocation.
void printRecord(bsl::ostream&                  stream,
                 const mqbs::ConfirmRecord&     rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                 bslma::Allocator*              allocator);

/// Print the specified delete record `rec` and QueueInfo pointed by the
/// specified `queueInfo_p` to the specified `stream`, using the specified
/// `allocator` for memory allocation.
void printRecord(bsl::ostream&                  stream,
                 const mqbs::DeletionRecord&    rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                 bslma::Allocator*              allocator);

/// Print the specified queueOp record `rec` and QueueInfo pointed by the
/// specified `queueInfo_p` to the specified `stream`, using the specified
/// `allocator` for memory allocation.
void printRecord(bsl::ostream&                  stream,
                 const mqbs::QueueOpRecord&     rec,
                 const bmqp_ctrlmsg::QueueInfo* queueInfo_p,
                 bslma::Allocator*              allocator);

/// Find AppId in the specified `appIds` by the specified `appKey` and store
/// the result in the specified `appId`. Return `true` on success and `false
/// otherwise.
bool findQueueAppIdByAppKey(
    bsl::string*                                             appId,
    const bsl::vector<BloombergLP::bmqp_ctrlmsg::AppIdInfo>& appIds,
    const mqbu::StorageKey&                                  appKey);

}  // close namespace RecordPrinter

}  // close package namespace
}  // close enterprise namespace

#endif
