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

// mwct_propertybag.cpp                                               -*-C++-*-
#include <mwct_propertybag.h>

#include <mwcscm_version.h>
// BDE
#include <bdlma_localsequentialallocator.h>
#include <bslim_printer.h>
#include <bslmf_movableref.h>

namespace BloombergLP {
namespace mwct {

// ----------------------
// class PropertyBagValue
// ----------------------

PropertyBagValue::PropertyBagValue(const bslstl::StringRef& name,
                                   const bdld::Datum&       datum,
                                   bslma::Allocator*        datumAllocator,
                                   bslma::Allocator*        allocator)
: d_name(name, allocator)
, d_value(bdld::ManagedDatum(datum, datumAllocator), allocator)
{
    // NOTHING
}

PropertyBagValue::PropertyBagValue(const bslstl::StringRef&     name,
                                   const bsl::shared_ptr<void>& sptr,
                                   bslma::Allocator*            allocator)
: d_name(name, allocator)
, d_value(sptr, allocator)
{
    // NOTHING
}

PropertyBagValue::PropertyBagValue(const PropertyBagValue& original,
                                   bslma::Allocator*       allocator)
: d_name(original.d_name, allocator)
, d_value(original.d_value, allocator)
{
    // NOTHING
}

bsl::ostream& PropertyBagValue::print(bsl::ostream& stream,
                                      int           level,
                                      int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute(d_name.c_str(), d_value);
    printer.end();

    return stream;
}

// -----------------
// class PropertyBag
// -----------------

void PropertyBag::insertValueImp(const ValueSPtr& value)
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(d_lock.tryLock() != 0);

    d_values.erase(value->d_name);
    d_values[value->name()] = value;
}

PropertyBag::PropertyBag(bslma::Allocator* allocator)
: d_values(allocator)
, d_lock()
{
    // NOTHING
}

PropertyBag::PropertyBag(const PropertyBag& original,
                         bslma::Allocator*  allocator)
: d_values(allocator)
, d_lock()
{
    *this = original;
}

PropertyBag& PropertyBag::operator=(const PropertyBag& rhs)
{
    if (this != &rhs) {
        // Copy the properties of 'rhs' into local ValueMap, and then swap it
        // with 'd_values'.  Note that we need to copy all the properties, even
        // though they are shared_ptrs because they were created using 'rhs''s
        // allocator, and are only guaranteed to be good for the lifetime of
        // 'rhs'.
        ValueMap localValues(allocator());

        {  //   rhs LOCK
            bsls::SpinLockGuard guard(&rhs.d_lock);
            for (ValueMap::const_iterator iter = rhs.d_values.begin();
                 iter != rhs.d_values.end();
                 ++iter) {
                ValueSPtr newVal;
                newVal.createInplace(allocator(), *iter->second, allocator());
                localValues[newVal->name()] = newVal;
            }
        }  // rhs UNLOCK

        bsls::SpinLockGuard guard(&d_lock);  // LOCK
        d_values.swap(localValues);
    }

    return *this;
}

PropertyBag& PropertyBag::import(const Value& value)
{
    ValueSPtr newVal;
    newVal.createInplace(allocator(), value, allocator());

    bsls::SpinLockGuard guard(&d_lock);  // LOCK
    insertValueImp(newVal);

    return *this;
}

PropertyBag&
PropertyBag::import(const bsl::vector<bslma::ManagedPtr<Value> >& values)
{
    bsls::SpinLockGuard guard(&d_lock);  // LOCK

    for (size_t i = 0; i < values.size(); ++i) {
        ValueSPtr newVal;
        newVal.createInplace(allocator(), *values[i], allocator());
        insertValueImp(newVal);
    }

    return *this;
}

PropertyBag& PropertyBag::set(const bslstl::StringRef& key, const Value& value)
{
    ValueSPtr newVal;
    newVal.createInplace(allocator(), value, allocator());
    newVal->d_name = key;

    bsls::SpinLockGuard guard(&d_lock);  // LOCK
    insertValueImp(newVal);

    return *this;
}

PropertyBag& PropertyBag::set(const bslstl::StringRef& key,
                              const bdld::Datum&       datum,
                              bslma::Allocator*        datumAllocator)
{
    ValueSPtr newVal;
    newVal.createInplace(allocator(), key, datum, datumAllocator, allocator());

    bsls::SpinLockGuard guard(&d_lock);  // LOCK
    insertValueImp(newVal);

    return *this;
}

PropertyBag& PropertyBag::set(const bslstl::StringRef&     key,
                              const bsl::shared_ptr<void>& ptr)
{
    ValueSPtr newVal;
    newVal.createInplace(allocator(), key, ptr, allocator());

    bsls::SpinLockGuard guard(&d_lock);  // LOCK
    insertValueImp(newVal);

    return *this;
}

PropertyBag& PropertyBag::set(const bslstl::StringRef& key, int value)
{
    return set(key, bdld::Datum::createInteger(value), allocator());
}

PropertyBag& PropertyBag::set(const bslstl::StringRef& key,
                              bsls::Types::Int64       value)
{
    return set(key,
               bdld::Datum::createInteger64(value, allocator()),
               allocator());
}

PropertyBag& PropertyBag::set(const bslstl::StringRef& key,
                              const bslstl::StringRef& value)
{
    return set(key, bdld::Datum::copyString(value, allocator()), allocator());
}

PropertyBag& PropertyBag::unset(const bslstl::StringRef& key)
{
    bsls::SpinLockGuard guard(&d_lock);  // LOCK
    d_values.erase(key);

    return *this;
}

void PropertyBag::loadAll(bsl::vector<bslma::ManagedPtr<Value> >* dest) const
{
    bsls::SpinLockGuard guard(&d_lock);  // LOCK
    dest->reserve(dest->size() + d_values.size());
    for (ValueMap::const_iterator iter = d_values.begin();
         iter != d_values.end();
         ++iter) {
        bslma::ManagedPtr<Value> value(iter->second.managedPtr());
        dest->emplace_back(bslmf::MovableRefUtil::move(value));
    }
}

bool PropertyBag::load(bslma::ManagedPtr<Value>* dest,
                       const bslstl::StringRef&  key) const
{
    bsls::SpinLockGuard      guard(&d_lock);  // LOCK
    ValueMap::const_iterator iter = d_values.find(key);
    if (iter == d_values.end()) {
        return false;  // RETURN
    }

    *dest = iter->second.managedPtr();
    return true;
}

bool PropertyBag::load(int* dest, const bslstl::StringRef& key) const
{
    bsls::SpinLockGuard      guard(&d_lock);  // LOCK
    ValueMap::const_iterator iter = d_values.find(key);
    if (iter == d_values.end() || !iter->second->isDatum() ||
        !iter->second->theDatum().isInteger()) {
        return false;  // RETURN
    }

    *dest = iter->second->theDatum().theInteger();
    return true;
}

bool PropertyBag::load(bsls::Types::Int64*      dest,
                       const bslstl::StringRef& key) const
{
    bsls::SpinLockGuard      guard(&d_lock);  // LOCK
    ValueMap::const_iterator iter = d_values.find(key);
    if (iter == d_values.end() || !iter->second->isDatum() ||
        !iter->second->theDatum().isInteger64()) {
        return false;  // RETURN
    }

    *dest = iter->second->theDatum().theInteger64();
    return true;
}

bool PropertyBag::load(bslstl::StringRef*       dest,
                       const bslstl::StringRef& key) const
{
    bsls::SpinLockGuard      guard(&d_lock);  // LOCK
    ValueMap::const_iterator iter = d_values.find(key);
    if (iter == d_values.end() || !iter->second->isDatum() ||
        !iter->second->theDatum().isString()) {
        return false;  // RETURN
    }

    *dest = iter->second->theDatum().theString();
    return true;
}

bsl::ostream&
PropertyBag::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bsls::SpinLockGuard guard(&d_lock);  // LOCK

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    for (ValueMap::const_iterator iter = d_values.begin();
         iter != d_values.end();
         ++iter) {
        printer.printAttribute(iter->second->name().c_str(),
                               iter->second->d_value);
    }
    printer.end();

    return stream;
}

// ----------------------
// struct PropertyBagUtil
// ----------------------

void PropertyBagUtil::update(PropertyBag* dest, const PropertyBag& src)
{
    bdlma::LocalSequentialAllocator<1024>             arena;
    bsl::vector<bslma::ManagedPtr<PropertyBagValue> > values(&arena);

    src.loadAll(&values);
    dest->import(values);
}

}  // close package namespace
}  // close enterprise namespace
