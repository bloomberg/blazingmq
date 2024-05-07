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

// mwcst_statcontexttableinfoprovider.cpp                             -*-C++-*-
#include <mwcst_statcontexttableinfoprovider.h>

#include <mwcscm_version.h>
#include <mwcst_statcontext.h>
#include <mwcst_statvalue.h>

#include <mwcst_printutil.h>

#include <bdlf_bind.h>
#include <bdlma_localsequentialallocator.h>
#include <bsls_alignedbuffer.h>

#include <bsl_ios.h>

namespace BloombergLP {
namespace mwcst {

namespace {

// CONSTANTS
const bslstl::StringRef  DIRECT_NAME("*direct*");
const bslstl::StringRef  EXPIRED_NAME("*expired*");
const bslstl::StringRef  UNKNOWN_NAME("!UNKNOWN!");
const bslstl::StringRef  NA_STRING("N/A");
const bsls::Types::Int64 MAX_INT =
    bsl::numeric_limits<bsls::Types::Int64>::max();
const bsls::Types::Int64 MIN_INT =
    bsl::numeric_limits<bsls::Types::Int64>::min();

// =====================
// class DefaultIdColumn
// =====================

/// A CustomColumn printing the context ids at an appropriate indentation
/// level
class DefaultIdColumn : public StatContextTableInfoProviderCustomColumn {
  private:
    // DATA
    mutable bsl::string d_nameBuf;
    const int           d_maxSize;

  public:
    // CREATORS
    DefaultIdColumn(int maxSize, bslma::Allocator* basicAllocator = 0)
    : d_nameBuf(basicAllocator)
    , d_maxSize(maxSize)
    {
    }

    ~DefaultIdColumn() BSLS_KEYWORD_OVERRIDE {}

    // ACCESSORS
    int
    getValueSize(int                    level,
                 const StatContext&     context,
                 StatContext::ValueType valueType) const BSLS_KEYWORD_OVERRIDE
    {
        int length = 0;
        if (valueType == StatContext::DMCST_TOTAL_VALUE) {
            if (context.hasName()) {
                length = static_cast<int>(context.name().length());
            }
            else {
                length = mwcst::PrintUtil::printedValueLength(context.id());
            }
        }
        else if (valueType == StatContext::DMCST_DIRECT_VALUE) {
            length = static_cast<int>(DIRECT_NAME.length());
        }
        else if (valueType == StatContext::DMCST_EXPIRED_VALUE) {
            length = static_cast<int>(EXPIRED_NAME.length());
        }
        else {
            length = static_cast<int>(UNKNOWN_NAME.length());
        }

        length += 2 * level;
        if (context.isDeleted()) {
            length += 2;
        }

        if (d_maxSize > 0) {
            return bsl::min(length, d_maxSize);
        }
        else {
            return length;
        }
    }

    bsl::ostream&
    printValue(bsl::ostream&          stream,
               int                    level,
               const StatContext&     context,
               StatContext::ValueType valueType) const BSLS_KEYWORD_OVERRIDE
    {
        bslstl::StringRef name;
        if (valueType == StatContext::DMCST_TOTAL_VALUE) {
            if (context.hasName()) {
                name = context.name();
            }
        }
        else if (valueType == StatContext::DMCST_DIRECT_VALUE) {
            name = DIRECT_NAME;
        }
        else if (valueType == StatContext::DMCST_EXPIRED_VALUE) {
            name = EXPIRED_NAME;
        }
        else {
            name = UNKNOWN_NAME;
        }

        enum { k_SIZE = 64 };
        char idBuf[k_SIZE];
        if (name.isEmpty()) {
            // Must be Id
            snprintf(idBuf, k_SIZE, "%lld", context.id());
            name = idBuf;
        }

        d_nameBuf.assign(2 * level, ' ');
        if (context.isDeleted()) {
            d_nameBuf.append(1, '(');
        }
        d_nameBuf.append(name.begin(), name.end());
        if (context.isDeleted()) {
            d_nameBuf.append(1, ')');
        }

        if (d_maxSize > 0) {
            if (d_nameBuf.size() > static_cast<bsl::size_t>(d_maxSize)) {
                d_nameBuf.erase(d_nameBuf.begin() + d_maxSize,
                                d_nameBuf.end());
            }
        }

        bsl::ios::fmtflags flags(stream.flags());
        stream << bsl::left << d_nameBuf;
        stream.flags(flags);
        return stream;
    }
};

// TYPES

/// Struct used by addContext because local types can't be used as template
/// parameters
struct SubcontextInfo {
    const StatContext* d_context_p;
    bool               d_filteredOut;
};

// FUNCTIONS
struct SubcontextInfoComparator {
    StatContextTableInfoProvider::SortFn* d_sort_p;

    // ACCESSORS
    bool operator()(const SubcontextInfo& lhs, const SubcontextInfo& rhs)
    {
        return (*d_sort_p)(lhs.d_context_p, rhs.d_context_p);
    }
};

}  // close anonymous namespace

// ----------------------------------------------
// class StatContextTableInfoProviderCustomColumn
// ----------------------------------------------

// CREATORS
StatContextTableInfoProviderCustomColumn::
    ~StatContextTableInfoProviderCustomColumn()
{
}

// ----------------------------------
// class StatContextTableInfoProvider
// ----------------------------------
// PRIVATE MANIPULATORS
void StatContextTableInfoProvider::addContext(const StatContext* context,
                                              int                level,
                                              bool               isFilteredOut)
{
    RowInfo info;

    if (!isFilteredOut) {
        info.d_context_p = context;
        info.d_valueType = StatContext::DMCST_TOTAL_VALUE;
        info.d_level     = level;
        d_rows.push_back(info);
    }
    else if (!d_considerChildrenOfFilteredContexts) {
        return;
    }

    bdlma::LocalSequentialAllocator<5 * 1024> seqAlloc;

    // First filter anything we don't need
    bool                        haveUnfilteredChildren = false;
    bsl::vector<SubcontextInfo> contexts(&seqAlloc);
    contexts.reserve(context->numSubcontexts());
    StatContextIterator iter = context->subcontextIterator();
    for (; iter; ++iter) {
        contexts.resize(contexts.size() + 1);
        contexts.back().d_context_p = iter;
        if (!d_filter ||
            d_filter(iter, StatContext::DMCST_TOTAL_VALUE, level + 1)) {
            contexts.back().d_filteredOut = false;
            haveUnfilteredChildren        = true;
        }
        else {
            contexts.back().d_filteredOut = true;
        }
    }

    // Add 'direct' row if we have any children left
    if (haveUnfilteredChildren && !isFilteredOut) {
        if (!d_filter ||
            d_filter(context, StatContext::DMCST_DIRECT_VALUE, level)) {
            info.d_valueType = StatContext::DMCST_DIRECT_VALUE;
            info.d_level     = level + 1;
            d_rows.push_back(info);
        }
    }

    // Add 'expired' row if the StatContext has any expired values
    if (context->hasExpiredValues() && !isFilteredOut) {
        if (!d_filter ||
            d_filter(context, StatContext::DMCST_EXPIRED_VALUE, level)) {
            info.d_valueType = StatContext::DMCST_EXPIRED_VALUE;
            info.d_level     = level + 1;
            d_rows.push_back(info);
        }
    }

    // TODO decide if we should have a line for active children total

    // Sort
    if (d_cmp) {
        SubcontextInfoComparator cmp;
        cmp.d_sort_p = &d_cmp;
        bsl::sort(contexts.begin(), contexts.end(), cmp);
    }

    for (size_t i = 0; i < contexts.size(); ++i) {
        addContext(contexts[i].d_context_p,
                   level + 1,
                   contexts[i].d_filteredOut);
    }
}

// PRIVATE ACCESSORS
const StatValue&
StatContextTableInfoProvider::getValue(const RowInfo&    row,
                                       const ColumnInfo& column) const
{
    return row.d_context_p->value(row.d_valueType, column.d_statValueIndex);
}

// CREATORS
StatContextTableInfoProvider::StatContextTableInfoProvider(
    bslma::Allocator* basicAllocator)
: d_defaultIdColumn_p()
, d_context_p(0)
, d_rows(basicAllocator)
, d_title(basicAllocator)
, d_columnGroups(basicAllocator)
, d_columns(basicAllocator)
, d_precision(2)
, d_filter(bsl::allocator_arg, basicAllocator)
, d_cmp(bsl::allocator_arg, basicAllocator)
, d_considerChildrenOfFilteredContexts(false)
, d_printSeparators(true)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

StatContextTableInfoProvider::~StatContextTableInfoProvider()
{
}

// MANIPULATORS
void StatContextTableInfoProvider::update()
{
    d_rows.clear();
    addContext(d_context_p,
               0,
               d_filter &&
                   !d_filter(d_context_p, StatContext::DMCST_TOTAL_VALUE, 0));
}

void StatContextTableInfoProvider::setTitle(const bslstl::StringRef& title)
{
    d_title = title;
}

void StatContextTableInfoProvider::setColumnGroup(
    const bslstl::StringRef& name)
{
    d_columnGroups.push_back(name);
}

void StatContextTableInfoProvider::addDefaultIdColumn(
    const bslstl::StringRef& name,
    int                      maxSize)
{
    if (!d_defaultIdColumn_p) {
        d_defaultIdColumn_p.load(new (*d_allocator_p)
                                     DefaultIdColumn(maxSize, d_allocator_p),
                                 d_allocator_p);
    }

    addColumn(name, d_defaultIdColumn_p.ptr());
}

void StatContextTableInfoProvider::addColumn(const bslstl::StringRef& name,
                                             const CustomColumn*      column)
{
    BSLS_ASSERT(!d_columnGroups.empty());
    d_columns.push_back(
        ColumnInfo(static_cast<int>(d_columnGroups.size()) - 1, name, column));
}

void StatContextTableInfoProvider::addColumn(const bslstl::StringRef& name,
                                             int       statValueIndex,
                                             PrintType printType,
                                             const IntValueFunctor& func)
{
    BSLS_ASSERT(!d_columnGroups.empty());
    d_columns.push_back(ColumnInfo(static_cast<int>(d_columnGroups.size()) - 1,
                                   name,
                                   statValueIndex,
                                   printType,
                                   func));
}

void StatContextTableInfoProvider::addColumn(const bslstl::StringRef& name,
                                             int       statValueIndex,
                                             PrintType printType,
                                             const DoubleValueFunctor& func)
{
    BSLS_ASSERT(!d_columnGroups.empty());
    d_columns.push_back(ColumnInfo(static_cast<int>(d_columnGroups.size()) - 1,
                                   name,
                                   statValueIndex,
                                   printType,
                                   func));
}

void StatContextTableInfoProvider::addColumn(const bslstl::StringRef& name,
                                             int         statValueIndex,
                                             Int0ArgFunc func)
{
    using namespace bdlf::PlaceHolders;
    addColumn(name, statValueIndex, DMCST_INT_VALUE, IntValueFunctor(func));
}

void StatContextTableInfoProvider::addColumn(
    const bslstl::StringRef&           name,
    int                                statValueIndex,
    Int1ArgFunc                        func,
    const StatValue::SnapshotLocation& arg1)
{
    using namespace bdlf::PlaceHolders;
    IntValueFunctor f = bdlf::BindUtil::bind(func, _1, arg1);
    addColumn(name, statValueIndex, DMCST_INT_VALUE, f);
}

void StatContextTableInfoProvider::addColumn(
    const bslstl::StringRef&           name,
    int                                statValueIndex,
    Int2ArgFunc                        func,
    const StatValue::SnapshotLocation& arg1,
    const StatValue::SnapshotLocation& arg2)
{
    using namespace bdlf::PlaceHolders;
    IntValueFunctor f = bdlf::BindUtil::bind(func, _1, arg1, arg2);
    addColumn(name, statValueIndex, DMCST_INT_VALUE, f);
}

void StatContextTableInfoProvider::addColumn(const bslstl::StringRef& name,
                                             int            statValueIndex,
                                             Double0ArgFunc func)
{
    using namespace bdlf::PlaceHolders;
    addColumn(name, statValueIndex, DMCST_INT_VALUE, DoubleValueFunctor(func));
}

void StatContextTableInfoProvider::addColumn(
    const bslstl::StringRef&           name,
    int                                statValueIndex,
    Double1ArgFunc                     func,
    const StatValue::SnapshotLocation& arg1)
{
    using namespace bdlf::PlaceHolders;
    DoubleValueFunctor f = bdlf::BindUtil::bind(func, _1, arg1);
    addColumn(name, statValueIndex, DMCST_INT_VALUE, f);
}

void StatContextTableInfoProvider::addColumn(
    const bslstl::StringRef&           name,
    int                                statValueIndex,
    Double2ArgFunc                     func,
    const StatValue::SnapshotLocation& arg1,
    const StatValue::SnapshotLocation& arg2)
{
    using namespace bdlf::PlaceHolders;
    DoubleValueFunctor f = bdlf::BindUtil::bind(func, _1, arg1, arg2);
    addColumn(name, statValueIndex, DMCST_INT_VALUE, f);
}

// ACCESSORS
int StatContextTableInfoProvider::numRows() const
{
    return static_cast<int>(d_rows.size());
}

int StatContextTableInfoProvider::numColumns(int level) const
{
    if (level == 0) {
        return static_cast<int>(d_columns.size());
    }
    else {
        return static_cast<int>(d_columnGroups.size());
    }
}

bool StatContextTableInfoProvider::hasTitle() const
{
    return 0 != d_title.size();
}

int StatContextTableInfoProvider::numHeaderLevels() const
{
    // Don't include second header row if no column group has a name
    for (size_t i = 0; i < d_columnGroups.size(); ++i) {
        if (d_columnGroups[i].length() > 0) {
            return 2;
        }
    }

    return 1;
}

int StatContextTableInfoProvider::getValueSize(int row, int column) const
{
    const RowInfo&    rowInfo = d_rows[row];
    const ColumnInfo& colInfo = d_columns[column];

    if (colInfo.d_customColumn_p) {
        return colInfo.d_customColumn_p->getValueSize(rowInfo.d_level,
                                                      *rowInfo.d_context_p,
                                                      rowInfo.d_valueType);
    }
    else {
        const StatValue& value = getValue(rowInfo, colInfo);
        if (colInfo.d_intFunc) {
            bsls::Types::Int64 funcValue = colInfo.d_intFunc(value);
            if (funcValue == MAX_INT || funcValue == MIN_INT) {
                return static_cast<int>(NA_STRING.length());
            }
            else if (colInfo.d_printType == DMCST_INT_VALUE) {
                if (d_printSeparators) {
                    return mwcst::PrintUtil::printedValueLengthWithSeparator(
                        funcValue,
                        3);
                }
                else {
                    return mwcst::PrintUtil::printedValueLength(funcValue);
                }
            }
            else {
                return mwcst::PrintUtil::printedTimeIntervalNsLength(
                    funcValue,
                    d_precision);
            }
        }
        else if (colInfo.d_doubleFunc) {
            double funcValue = colInfo.d_doubleFunc(value);
            return mwcst::PrintUtil::printedValueLength(
                       (bsls::Types::Int64)funcValue) +
                   d_precision + 1;
        }
    }

    return 100;
}

bsl::ostream& StatContextTableInfoProvider::printValue(bsl::ostream& stream,
                                                       int           row,
                                                       int           column,
                                                       int /*width*/) const
{
    const RowInfo&    rowInfo = d_rows[row];
    const ColumnInfo& colInfo = d_columns[column];

    if (colInfo.d_customColumn_p) {
        return colInfo.d_customColumn_p->printValue(stream,
                                                    rowInfo.d_level,
                                                    *rowInfo.d_context_p,
                                                    rowInfo.d_valueType);
    }
    else {
        bsl::ios::fmtflags flags(stream.flags());
        const StatValue&   value = getValue(rowInfo, colInfo);
        stream << bsl::right;
        if (colInfo.d_intFunc) {
            bsls::Types::Int64 funcValue = colInfo.d_intFunc(value);
            if (funcValue == MAX_INT || funcValue == MIN_INT) {
                stream << NA_STRING;
            }
            else if (colInfo.d_printType == DMCST_INT_VALUE) {
                if (d_printSeparators) {
                    mwcst::PrintUtil::printValueWithSeparator(stream,
                                                              funcValue,
                                                              3,
                                                              ',');
                }
                else {
                    stream << funcValue;
                }
            }
            else {
                mwcst::PrintUtil::printTimeIntervalNs(stream,
                                                      funcValue,
                                                      d_precision);
            }
        }
        else if (colInfo.d_doubleFunc) {
            double funcValue = colInfo.d_doubleFunc(value);
            stream << bsl::right << bsl::fixed
                   << bsl::setprecision(d_precision) << funcValue;
        }

        stream.flags(flags);
        return stream;  // RETURN
    }
}

int StatContextTableInfoProvider::getHeaderSize(int level, int column) const
{
    if (level == 0) {
        return static_cast<int>(d_columns[column].d_name.length());
    }
    else {
        return static_cast<int>(d_columnGroups[column].length());
    }
}

int StatContextTableInfoProvider::getParentHeader(int /*level*/,
                                                  int column) const
{
    return d_columns[column].d_groupIndex;
}

bsl::ostream&
StatContextTableInfoProvider::printTitle(bsl::ostream& stream) const
{
    return mwcst::PrintUtil::printStringCentered(stream, d_title);
}

bsl::ostream& StatContextTableInfoProvider::printHeader(bsl::ostream& stream,
                                                        int           level,
                                                        int           column,
                                                        int /*width*/) const
{
    if (level == 0) {
        return mwcst::PrintUtil::printStringCentered(stream,
                                                     d_columns[column].d_name);
    }
    else {
        return mwcst::PrintUtil::printStringCentered(stream,
                                                     d_columnGroups[column]);
    }
}

}  // close package namespace
}  // close enterprise namespace
