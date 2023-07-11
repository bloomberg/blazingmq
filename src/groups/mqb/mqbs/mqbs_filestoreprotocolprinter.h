// Copyright 2022-2023 Bloomberg Finance L.P.
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

// mqbs_filestoreprotocolprinter.h                                    -*-C++-*-
#ifndef INCLUDED_MQBS_FILESTOREPROTOCOLPRINTER
#define INCLUDED_MQBS_FILESTOREPROTOCOLPRINTER

//@PURPOSE: Provide utilities for printing file store protocol primitives.
//
//@CLASSES:
//
//@DESCRIPTION: 'mqbs::FileStoreProtocolPrinter' provides methods for printing
// file store primitives.

// MQB
#include <mqbs_datafileiterator.h>
#include <mqbs_filestoreprotocol.h>
#include <mqbs_journalfileiterator.h>
#include <mqbs_mappedfiledescriptor.h>
#include <mqbs_qlistfileiterator.h>
#include <mqbu_storagekey.h>

// BDE
#include <bsl_map.h>
#include <bsl_ostream.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsls_annotation.h>

// BMQ
#include <bmqp_messageproperties.h>
#include <bmqp_optionsview.h>

namespace BloombergLP {
namespace mqbs {

// FREE FUNCTIONS

/// Print the specified `dataHeader` to the specified `stream`.
bsl::ostream& operator<<(bsl::ostream&           stream,
                         const mqbs::DataHeader& dataHeader);

/// Print the specified `header` to the specified `stream`.
bsl::ostream& operator<<(bsl::ostream&               stream,
                         const mqbs::DataFileHeader& header);

/// Print the specified `header` to the specified `stream`.
bsl::ostream& operator<<(bsl::ostream&                stream,
                         const mqbs::QlistFileHeader& header);

/// Print the header of the mapped file by the specified `mfd` to the
/// specified `stream`.
bsl::ostream& operator<<(bsl::ostream&                     stream,
                         const mqbs::MappedFileDescriptor& mfd);

/// Print the value of message property currently pointed by the
/// specified `iterator` to the specified `stream`.
bsl::ostream& operator<<(bsl::ostream&                          stream,
                         const bmqp::MessagePropertiesIterator& iterator);

/// Print the summary of the specified `it` to the specified `stream`.
bsl::ostream& operator<<(bsl::ostream&                 stream,
                         const mqbs::DataFileIterator& it);

/// Print the specified `it` to the specified `stream`.
bsl::ostream& operator<<(bsl::ostream&                  stream,
                         const mqbs::QlistFileIterator& it);

/// Print the specified `it` to the specified `stream`.
bsl::ostream& operator<<(bsl::ostream&                    stream,
                         const mqbs::JournalFileIterator& it);

// ==================================
// namespace FileStoreProtocolPrinter
// ==================================

namespace FileStoreProtocolPrinter {

// FREE FUNCTIONS

/// Print the specified `header` while using the specified `journalFd`
/// to the specified `stream`.
void printHeader(bsl::ostream&                     stream,
                 const mqbs::JournalFileHeader&    header,
                 const mqbs::MappedFileDescriptor& journalFd);

/// Print the data from the data file currently pointed by the specified
/// `it`.
void printIterator(mqbs::DataFileIterator& it);

/// Print the data from the queue list file currently pointed by the
/// specified `it`.
void printIterator(mqbs::QlistFileIterator& it);

/// Print the data from the journal file currently pointed by the
/// specified `it`.
void printIterator(mqbs::JournalFileIterator& it);

/// Print the message properties contained in the specified `appData`
/// payload to the specified `stream` and populate the specified
/// `propertiesAreaLen` with the size of properties area.  Return zero
/// on success and a non-zero value otherwise.  Behavior is undefined
/// unless `appData` is non-null and contains message properties.
/// Behavior is also undefined unless `propertiesAreaLen` is non-null.
int printMessageProperties(unsigned int* propertiesAreaLen,
                           bsl::ostream& stream,
                           const char*   appData,
                           const bmqp::MessagePropertiesInfo& logic);

/// TBD: Once OptionsView is updated to support GroupId option, update
/// this switch case to handle and print that option.  At that time,
/// also update the logic to correctly print (with proper indentation
/// etc) the subQueueId option (even though this options won't be stored
/// in the DATA file).
void printOption(bsl::ostream&                stream,
                 BSLS_ANNOTATION_UNUSED const bmqp::OptionsView* ov,
                 const bmqp::OptionsView::const_iterator&        cit);

/// Print the options pointed by the specified `options` of the
/// specified `len` to the specified `stream`.
void printOptions(bsl::ostream& stream, const char* options, unsigned int len);

/// Print the specified `data` of the specified `len` to the specified
/// `stream`.
void printPayload(bsl::ostream& stream, const char* data, unsigned int len);

/// Print the specified `rec` to the specified `stream`.
void printRecord(bsl::ostream& stream, const mqbs::MessageRecord& rec);

/// Print the specified `rec` to the specified `stream`.
void printRecord(bsl::ostream& stream, const mqbs::ConfirmRecord& rec);

/// Print the specified `rec` to the specified `stream`.
void printRecord(bsl::ostream& stream, const mqbs::DeletionRecord& rec);

/// Print the specified `rec` to the specified `stream`.
void printRecord(bsl::ostream& stream, const mqbs::QueueOpRecord& rec);

/// Print the specified `rec` to the specified `stream`.
void printRecord(bsl::ostream& stream, const mqbs::JournalOpRecord& rec);

}  // close namespace FileStoreProtocolPrinter

}  // close package namespace
}  // close enterprise namespace

#endif
