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

// bmqst_basictableinfoprovider.cpp -*-C++-*-
#include <bmqst_basictableinfoprovider.h>

#include <bmqscm_version.h>
#include <bmqst_basetable.h>
#include <bmqst_printutil.h>

#include <bmqst_value.h>

#include <bsl_ios.h>
#include <bsl_limits.h>

namespace BloombergLP {
namespace bmqst {

namespace {

// CONSTANTS
const bslstl::StringRef NA_STRING("N/A");

// FUNCTIONS
int findColumn(bmqst::BaseTable* table, const bslstl::StringRef& columnName)
{
    if (table) {
        for (int i = 0; i < table->numColumns(); ++i) {
            if (table->columnName(i) == columnName) {
                return i;
            }
        }
    }

    return -1;
}

}  // close anonymous namespace

// -----------------------------------------
// class BasicTableInfoProvider_ColumnFormat
// -----------------------------------------

// CREATORS
BasicTableInfoProvider_ColumnFormat::BasicTableInfoProvider_ColumnFormat(
    const bslstl::StringRef& tableColumnName,
    int                      tableColumnIndex,
    const bslstl::StringRef& printColumnName,
    bslma::Allocator*        basicAllocator)
: d_tableColumnName(tableColumnName, basicAllocator)
, d_tableColumnIndex(tableColumnIndex)
, d_printColumnName(printColumnName, basicAllocator)
, d_printSeparators(true)
, d_precision(2)
, d_type(DMCU_DEFAULT)
, d_justification(DMCU_RIGHT)
, d_columnGroupIndex(0)
, d_zeroString(basicAllocator)
, d_extremeString(basicAllocator)
{
}

BasicTableInfoProvider_ColumnFormat::BasicTableInfoProvider_ColumnFormat(
    const BasicTableInfoProvider_ColumnFormat& other,
    bslma::Allocator*                          basicAllocator)
: d_tableColumnName(other.d_tableColumnName, basicAllocator)
, d_tableColumnIndex(other.d_tableColumnIndex)
, d_printColumnName(other.d_printColumnName, basicAllocator)
, d_printSeparators(other.d_printSeparators)
, d_precision(other.d_precision)
, d_type(other.d_type)
, d_justification(other.d_justification)
, d_columnGroupIndex(other.d_columnGroupIndex)
, d_zeroString(other.d_zeroString, basicAllocator)
, d_extremeString(other.d_extremeString, basicAllocator)
{
}

// MANIPULATORS
BasicTableInfoProvider_ColumnFormat&
BasicTableInfoProvider_ColumnFormat::justifyLeft()
{
    d_justification = DMCU_LEFT;
    return *this;
}

BasicTableInfoProvider_ColumnFormat&
BasicTableInfoProvider_ColumnFormat::justifyRight()
{
    d_justification = DMCU_RIGHT;
    return *this;
}

BasicTableInfoProvider_ColumnFormat&
BasicTableInfoProvider_ColumnFormat::justifyCenter()
{
    d_justification = DMCU_CENTER;
    return *this;
}

BasicTableInfoProvider_ColumnFormat&
BasicTableInfoProvider_ColumnFormat::printAsNsTimeInterval()
{
    d_type = DMCU_NS_TIME_INTERVAL;
    return *this;
}

BasicTableInfoProvider_ColumnFormat&
BasicTableInfoProvider_ColumnFormat::printAsMemory()
{
    d_type = DMCU_MEMORY;
    return *this;
}

BasicTableInfoProvider_ColumnFormat&
BasicTableInfoProvider_ColumnFormat::setPrecision(int precision)
{
    d_precision = precision;
    return *this;
}

BasicTableInfoProvider_ColumnFormat&
BasicTableInfoProvider_ColumnFormat::printSeparators(bool value)
{
    d_printSeparators = value;
    return *this;
}

BasicTableInfoProvider_ColumnFormat&
BasicTableInfoProvider_ColumnFormat::zeroString(const bslstl::StringRef& value)
{
    d_zeroString = value;
    return *this;
}

BasicTableInfoProvider_ColumnFormat&
BasicTableInfoProvider_ColumnFormat::extremeValueString(
    const bslstl::StringRef& value)
{
    d_extremeString = value;
    return *this;
}

// ---------------------------------------------
// class BasicTableInfoProvider_ValueSizeVisitor
// ---------------------------------------------

// CREATORS
BasicTableInfoProvider_ValueSizeVisitor::
    BasicTableInfoProvider_ValueSizeVisitor()
: d_fmt_p(0)
{
}

// MANIPULATORS
void BasicTableInfoProvider_ValueSizeVisitor::setFormat(
    const ColumnFormat* fmt)
{
    d_fmt_p = fmt;
}

// ACCESSORS
int BasicTableInfoProvider_ValueSizeVisitor::operator()(
    bsls::Types::Int64 value) const
{
    if (value == bsl::numeric_limits<bsls::Types::Int64>::max() ||
        value == bsl::numeric_limits<bsls::Types::Int64>::min()) {
        if (!d_fmt_p->d_extremeString.isNull()) {
            return static_cast<int>(d_fmt_p->d_extremeString.value().length());
        }
        else {
            return static_cast<int>(NA_STRING.length());
        }
    }
    else if (!d_fmt_p->d_zeroString.isNull() && value == 0) {
        return static_cast<int>(d_fmt_p->d_zeroString.value().length());
    }
    else if (d_fmt_p->d_type == ColumnFormat::DMCU_NS_TIME_INTERVAL) {
        return bmqst::PrintUtil::printedTimeIntervalNsLength(
            value,
            d_fmt_p->d_precision);
    }
    else if (d_fmt_p->d_type == ColumnFormat::DMCU_MEMORY) {
        return bmqst::PrintUtil::printedMemoryLength(value,
                                                     d_fmt_p->d_precision);
    }
    else if (d_fmt_p->d_printSeparators) {
        return bmqst::PrintUtil::printedValueLengthWithSeparator(value, 3);
    }
    else {
        return bmqst::PrintUtil::printedValueLength(value);
    }
}

int BasicTableInfoProvider_ValueSizeVisitor::operator()(int value) const
{
    return operator()((bsls::Types::Int64)value);
}

int BasicTableInfoProvider_ValueSizeVisitor::operator()(
    const bslstl::StringRef& value) const
{
    return static_cast<int>(value.length());
}

int BasicTableInfoProvider_ValueSizeVisitor::operator()(double value) const
{
    if (!d_fmt_p->d_zeroString.isNull() && value == 0.0) {
        return static_cast<int>(d_fmt_p->d_zeroString.value().length());
    }
    else if (d_fmt_p->d_printSeparators) {
        return bmqst::PrintUtil::printedValueLengthWithSeparator(
            value,
            d_fmt_p->d_precision,
            3);
    }

    return bmqst::PrintUtil::printedValueLength(value, d_fmt_p->d_precision);
}

int BasicTableInfoProvider_ValueSizeVisitor::operator()(
    bslmf::Nil /*value*/) const
{
    return 0;
}

// ----------------------------------------------
// class BasicTableInfoProvider_ValuePrintVisitor
// ----------------------------------------------

// CREATORS
BasicTableInfoProvider_ValuePrintVisitor::
    BasicTableInfoProvider_ValuePrintVisitor()
: d_fmt_p(0)
, d_stream_p(0)
{
}

// MANIPULATORS
void BasicTableInfoProvider_ValuePrintVisitor::reset(const ColumnFormat* fmt,
                                                     bsl::ostream* stream)
{
    d_fmt_p    = fmt;
    d_stream_p = stream;
}

// ACCESSORS
int BasicTableInfoProvider_ValuePrintVisitor::operator()(
    bsls::Types::Int64 value) const
{
    if (value == bsl::numeric_limits<bsls::Types::Int64>::max() ||
        value == bsl::numeric_limits<bsls::Types::Int64>::min()) {
        if (!d_fmt_p->d_extremeString.isNull()) {
            (*d_stream_p) << d_fmt_p->d_extremeString.value();
        }
        else {
            (*d_stream_p) << NA_STRING;
        }
    }
    else if (!d_fmt_p->d_zeroString.isNull() && value == 0) {
        (*d_stream_p) << d_fmt_p->d_zeroString;
    }
    else if (d_fmt_p->d_type == ColumnFormat::DMCU_NS_TIME_INTERVAL) {
        bmqst::PrintUtil::printTimeIntervalNs(*d_stream_p,
                                              value,
                                              d_fmt_p->d_precision);
    }
    else if (d_fmt_p->d_type == ColumnFormat::DMCU_MEMORY) {
        bmqst::PrintUtil::printMemory(*d_stream_p,
                                      value,
                                      d_fmt_p->d_precision);
    }
    else if (d_fmt_p->d_printSeparators) {
        bmqst::PrintUtil::printValueWithSeparator(*d_stream_p, value, 3, ',');
    }
    else {
        (*d_stream_p) << value;
    }

    return 0;
}

int BasicTableInfoProvider_ValuePrintVisitor::operator()(int value) const
{
    operator()((bsls::Types::Int64)value);
    return 0;
}

int BasicTableInfoProvider_ValuePrintVisitor::operator()(
    const bslstl::StringRef& value) const
{
    (*d_stream_p) << value;
    return 0;
}

int BasicTableInfoProvider_ValuePrintVisitor::operator()(double value) const
{
    if (!d_fmt_p->d_zeroString.isNull() && value == 0.0) {
        (*d_stream_p) << d_fmt_p->d_zeroString;
    }
    else if (d_fmt_p->d_printSeparators) {
        bmqst::PrintUtil::printValueWithSeparator(*d_stream_p,
                                                  value,
                                                  d_fmt_p->d_precision,
                                                  3,
                                                  ',');
    }
    else {
        (*d_stream_p) << bsl::fixed << bsl::setprecision(d_fmt_p->d_precision)
                      << value;
    }

    return 0;
}

int BasicTableInfoProvider_ValuePrintVisitor::operator()(
    bslmf::Nil /*value*/) const
{
    (*d_stream_p) << "";
    return 0;
}

// ----------------------------
// class BasicTableInfoProvider
// ----------------------------

// CREATORS
BasicTableInfoProvider::BasicTableInfoProvider(
    bslma::Allocator* basicAllocator)
: d_table_p(0)
, d_title(basicAllocator)
, d_columnGroups(basicAllocator)
, d_columns(basicAllocator)
, d_valueSizeVisitor()
, d_valuePrintVisitor()
{
}

BasicTableInfoProvider::BasicTableInfoProvider(
    bmqst::BaseTable* table,
    bslma::Allocator* basicAllocator)
: d_table_p(table)
, d_title(basicAllocator)
, d_columnGroups(basicAllocator)
, d_columns(basicAllocator)
, d_valueSizeVisitor()
, d_valuePrintVisitor()
{
}

// MANIPULATORS
void BasicTableInfoProvider::reset()
{
    d_columnGroups.clear();
    d_columns.clear();
}

void BasicTableInfoProvider::setTable(bmqst::BaseTable* table)
{
    d_table_p = table;
}

void BasicTableInfoProvider::bind()
{
    for (size_t i = 0; i < d_columns.size(); ++i) {
        d_columns[i].d_tableColumnIndex =
            findColumn(d_table_p, d_columns[i].d_tableColumnName);
    }
}

void BasicTableInfoProvider::setTitle(const bslstl::StringRef& title)
{
    d_title = title;
}

void BasicTableInfoProvider::setColumnGroup(const bslstl::StringRef& groupName)
{
    if (!d_columns.empty() && d_columnGroups.empty()) {
        d_columnGroups.push_back("");
    }

    d_columnGroups.push_back(groupName);
}

BasicTableInfoProvider::ColumnFormat&
BasicTableInfoProvider::addColumn(const bslstl::StringRef& tableColumnName,
                                  const bslstl::StringRef& printColumnName)
{
    d_columns.emplace_back(tableColumnName,
                           findColumn(d_table_p, tableColumnName),
                           printColumnName);
    ColumnFormat& column = d_columns.back();

    if (!d_columnGroups.empty()) {
        column.d_columnGroupIndex = static_cast<int>(d_columnGroups.size()) -
                                    1;
    }

    return column;
}

// ACCESSORS

// bmqst::TableInfoProvider
int BasicTableInfoProvider::numRows() const
{
    return d_table_p->numRows();
}

int BasicTableInfoProvider::numColumns(int level) const
{
    if (level == 0) {
        return static_cast<int>(d_columns.size());
    }
    else {
        return static_cast<int>(d_columnGroups.size());
    }
}

bool BasicTableInfoProvider::hasTitle() const
{
    return 0 != d_title.size();
}

int BasicTableInfoProvider::numHeaderLevels() const
{
    return d_columnGroups.empty() ? 1 : 2;
}

int BasicTableInfoProvider::getValueSize(int row, int column) const
{
    const ColumnFormat& fmt = d_columns[column];
    bmqst::Value        value;

    if (fmt.d_tableColumnIndex >= 0) {
        d_table_p->value(&value, row, fmt.d_tableColumnIndex);
    }
    else {
        value.set(bslstl::StringRef("UNBOUND"));
    }

    d_valueSizeVisitor.setFormat(&fmt);
    return value.apply(d_valueSizeVisitor);
}

bsl::ostream& BasicTableInfoProvider::printValue(bsl::ostream& stream,
                                                 int           row,
                                                 int           column,
                                                 int /*width*/) const
{
    const ColumnFormat& fmt = d_columns[column];
    bmqst::Value        value;

    if (fmt.d_tableColumnIndex >= 0) {
        d_table_p->value(&value, row, fmt.d_tableColumnIndex);
    }
    else {
        value.set(bslstl::StringRef("UNBOUND"));
    }

    if (fmt.d_justification == ColumnFormat::DMCU_LEFT) {
        stream << bsl::left;
    }
    else {
        // CENTERED not handled for now.  Whoever needs it should implement it
        stream << bsl::right;
    }

    d_valuePrintVisitor.reset(&fmt, &stream);
    value.apply(d_valuePrintVisitor);
    return stream;
}

int BasicTableInfoProvider::getHeaderSize(int level, int column) const
{
    if (level == 0) {
        return static_cast<int>(d_columns[column].d_printColumnName.length());
    }
    else {
        return static_cast<int>(d_columnGroups[column].length());
    }
}

int BasicTableInfoProvider::getParentHeader(int /*level*/, int column) const
{
    return d_columns[column].d_columnGroupIndex;
}

bsl::ostream& BasicTableInfoProvider::printTitle(bsl::ostream& stream) const
{
    return bmqst::PrintUtil::printStringCentered(stream, d_title);
}

bsl::ostream& BasicTableInfoProvider::printHeader(bsl::ostream& stream,
                                                  int           level,
                                                  int           column,
                                                  int /*width*/) const
{
    if (level == 0) {
        return bmqst::PrintUtil::printStringCentered(
            stream,
            d_columns[column].d_printColumnName);
    }
    else {
        return bmqst::PrintUtil::printStringCentered(stream,
                                                     d_columnGroups[column]);
    }
}

}  // close package namespace
}  // close enterprise namespace
