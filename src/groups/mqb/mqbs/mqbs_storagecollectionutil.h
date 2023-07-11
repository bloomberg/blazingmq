// Copyright 2018-2023 Bloomberg Finance L.P.
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

// mqbs_storagecollectionutil.h                                       -*-C++-*-
#ifndef INCLUDED_MQBS_STORAGECOLLECTIONUTIL
#define INCLUDED_MQBS_STORAGECOLLECTIONUTIL

//@PURPOSE: Provide utilities for a collection of BlazingMQ storages.
//
//@CLASSES:
//  mqbs::StorageCollectionUtil:              Utilities for a collection of BMQ
//                                            storages
//  mqbs::StorageCollectionUtilSortMetric:    Metrics that BlazingMQ storages
//                                            should be sorted on
//  mqbs::StorageCollectionUtilFilterFactory: BlazingMQ storage filter creation
//                                            utility
//
//@SEE ALSO: mqbs::ReplicatedStorage
//
//@DESCRIPTION: 'mqbs::StorageCollectionUtil' provides utilities for a
// collection of BlazingMQ storages.

// MQB
#include <mqbs_replicatedstorage.h>
#include <mqbu_storagekey.h>

// BDE
#include <bsl_functional.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace mqbs {

// ======================================
// struct StorageCollectionUtilSortMetric
// ======================================

/// This struct defines the types of metrics a collection of storages should
/// be sorted on.
struct StorageCollectionUtilSortMetric {
    // TYPES
    enum Enum {
        /// Alphanumeric order of its queue's URI
        e_QUEUE_URI     = 0,
        e_MESSAGE_COUNT = 1
        // Number of messages in its queue, in descending order
        ,
        e_BYTE_COUNT = 2
        // Number of bytes in its queue, in descending order

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
    /// `StorageCollectionUtilSortMetric::Enum` value.
    static bsl::ostream& print(bsl::ostream&                         stream,
                               StorageCollectionUtilSortMetric::Enum value,
                               int                                   level = 0,
                               int spacesPerLevel = 4);

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "e_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << StorageCollectionUtilSortMetric::toAscii(
    ///                  StorageCollectionUtilSortMetric::e_MESSAGE_COUNT);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// MESSAGE_COUNT
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(StorageCollectionUtilSortMetric::Enum value);
};

// FREE OPERATORS

/// Format the specified `value` to the specified output `stream` and return
/// a reference to the modifiable `stream`.
bsl::ostream& operator<<(bsl::ostream&                         stream,
                         StorageCollectionUtilSortMetric::Enum value);

// ============================
// struct StorageCollectionUtil
// ============================

/// This component provides utilities to work with a collection of BMQ
/// storages.
struct StorageCollectionUtil {
  public:
    // TYPES
    typedef bsl::vector<const ReplicatedStorage*> StorageList;
    typedef StorageList::iterator                 StorageListIter;
    typedef StorageList::const_iterator           StorageListConstIter;

    /// QueueKey -> ReplicatedStorage* map
    typedef bsl::unordered_map<mqbu::StorageKey, ReplicatedStorage*>
                                        StoragesMap;
    typedef StoragesMap::iterator       StorageMapIter;
    typedef StoragesMap::const_iterator StorageMapConstIter;

    typedef bsl::function<bool(const ReplicatedStorage*)> StorageFilter;

  public:
    // CLASS METHODS

    /// Load into the specified `storages` the set of values of the
    /// specified `storageMap`.
    static void loadStorages(StorageList*       storages,
                             const StoragesMap& storageMap);

    /// Filter out from the specified `storages` all items for which the
    /// specified `filter` returns false.
    static void filterStorages(StorageList*         storages,
                               const StorageFilter& filter);

    /// Sort the content of the specified `storages` by the specified
    /// `metric`.
    static void sortStorages(StorageList*                          storages,
                             StorageCollectionUtilSortMetric::Enum metric);
};

// =========================================
// struct StorageCollectionUtilFilterFactory
// =========================================

/// Utility class for creating filters for BlazingMQ storages.
struct StorageCollectionUtilFilterFactory {
  private:
    // PRIVATE TYPES
    typedef StorageCollectionUtil::StorageFilter StorageFilter;

  public:
    // CLASS METHODS

    /// Return a storage filter that returns false for all storages whose
    /// queue does not belong to the specified `domainName`.
    static StorageFilter byDomain(const bsl::string& domainName);

    /// Return a storage filter that returns false for all storages whose
    /// number of messages in its queue, `numMessages`, does not satisfy
    /// `lowerLimit <= numMessages <= upperLimit`.
    static StorageFilter
    byMessageCount(bsls::Types::Int64 lowerLimit = 0,
                   bsls::Types::Int64 upperLimit =
                       bsl::numeric_limits<bsls::Types::Int64>::max());

    /// Return a storage filter that returns false for all storages whose
    /// number of bytes in its queue, `numBytes`, does not satisfy
    /// `lowerLimit <= numMessages <= upperLimit`.
    static StorageFilter
    byByteCount(bsls::Types::Int64 lowerLimit = 0,
                bsls::Types::Int64 upperLimit =
                    bsl::numeric_limits<bsls::Types::Int64>::max());
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------------------------
// struct StorageCollectionUtilSortMetric
// --------------------------------------
// FREE OPERATORS
inline bsl::ostream& operator<<(bsl::ostream&                         stream,
                                StorageCollectionUtilSortMetric::Enum value)
{
    return StorageCollectionUtilSortMetric::print(stream, value, 0, -1);
}

}  // close package namespace
}  // close enterprise namespace

#endif
