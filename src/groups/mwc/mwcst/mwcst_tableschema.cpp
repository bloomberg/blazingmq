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

// mwcst_tableschema.cpp                                              -*-C++-*-
#include <mwcst_tableschema.h>

#include <bdlf_bind.h>
#include <bdlma_localsequentialallocator.h>
#include <bsl_functional.h>
#include <bslmf_allocatorargt.h>
#include <bsls_alignedbuffer.h>
#include <mwcscm_version.h>

namespace BloombergLP {
namespace mwcst {

namespace {

// TYPEDEFS
typedef bsl::function<bsls::Types::Int64(const StatValue& value)> IntFunc;
typedef bsl::function<double(const StatValue& value)>             DoubleFunc;

// CONSTANTS
const char DIRECT_NAME[]  = "*direct*";
const char EXPIRED_NAME[] = "*expired*";
const char UNKNOWN_NAME[] = "*unknown*";

// FUNCTIONS
void intFuncWrapper(mwct::Value*           value,
                    const StatContext&     context,
                    StatContext::ValueType type,
                    int                    statIndex,
                    const IntFunc&         fn)
{
    const StatValue& sv = context.value(type, statIndex);
    value->set(fn(sv));
}

void doubleFuncWrapper(mwct::Value*           value,
                       const StatContext&     context,
                       StatContext::ValueType type,
                       int                    statIndex,
                       const DoubleFunc&      fn)
{
    const StatValue& sv = context.value(type, statIndex);
    value->set(fn(sv));
}

void defaultIdColumn(mwct::Value*           value,
                     const StatContext&     context,
                     int                    level,
                     StatContext::ValueType type)
{
    bslstl::StringRef name;
    if (type == StatContext::DMCST_TOTAL_VALUE) {
        if (context.hasName()) {
            name = context.name();
        }
    }
    else if (type == StatContext::DMCST_DIRECT_VALUE) {
        name = DIRECT_NAME;
    }
    else if (type == StatContext::DMCST_EXPIRED_VALUE) {
        name = EXPIRED_NAME;
    }
    else {
        name = UNKNOWN_NAME;
    }

    char idBuf[64];
    if (name.isEmpty()) {
        // Must be Id
        sprintf(idBuf, "%lld", context.id());
        name = idBuf;
    }

    bdlma::LocalSequentialAllocator<128> seqAlloc;

    bsl::string storage(&seqAlloc);

    storage.assign(2 * level, ' ');
    if (context.isDeleted()) {
        storage.append(1, '(');
    }
    storage.append(name.begin(), name.end());
    if (context.isDeleted()) {
        storage.append(1, ')');
    }

    value->set(storage);
    value->ownValue();
}

}  // close anonymous namespace

// -----------------------
// class TableSchemaColumn
// -----------------------

// CREATORS
TableSchemaColumn::TableSchemaColumn(const bslstl::StringRef& name,
                                     const ValueFn&           fn,
                                     bslma::Allocator*        basicAllocator)
: d_name(name, basicAllocator)
, d_fn(bsl::allocator_arg, basicAllocator, fn)
{
}

TableSchemaColumn::TableSchemaColumn(const TableSchemaColumn& other,
                                     bslma::Allocator*        basicAllocator)
: d_name(other.d_name, basicAllocator)
, d_fn(bsl::allocator_arg, basicAllocator, other.d_fn)
{
}

// ACCESSORS
const bsl::string& TableSchemaColumn::name() const
{
    return d_name;
}

void TableSchemaColumn::evaluate(mwct::Value*           value,
                                 const StatContext&     context,
                                 int                    level,
                                 StatContext::ValueType type) const
{
    return d_fn(value, context, level, type);
}

// -----------------
// class TableSchema
// -----------------

// CREATORS
TableSchema::TableSchema(bslma::Allocator* basicAllocator)
: d_columns(basicAllocator)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

TableSchema::TableSchema(const TableSchema& other,
                         bslma::Allocator*  basicAllocator)
: d_columns(other.d_columns, basicAllocator)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

// MANIPULATORS
TableSchemaColumn& TableSchema::addColumn(const bslstl::StringRef& name,
                                          const Column::ValueFn&   func)
{
    d_columns.emplace_back(name, func);
    return d_columns.back();
}

void TableSchema::addDefaultIdColumn(const bslstl::StringRef& name)
{
    Column::ValueFn columnFn = bdlf::BindUtil::bind(defaultIdColumn,
                                                    bdlf::PlaceHolders::_1,
                                                    bdlf::PlaceHolders::_2,
                                                    bdlf::PlaceHolders::_3,
                                                    bdlf::PlaceHolders::_4);

    addColumn(name, columnFn);
}

TableSchemaColumn&
TableSchema::addColumn(const bslstl::StringRef& name,
                       int                      statIndex,
                       bsls::Types::Int64 (*func)(const StatValue&))
{
    IntFunc         intFn = bdlf::BindUtil::bind(func, bdlf::PlaceHolders::_1);
    Column::ValueFn columnFn = bdlf::BindUtil::bind(&intFuncWrapper,
                                                    bdlf::PlaceHolders::_1,
                                                    bdlf::PlaceHolders::_2,
                                                    bdlf::PlaceHolders::_4,
                                                    statIndex,
                                                    intFn);

    return addColumn(name, columnFn);
}

TableSchemaColumn& TableSchema::addColumn(
    const bslstl::StringRef& name,
    int                      statIndex,
    bsls::Types::Int64 (*func)(const StatValue&,
                               const StatValue::SnapshotLocation&),
    const StatValue::SnapshotLocation& snapshot)
{
    IntFunc         intFn    = bdlf::BindUtil::bind(func,
                                         bdlf::PlaceHolders::_1,
                                         snapshot);
    Column::ValueFn columnFn = bdlf::BindUtil::bind(&intFuncWrapper,
                                                    bdlf::PlaceHolders::_1,
                                                    bdlf::PlaceHolders::_2,
                                                    bdlf::PlaceHolders::_4,
                                                    statIndex,
                                                    intFn);

    return addColumn(name, columnFn);
}

TableSchemaColumn& TableSchema::addColumn(
    const bslstl::StringRef& name,
    int                      statIndex,
    bsls::Types::Int64 (*func)(const StatValue&,
                               const StatValue::SnapshotLocation&,
                               const StatValue::SnapshotLocation&),
    const StatValue::SnapshotLocation& snapshot1,
    const StatValue::SnapshotLocation& snapshot2)
{
    IntFunc         intFn    = bdlf::BindUtil::bind(func,
                                         bdlf::PlaceHolders::_1,
                                         snapshot1,
                                         snapshot2);
    Column::ValueFn columnFn = bdlf::BindUtil::bind(&intFuncWrapper,
                                                    bdlf::PlaceHolders::_1,
                                                    bdlf::PlaceHolders::_2,
                                                    bdlf::PlaceHolders::_4,
                                                    statIndex,
                                                    intFn);

    return addColumn(name, columnFn);
}

TableSchemaColumn& TableSchema::addColumn(const bslstl::StringRef& name,
                                          int                      statIndex,
                                          double (*func)(const StatValue&))
{
    DoubleFunc doubleFn = bdlf::BindUtil::bind(func, bdlf::PlaceHolders::_1);
    Column::ValueFn columnFn = bdlf::BindUtil::bind(&doubleFuncWrapper,
                                                    bdlf::PlaceHolders::_1,
                                                    bdlf::PlaceHolders::_2,
                                                    bdlf::PlaceHolders::_4,
                                                    statIndex,
                                                    doubleFn);

    return addColumn(name, columnFn);
}

TableSchemaColumn& TableSchema::addColumn(
    const bslstl::StringRef& name,
    int                      statIndex,
    double (*func)(const StatValue&, const StatValue::SnapshotLocation&),
    const StatValue::SnapshotLocation& snapshot)
{
    DoubleFunc      doubleFn = bdlf::BindUtil::bind(func,
                                               bdlf::PlaceHolders::_1,
                                               snapshot);
    Column::ValueFn columnFn = bdlf::BindUtil::bind(&doubleFuncWrapper,
                                                    bdlf::PlaceHolders::_1,
                                                    bdlf::PlaceHolders::_2,
                                                    bdlf::PlaceHolders::_4,
                                                    statIndex,
                                                    doubleFn);

    return addColumn(name, columnFn);
}

TableSchemaColumn&
TableSchema::addColumn(const bslstl::StringRef& name,
                       int                      statIndex,
                       double (*func)(const StatValue&,
                                      const StatValue::SnapshotLocation&,
                                      const StatValue::SnapshotLocation&),
                       const StatValue::SnapshotLocation& snapshot1,
                       const StatValue::SnapshotLocation& snapshot2)
{
    DoubleFunc      doubleFn = bdlf::BindUtil::bind(func,
                                               bdlf::PlaceHolders::_1,
                                               snapshot1,
                                               snapshot2);
    Column::ValueFn columnFn = bdlf::BindUtil::bind(&doubleFuncWrapper,
                                                    bdlf::PlaceHolders::_1,
                                                    bdlf::PlaceHolders::_2,
                                                    bdlf::PlaceHolders::_4,
                                                    statIndex,
                                                    doubleFn);

    return addColumn(name, columnFn);
}

// ACCESSORS
int TableSchema::numColumns() const
{
    return static_cast<int>(d_columns.size());
}

const TableSchemaColumn& TableSchema::column(int index) const
{
    BSLS_ASSERT(index < numColumns());
    return d_columns[index];
}

}  // close package namespace
}  // close enterprise namespace
