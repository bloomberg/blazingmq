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

// mqbs_storagecollectionutil.cpp                                     -*-C++-*-
#include <mqbs_storagecollectionutil.h>

// BDE
#include <bdlb_print.h>
#include <bdlf_bind.h>
#include <bdlf_placeholder.h>
#include <bsl_algorithm.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace mqbs {

namespace {

typedef bsl::function<bool(const ReplicatedStorage*, const ReplicatedStorage*)>
    StorageComparator;

bool queueUriStorageComparator(const ReplicatedStorage* rs1,
                               const ReplicatedStorage* rs2)
{
    return rs1->queueUri() < rs2->queueUri();
}

bool messageCountStorageComparator(const ReplicatedStorage* rs1,
                                   const ReplicatedStorage* rs2)
{
    return rs1->numMessages(mqbu::StorageKey::k_NULL_KEY) >
           rs2->numMessages(mqbu::StorageKey::k_NULL_KEY);
}

bool byteCountStorageComparator(const ReplicatedStorage* rs1,
                                const ReplicatedStorage* rs2)
{
    return rs1->numBytes(mqbu::StorageKey::k_NULL_KEY) >
           rs2->numBytes(mqbu::StorageKey::k_NULL_KEY);
}

StorageComparator
createStorageComparator(StorageCollectionUtilSortMetric::Enum metricType)
{
    switch (metricType) {
    case StorageCollectionUtilSortMetric::e_MESSAGE_COUNT:
        return messageCountStorageComparator;  // RETURN
    case StorageCollectionUtilSortMetric::e_BYTE_COUNT:
        return byteCountStorageComparator;  // RETURN
    case StorageCollectionUtilSortMetric::e_QUEUE_URI:
        return queueUriStorageComparator;  // RETURN
    default:
        BSLS_ASSERT_OPT(false && "Unknown metricType");
        return queueUriStorageComparator;
    }
}

bool byDomainFilter(const ReplicatedStorage* storage,
                    const bsl::string&       domainName)
{
    return strncmp(storage->queueUri().qualifiedDomain().data(),
                   domainName.c_str(),
                   storage->queueUri().qualifiedDomain().length()) == 0;
}

bool byMessageCountFilter(const ReplicatedStorage* storage,
                          bsls::Types::Int64       lowerLimit,
                          bsls::Types::Int64       upperLimit)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(lowerLimit <= upperLimit);

    const bsls::Types::Int64 numMessage = storage->numMessages(
        mqbu::StorageKey::k_NULL_KEY);
    return (lowerLimit <= numMessage) && (numMessage <= upperLimit);
}

bool byByteCountFilter(const ReplicatedStorage* storage,
                       bsls::Types::Int64       lowerLimit,
                       bsls::Types::Int64       upperLimit)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(lowerLimit <= upperLimit);

    const bsls::Types::Int64 numByte = storage->numBytes(
        mqbu::StorageKey::k_NULL_KEY);
    return (lowerLimit <= numByte) && (numByte <= upperLimit);
}

}  // close unnamed namespace

// --------------------------------------
// struct StorageCollectionUtilSortMetric
// --------------------------------------

bsl::ostream& StorageCollectionUtilSortMetric::print(
    bsl::ostream&                         stream,
    StorageCollectionUtilSortMetric::Enum value,
    int                                   level,
    int                                   spacesPerLevel)
{
    bdlb::Print::indent(stream, level, spacesPerLevel);
    stream << StorageCollectionUtilSortMetric::toAscii(value);

    if (spacesPerLevel >= 0) {
        stream << '\n';
    }

    return stream;
}

const char* StorageCollectionUtilSortMetric::toAscii(
    StorageCollectionUtilSortMetric::Enum value)
{
#define CASE(X)                                                               \
    case e_##X: return #X;

    switch (value) {
        CASE(QUEUE_URI)
        CASE(MESSAGE_COUNT)
        CASE(BYTE_COUNT)
    default: return "(* UNKNOWN *)";
    }

#undef CASE
}

// ----------------------------
// struct StorageCollectionUtil
// ----------------------------

// CLASS METHODS
void StorageCollectionUtil::loadStorages(StorageList*       storages,
                                         const StoragesMap& storageMap)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storages);

    storages->reserve(storageMap.size());
    for (StorageMapConstIter cit = storageMap.cbegin();
         cit != storageMap.cend();
         ++cit) {
        storages->emplace_back(cit->second);
    }
}

namespace {

template <class Predicate>
struct Not1 {
    Not1(const Predicate& predicate)
    : d_predicate(predicate)
    {
    }

    template <class Value>
    bool operator()(const Value& value) const
    {
        return !d_predicate(value);
    }
    Predicate d_predicate;
};

template <class Predicate>
Not1<Predicate> not_pred(const Predicate& predicate)
{
    return Not1<Predicate>(predicate);
}

}  // close anonymous namespace

void StorageCollectionUtil::filterStorages(StorageList*         storages,
                                           const StorageFilter& filter)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storages);

    StorageListIter iter = bsl::remove_if(storages->begin(),
                                          storages->end(),
                                          not_pred(filter));
    storages->erase(iter, storages->end());
}

void StorageCollectionUtil::sortStorages(
    StorageList*                          storages,
    StorageCollectionUtilSortMetric::Enum metric)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(storages);

    bsl::sort(storages->begin(),
              storages->end(),
              createStorageComparator(metric));
}

// -----------------------------------------
// struct StorageCollectionUtilFilterFactory
// -----------------------------------------

StorageCollectionUtil::StorageFilter
StorageCollectionUtilFilterFactory::byDomain(const bsl::string& domainName)
{
    return bdlf::BindUtil::bind(&byDomainFilter,
                                bdlf::PlaceHolders::_1,  // storage
                                domainName);
}

StorageCollectionUtil::StorageFilter
StorageCollectionUtilFilterFactory::byMessageCount(
    bsls::Types::Int64 lowerLimit,
    bsls::Types::Int64 upperLimit)
{
    return bdlf::BindUtil::bind(&byMessageCountFilter,
                                bdlf::PlaceHolders::_1,  // storage
                                lowerLimit,
                                upperLimit);
}

StorageCollectionUtil::StorageFilter
StorageCollectionUtilFilterFactory::byByteCount(bsls::Types::Int64 lowerLimit,
                                                bsls::Types::Int64 upperLimit)
{
    return bdlf::BindUtil::bind(&byByteCountFilter,
                                bdlf::PlaceHolders::_1,  // storage
                                lowerLimit,
                                upperLimit);
}

}  // close package namespace
}  // close enterprise namespace
