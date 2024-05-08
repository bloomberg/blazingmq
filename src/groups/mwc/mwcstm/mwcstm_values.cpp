// Copyright 2024 Bloomberg Finance L.P.
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

// mwcstm_values.cpp             *DO NOT EDIT*             @generated -*-C++-*-

#include <mwcstm_values.h>

#include <bdlat_formattingmode.h>
#include <bdlat_valuetypefunctions.h>
#include <bdlb_print.h>
#include <bdlb_printmethods.h>
#include <bdlb_string.h>

#include <bdlb_nullablevalue.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslim_printer.h>
#include <bsls_assert.h>
#include <bsls_types.h>

#include <bsl_cstring.h>
#include <bsl_iomanip.h>
#include <bsl_limits.h>
#include <bsl_ostream.h>
#include <bsl_utility.h>

namespace BloombergLP {
namespace mwcstm {

// ------------------------------------
// class StatContextConfigurationChoice
// ------------------------------------

// CONSTANTS

const char StatContextConfigurationChoice::CLASS_NAME[] =
    "StatContextConfigurationChoice";

const bdlat_SelectionInfo
    StatContextConfigurationChoice::SELECTION_INFO_ARRAY[] = {
        {SELECTION_ID_ID,
         "id",
         sizeof("id") - 1,
         "",
         bdlat_FormattingMode::e_DEC},
        {SELECTION_ID_NAME,
         "name",
         sizeof("name") - 1,
         "",
         bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_SelectionInfo*
StatContextConfigurationChoice::lookupSelectionInfo(const char* name,
                                                    int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_SelectionInfo& selectionInfo =
            StatContextConfigurationChoice::SELECTION_INFO_ARRAY[i];

        if (nameLength == selectionInfo.d_nameLength &&
            0 == bsl::memcmp(selectionInfo.d_name_p, name, nameLength)) {
            return &selectionInfo;
        }
    }

    return 0;
}

const bdlat_SelectionInfo*
StatContextConfigurationChoice::lookupSelectionInfo(int id)
{
    switch (id) {
    case SELECTION_ID_ID: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_ID];
    case SELECTION_ID_NAME: return &SELECTION_INFO_ARRAY[SELECTION_INDEX_NAME];
    default: return 0;
    }
}

// CREATORS

StatContextConfigurationChoice::StatContextConfigurationChoice(
    const StatContextConfigurationChoice& original,
    bslma::Allocator*                     basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ID: {
        new (d_id.buffer()) bsls::Types::Int64(original.d_id.object());
    } break;
    case SELECTION_ID_NAME: {
        new (d_name.buffer())
            bsl::string(original.d_name.object(), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatContextConfigurationChoice::StatContextConfigurationChoice(
    StatContextConfigurationChoice&& original) noexcept
: d_selectionId(original.d_selectionId),
  d_allocator_p(original.d_allocator_p)
{
    switch (d_selectionId) {
    case SELECTION_ID_ID: {
        new (d_id.buffer())
            bsls::Types::Int64(bsl::move(original.d_id.object()));
    } break;
    case SELECTION_ID_NAME: {
        new (d_name.buffer())
            bsl::string(bsl::move(original.d_name.object()), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}

StatContextConfigurationChoice::StatContextConfigurationChoice(
    StatContextConfigurationChoice&& original,
    bslma::Allocator*                basicAllocator)
: d_selectionId(original.d_selectionId)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    switch (d_selectionId) {
    case SELECTION_ID_ID: {
        new (d_id.buffer())
            bsls::Types::Int64(bsl::move(original.d_id.object()));
    } break;
    case SELECTION_ID_NAME: {
        new (d_name.buffer())
            bsl::string(bsl::move(original.d_name.object()), d_allocator_p);
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }
}
#endif

// MANIPULATORS

StatContextConfigurationChoice& StatContextConfigurationChoice::operator=(
    const StatContextConfigurationChoice& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ID: {
            makeId(rhs.d_id.object());
        } break;
        case SELECTION_ID_NAME: {
            makeName(rhs.d_name.object());
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatContextConfigurationChoice&
StatContextConfigurationChoice::operator=(StatContextConfigurationChoice&& rhs)
{
    if (this != &rhs) {
        switch (rhs.d_selectionId) {
        case SELECTION_ID_ID: {
            makeId(bsl::move(rhs.d_id.object()));
        } break;
        case SELECTION_ID_NAME: {
            makeName(bsl::move(rhs.d_name.object()));
        } break;
        default:
            BSLS_ASSERT(SELECTION_ID_UNDEFINED == rhs.d_selectionId);
            reset();
        }
    }

    return *this;
}
#endif

void StatContextConfigurationChoice::reset()
{
    switch (d_selectionId) {
    case SELECTION_ID_ID: {
        // no destruction required
    } break;
    case SELECTION_ID_NAME: {
        typedef bsl::string Type;
        d_name.object().~Type();
    } break;
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
    }

    d_selectionId = SELECTION_ID_UNDEFINED;
}

int StatContextConfigurationChoice::makeSelection(int selectionId)
{
    switch (selectionId) {
    case SELECTION_ID_ID: {
        makeId();
    } break;
    case SELECTION_ID_NAME: {
        makeName();
    } break;
    case SELECTION_ID_UNDEFINED: {
        reset();
    } break;
    default: return -1;
    }
    return 0;
}

int StatContextConfigurationChoice::makeSelection(const char* name,
                                                  int         nameLength)
{
    const bdlat_SelectionInfo* selectionInfo = lookupSelectionInfo(name,
                                                                   nameLength);
    if (0 == selectionInfo) {
        return -1;
    }

    return makeSelection(selectionInfo->d_id);
}

bsls::Types::Int64& StatContextConfigurationChoice::makeId()
{
    if (SELECTION_ID_ID == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_id.object());
    }
    else {
        reset();
        new (d_id.buffer()) bsls::Types::Int64();
        d_selectionId = SELECTION_ID_ID;
    }

    return d_id.object();
}

bsls::Types::Int64&
StatContextConfigurationChoice::makeId(bsls::Types::Int64 value)
{
    if (SELECTION_ID_ID == d_selectionId) {
        d_id.object() = value;
    }
    else {
        reset();
        new (d_id.buffer()) bsls::Types::Int64(value);
        d_selectionId = SELECTION_ID_ID;
    }

    return d_id.object();
}

bsl::string& StatContextConfigurationChoice::makeName()
{
    if (SELECTION_ID_NAME == d_selectionId) {
        bdlat_ValueTypeFunctions::reset(&d_name.object());
    }
    else {
        reset();
        new (d_name.buffer()) bsl::string(d_allocator_p);
        d_selectionId = SELECTION_ID_NAME;
    }

    return d_name.object();
}

bsl::string& StatContextConfigurationChoice::makeName(const bsl::string& value)
{
    if (SELECTION_ID_NAME == d_selectionId) {
        d_name.object() = value;
    }
    else {
        reset();
        new (d_name.buffer()) bsl::string(value, d_allocator_p);
        d_selectionId = SELECTION_ID_NAME;
    }

    return d_name.object();
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
bsl::string& StatContextConfigurationChoice::makeName(bsl::string&& value)
{
    if (SELECTION_ID_NAME == d_selectionId) {
        d_name.object() = bsl::move(value);
    }
    else {
        reset();
        new (d_name.buffer()) bsl::string(bsl::move(value), d_allocator_p);
        d_selectionId = SELECTION_ID_NAME;
    }

    return d_name.object();
}
#endif

// ACCESSORS

bsl::ostream& StatContextConfigurationChoice::print(bsl::ostream& stream,
                                                    int           level,
                                                    int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    switch (d_selectionId) {
    case SELECTION_ID_ID: {
        printer.printAttribute("id", d_id.object());
    } break;
    case SELECTION_ID_NAME: {
        printer.printAttribute("name", d_name.object());
    } break;
    default: stream << "SELECTION UNDEFINED\n";
    }
    printer.end();
    return stream;
}

const char* StatContextConfigurationChoice::selectionName() const
{
    switch (d_selectionId) {
    case SELECTION_ID_ID:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_ID].name();
    case SELECTION_ID_NAME:
        return SELECTION_INFO_ARRAY[SELECTION_INDEX_NAME].name();
    default:
        BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId);
        return "(* UNDEFINED *)";
    }
}

// -----------------------------------
// class StatContextConfigurationFlags
// -----------------------------------

// CONSTANTS

const char StatContextConfigurationFlags::CLASS_NAME[] =
    "StatContextConfigurationFlags";

const bdlat_EnumeratorInfo
    StatContextConfigurationFlags::ENUMERATOR_INFO_ARRAY[] = {
        {StatContextConfigurationFlags::E_IS_TABLE,
         "e_IS_TABLE",
         sizeof("e_IS_TABLE") - 1,
         ""},
        {StatContextConfigurationFlags::E_STORE_EXPIRED_VALUES,
         "e_STORE_EXPIRED_VALUES",
         sizeof("e_STORE_EXPIRED_VALUES") - 1,
         ""}};

// CLASS METHODS

int StatContextConfigurationFlags::fromInt(
    StatContextConfigurationFlags::Value* result,
    int                                   number)
{
    switch (number) {
    case StatContextConfigurationFlags::E_IS_TABLE:
    case StatContextConfigurationFlags::E_STORE_EXPIRED_VALUES:
        *result = static_cast<StatContextConfigurationFlags::Value>(number);
        return 0;
    default: return -1;
    }
}

int StatContextConfigurationFlags::fromString(
    StatContextConfigurationFlags::Value* result,
    const char*                           string,
    int                                   stringLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            StatContextConfigurationFlags::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<StatContextConfigurationFlags::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* StatContextConfigurationFlags::toString(
    StatContextConfigurationFlags::Value value)
{
    switch (value) {
    case E_IS_TABLE: {
        return "e_IS_TABLE";
    }
    case E_STORE_EXPIRED_VALUES: {
        return "e_STORE_EXPIRED_VALUES";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ----------------------------
// class StatContextUpdateFlags
// ----------------------------

// CONSTANTS

const char StatContextUpdateFlags::CLASS_NAME[] = "StatContextUpdateFlags";

const bdlat_EnumeratorInfo StatContextUpdateFlags::ENUMERATOR_INFO_ARRAY[] = {
    {StatContextUpdateFlags::E_CONTEXT_CREATED,
     "e_CONTEXT_CREATED",
     sizeof("e_CONTEXT_CREATED") - 1,
     ""},
    {StatContextUpdateFlags::E_CONTEXT_DELETED,
     "e_CONTEXT_DELETED",
     sizeof("e_CONTEXT_DELETED") - 1,
     ""}};

// CLASS METHODS

int StatContextUpdateFlags::fromInt(StatContextUpdateFlags::Value* result,
                                    int                            number)
{
    switch (number) {
    case StatContextUpdateFlags::E_CONTEXT_CREATED:
    case StatContextUpdateFlags::E_CONTEXT_DELETED:
        *result = static_cast<StatContextUpdateFlags::Value>(number);
        return 0;
    default: return -1;
    }
}

int StatContextUpdateFlags::fromString(StatContextUpdateFlags::Value* result,
                                       const char*                    string,
                                       int stringLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            StatContextUpdateFlags::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<StatContextUpdateFlags::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char*
StatContextUpdateFlags::toString(StatContextUpdateFlags::Value value)
{
    switch (value) {
    case E_CONTEXT_CREATED: {
        return "e_CONTEXT_CREATED";
    }
    case E_CONTEXT_DELETED: {
        return "e_CONTEXT_DELETED";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ---------------------
// class StatValueFields
// ---------------------

// CONSTANTS

const char StatValueFields::CLASS_NAME[] = "StatValueFields";

const bdlat_EnumeratorInfo StatValueFields::ENUMERATOR_INFO_ARRAY[] = {
    {StatValueFields::E_ABSOLUTE_MIN,
     "e_ABSOLUTE_MIN",
     sizeof("e_ABSOLUTE_MIN") - 1,
     ""},
    {StatValueFields::E_ABSOLUTE_MAX,
     "e_ABSOLUTE_MAX",
     sizeof("e_ABSOLUTE_MAX") - 1,
     ""},
    {StatValueFields::E_MIN, "e_MIN", sizeof("e_MIN") - 1, ""},
    {StatValueFields::E_MAX, "e_MAX", sizeof("e_MAX") - 1, ""},
    {StatValueFields::E_EVENTS, "e_EVENTS", sizeof("e_EVENTS") - 1, ""},
    {StatValueFields::E_SUM, "e_SUM", sizeof("e_SUM") - 1, ""},
    {StatValueFields::E_VALUE, "e_VALUE", sizeof("e_VALUE") - 1, ""},
    {StatValueFields::E_INCREMENTS,
     "e_INCREMENTS",
     sizeof("e_INCREMENTS") - 1,
     ""},
    {StatValueFields::E_DECREMENTS,
     "e_DECREMENTS",
     sizeof("e_DECREMENTS") - 1,
     ""}};

// CLASS METHODS

int StatValueFields::fromInt(StatValueFields::Value* result, int number)
{
    switch (number) {
    case StatValueFields::E_ABSOLUTE_MIN:
    case StatValueFields::E_ABSOLUTE_MAX:
    case StatValueFields::E_MIN:
    case StatValueFields::E_MAX:
    case StatValueFields::E_EVENTS:
    case StatValueFields::E_SUM:
    case StatValueFields::E_VALUE:
    case StatValueFields::E_INCREMENTS:
    case StatValueFields::E_DECREMENTS:
        *result = static_cast<StatValueFields::Value>(number);
        return 0;
    default: return -1;
    }
}

int StatValueFields::fromString(StatValueFields::Value* result,
                                const char*             string,
                                int                     stringLength)
{
    for (int i = 0; i < 9; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            StatValueFields::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<StatValueFields::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* StatValueFields::toString(StatValueFields::Value value)
{
    switch (value) {
    case E_ABSOLUTE_MIN: {
        return "e_ABSOLUTE_MIN";
    }
    case E_ABSOLUTE_MAX: {
        return "e_ABSOLUTE_MAX";
    }
    case E_MIN: {
        return "e_MIN";
    }
    case E_MAX: {
        return "e_MAX";
    }
    case E_EVENTS: {
        return "e_EVENTS";
    }
    case E_SUM: {
        return "e_SUM";
    }
    case E_VALUE: {
        return "e_VALUE";
    }
    case E_INCREMENTS: {
        return "e_INCREMENTS";
    }
    case E_DECREMENTS: {
        return "e_DECREMENTS";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// -------------------
// class StatValueType
// -------------------

// CONSTANTS

const char StatValueType::CLASS_NAME[] = "StatValueType";

const bdlat_EnumeratorInfo StatValueType::ENUMERATOR_INFO_ARRAY[] = {
    {StatValueType::E_CONTINUOUS,
     "e_CONTINUOUS",
     sizeof("e_CONTINUOUS") - 1,
     ""},
    {StatValueType::E_DISCRETE, "e_DISCRETE", sizeof("e_DISCRETE") - 1, ""}};

// CLASS METHODS

int StatValueType::fromInt(StatValueType::Value* result, int number)
{
    switch (number) {
    case StatValueType::E_CONTINUOUS:
    case StatValueType::E_DISCRETE:
        *result = static_cast<StatValueType::Value>(number);
        return 0;
    default: return -1;
    }
}

int StatValueType::fromString(StatValueType::Value* result,
                              const char*           string,
                              int                   stringLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_EnumeratorInfo& enumeratorInfo =
            StatValueType::ENUMERATOR_INFO_ARRAY[i];

        if (stringLength == enumeratorInfo.d_nameLength &&
            0 == bsl::memcmp(enumeratorInfo.d_name_p, string, stringLength)) {
            *result = static_cast<StatValueType::Value>(
                enumeratorInfo.d_value);
            return 0;
        }
    }

    return -1;
}

const char* StatValueType::toString(StatValueType::Value value)
{
    switch (value) {
    case E_CONTINUOUS: {
        return "e_CONTINUOUS";
    }
    case E_DISCRETE: {
        return "e_DISCRETE";
    }
    }

    BSLS_ASSERT(!"invalid enumerator");
    return 0;
}

// ---------------------
// class StatValueUpdate
// ---------------------

// CONSTANTS

const char StatValueUpdate::CLASS_NAME[] = "StatValueUpdate";

const bdlat_AttributeInfo StatValueUpdate::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_FIELD_MASK,
     "fieldMask",
     sizeof("fieldMask") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_FIELDS,
     "fields",
     sizeof("fields") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
StatValueUpdate::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StatValueUpdate::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StatValueUpdate::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_FIELD_MASK:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FIELD_MASK];
    case ATTRIBUTE_ID_FIELDS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FIELDS];
    default: return 0;
    }
}

// CREATORS

StatValueUpdate::StatValueUpdate(bslma::Allocator* basicAllocator)
: d_fields(basicAllocator)
, d_fieldMask()
{
}

StatValueUpdate::StatValueUpdate(const StatValueUpdate& original,
                                 bslma::Allocator*      basicAllocator)
: d_fields(original.d_fields, basicAllocator)
, d_fieldMask(original.d_fieldMask)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatValueUpdate::StatValueUpdate(StatValueUpdate&& original) noexcept
: d_fields(bsl::move(original.d_fields)),
  d_fieldMask(bsl::move(original.d_fieldMask))
{
}

StatValueUpdate::StatValueUpdate(StatValueUpdate&& original,
                                 bslma::Allocator* basicAllocator)
: d_fields(bsl::move(original.d_fields), basicAllocator)
, d_fieldMask(bsl::move(original.d_fieldMask))
{
}
#endif

StatValueUpdate::~StatValueUpdate()
{
}

// MANIPULATORS

StatValueUpdate& StatValueUpdate::operator=(const StatValueUpdate& rhs)
{
    if (this != &rhs) {
        d_fieldMask = rhs.d_fieldMask;
        d_fields    = rhs.d_fields;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatValueUpdate& StatValueUpdate::operator=(StatValueUpdate&& rhs)
{
    if (this != &rhs) {
        d_fieldMask = bsl::move(rhs.d_fieldMask);
        d_fields    = bsl::move(rhs.d_fields);
    }

    return *this;
}
#endif

void StatValueUpdate::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_fieldMask);
    bdlat_ValueTypeFunctions::reset(&d_fields);
}

// ACCESSORS

bsl::ostream& StatValueUpdate::print(bsl::ostream& stream,
                                     int           level,
                                     int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("fieldMask", this->fieldMask());
    printer.printAttribute("fields", this->fields());
    printer.end();
    return stream;
}

// -------------------------
// class StatValueDefinition
// -------------------------

// CONSTANTS

const char StatValueDefinition::CLASS_NAME[] = "StatValueDefinition";

const bdlat_AttributeInfo StatValueDefinition::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_NAME,
     "name",
     sizeof("name") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_TYPE,
     "type",
     sizeof("type") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_HISTORY_SIZES,
     "historySizes",
     sizeof("historySizes") - 1,
     "",
     bdlat_FormattingMode::e_DEC}};

// CLASS METHODS

const bdlat_AttributeInfo*
StatValueDefinition::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StatValueDefinition::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StatValueDefinition::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_NAME: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME];
    case ATTRIBUTE_ID_TYPE: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TYPE];
    case ATTRIBUTE_ID_HISTORY_SIZES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_HISTORY_SIZES];
    default: return 0;
    }
}

// CREATORS

StatValueDefinition::StatValueDefinition(bslma::Allocator* basicAllocator)
: d_historySizes(basicAllocator)
, d_name(basicAllocator)
, d_type(static_cast<StatValueType::Value>(0))
{
}

StatValueDefinition::StatValueDefinition(const StatValueDefinition& original,
                                         bslma::Allocator* basicAllocator)
: d_historySizes(original.d_historySizes, basicAllocator)
, d_name(original.d_name, basicAllocator)
, d_type(original.d_type)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatValueDefinition::StatValueDefinition(StatValueDefinition&& original)
    noexcept : d_historySizes(bsl::move(original.d_historySizes)),
               d_name(bsl::move(original.d_name)),
               d_type(bsl::move(original.d_type))
{
}

StatValueDefinition::StatValueDefinition(StatValueDefinition&& original,
                                         bslma::Allocator*     basicAllocator)
: d_historySizes(bsl::move(original.d_historySizes), basicAllocator)
, d_name(bsl::move(original.d_name), basicAllocator)
, d_type(bsl::move(original.d_type))
{
}
#endif

StatValueDefinition::~StatValueDefinition()
{
}

// MANIPULATORS

StatValueDefinition&
StatValueDefinition::operator=(const StatValueDefinition& rhs)
{
    if (this != &rhs) {
        d_name         = rhs.d_name;
        d_type         = rhs.d_type;
        d_historySizes = rhs.d_historySizes;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatValueDefinition& StatValueDefinition::operator=(StatValueDefinition&& rhs)
{
    if (this != &rhs) {
        d_name         = bsl::move(rhs.d_name);
        d_type         = bsl::move(rhs.d_type);
        d_historySizes = bsl::move(rhs.d_historySizes);
    }

    return *this;
}
#endif

void StatValueDefinition::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_name);
    bdlat_ValueTypeFunctions::reset(&d_type);
    bdlat_ValueTypeFunctions::reset(&d_historySizes);
}

// ACCESSORS

bsl::ostream& StatValueDefinition::print(bsl::ostream& stream,
                                         int           level,
                                         int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("name", this->name());
    printer.printAttribute("type", this->type());
    printer.printAttribute("historySizes", this->historySizes());
    printer.end();
    return stream;
}

// ------------------------------
// class StatContextConfiguration
// ------------------------------

// CONSTANTS

const char StatContextConfiguration::CLASS_NAME[] = "StatContextConfiguration";

const bdlat_AttributeInfo StatContextConfiguration::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_FLAGS,
     "flags",
     sizeof("flags") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CHOICE,
     "Choice",
     sizeof("Choice") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT | bdlat_FormattingMode::e_UNTAGGED},
    {ATTRIBUTE_ID_VALUES,
     "values",
     sizeof("values") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
StatContextConfiguration::lookupAttributeInfo(const char* name, int nameLength)
{
    if (bdlb::String::areEqualCaseless("id", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    if (bdlb::String::areEqualCaseless("name", name, nameLength)) {
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    }

    for (int i = 0; i < 3; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StatContextConfiguration::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo*
StatContextConfiguration::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_FLAGS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS];
    case ATTRIBUTE_ID_CHOICE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE];
    case ATTRIBUTE_ID_VALUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUES];
    default: return 0;
    }
}

// CREATORS

StatContextConfiguration::StatContextConfiguration(
    bslma::Allocator* basicAllocator)
: d_values(basicAllocator)
, d_choice(basicAllocator)
, d_flags()
{
}

StatContextConfiguration::StatContextConfiguration(
    const StatContextConfiguration& original,
    bslma::Allocator*               basicAllocator)
: d_values(original.d_values, basicAllocator)
, d_choice(original.d_choice, basicAllocator)
, d_flags(original.d_flags)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatContextConfiguration::StatContextConfiguration(
    StatContextConfiguration&& original) noexcept
: d_values(bsl::move(original.d_values)),
  d_choice(bsl::move(original.d_choice)),
  d_flags(bsl::move(original.d_flags))
{
}

StatContextConfiguration::StatContextConfiguration(
    StatContextConfiguration&& original,
    bslma::Allocator*          basicAllocator)
: d_values(bsl::move(original.d_values), basicAllocator)
, d_choice(bsl::move(original.d_choice), basicAllocator)
, d_flags(bsl::move(original.d_flags))
{
}
#endif

StatContextConfiguration::~StatContextConfiguration()
{
}

// MANIPULATORS

StatContextConfiguration&
StatContextConfiguration::operator=(const StatContextConfiguration& rhs)
{
    if (this != &rhs) {
        d_flags  = rhs.d_flags;
        d_choice = rhs.d_choice;
        d_values = rhs.d_values;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatContextConfiguration&
StatContextConfiguration::operator=(StatContextConfiguration&& rhs)
{
    if (this != &rhs) {
        d_flags  = bsl::move(rhs.d_flags);
        d_choice = bsl::move(rhs.d_choice);
        d_values = bsl::move(rhs.d_values);
    }

    return *this;
}
#endif

void StatContextConfiguration::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_flags);
    bdlat_ValueTypeFunctions::reset(&d_choice);
    bdlat_ValueTypeFunctions::reset(&d_values);
}

// ACCESSORS

bsl::ostream& StatContextConfiguration::print(bsl::ostream& stream,
                                              int           level,
                                              int spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("flags", this->flags());
    printer.printAttribute("choice", this->choice());
    printer.printAttribute("values", this->values());
    printer.end();
    return stream;
}

// -----------------------
// class StatContextUpdate
// -----------------------

// CONSTANTS

const char StatContextUpdate::CLASS_NAME[] = "StatContextUpdate";

const bdlat_AttributeInfo StatContextUpdate::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_ID, "id", sizeof("id") - 1, "", bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_FLAGS,
     "flags",
     sizeof("flags") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_TIME_STAMP,
     "timeStamp",
     sizeof("timeStamp") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_CONFIGURATION,
     "configuration",
     sizeof("configuration") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_DIRECT_VALUES,
     "directValues",
     sizeof("directValues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_EXPIRED_VALUES,
     "expiredValues",
     sizeof("expiredValues") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT},
    {ATTRIBUTE_ID_SUBCONTEXTS,
     "subcontexts",
     sizeof("subcontexts") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
StatContextUpdate::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 7; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StatContextUpdate::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StatContextUpdate::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_ID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ID];
    case ATTRIBUTE_ID_FLAGS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS];
    case ATTRIBUTE_ID_TIME_STAMP:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIME_STAMP];
    case ATTRIBUTE_ID_CONFIGURATION:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIGURATION];
    case ATTRIBUTE_ID_DIRECT_VALUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DIRECT_VALUES];
    case ATTRIBUTE_ID_EXPIRED_VALUES:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPIRED_VALUES];
    case ATTRIBUTE_ID_SUBCONTEXTS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBCONTEXTS];
    default: return 0;
    }
}

// CREATORS

StatContextUpdate::StatContextUpdate(bslma::Allocator* basicAllocator)
: d_timeStamp()
, d_directValues(basicAllocator)
, d_expiredValues(basicAllocator)
, d_subcontexts(basicAllocator)
, d_configuration(basicAllocator)
, d_flags()
, d_id()
{
}

StatContextUpdate::StatContextUpdate(const StatContextUpdate& original,
                                     bslma::Allocator*        basicAllocator)
: d_timeStamp(original.d_timeStamp)
, d_directValues(original.d_directValues, basicAllocator)
, d_expiredValues(original.d_expiredValues, basicAllocator)
, d_subcontexts(original.d_subcontexts, basicAllocator)
, d_configuration(original.d_configuration, basicAllocator)
, d_flags(original.d_flags)
, d_id(original.d_id)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatContextUpdate::StatContextUpdate(StatContextUpdate&& original) noexcept
: d_timeStamp(bsl::move(original.d_timeStamp)),
  d_directValues(bsl::move(original.d_directValues)),
  d_expiredValues(bsl::move(original.d_expiredValues)),
  d_subcontexts(bsl::move(original.d_subcontexts)),
  d_configuration(bsl::move(original.d_configuration)),
  d_flags(bsl::move(original.d_flags)),
  d_id(bsl::move(original.d_id))
{
}

StatContextUpdate::StatContextUpdate(StatContextUpdate&& original,
                                     bslma::Allocator*   basicAllocator)
: d_timeStamp(bsl::move(original.d_timeStamp))
, d_directValues(bsl::move(original.d_directValues), basicAllocator)
, d_expiredValues(bsl::move(original.d_expiredValues), basicAllocator)
, d_subcontexts(bsl::move(original.d_subcontexts), basicAllocator)
, d_configuration(bsl::move(original.d_configuration), basicAllocator)
, d_flags(bsl::move(original.d_flags))
, d_id(bsl::move(original.d_id))
{
}
#endif

StatContextUpdate::~StatContextUpdate()
{
}

// MANIPULATORS

StatContextUpdate& StatContextUpdate::operator=(const StatContextUpdate& rhs)
{
    if (this != &rhs) {
        d_id            = rhs.d_id;
        d_flags         = rhs.d_flags;
        d_timeStamp     = rhs.d_timeStamp;
        d_configuration = rhs.d_configuration;
        d_directValues  = rhs.d_directValues;
        d_expiredValues = rhs.d_expiredValues;
        d_subcontexts   = rhs.d_subcontexts;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatContextUpdate& StatContextUpdate::operator=(StatContextUpdate&& rhs)
{
    if (this != &rhs) {
        d_id            = bsl::move(rhs.d_id);
        d_flags         = bsl::move(rhs.d_flags);
        d_timeStamp     = bsl::move(rhs.d_timeStamp);
        d_configuration = bsl::move(rhs.d_configuration);
        d_directValues  = bsl::move(rhs.d_directValues);
        d_expiredValues = bsl::move(rhs.d_expiredValues);
        d_subcontexts   = bsl::move(rhs.d_subcontexts);
    }

    return *this;
}
#endif

void StatContextUpdate::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_id);
    bdlat_ValueTypeFunctions::reset(&d_flags);
    bdlat_ValueTypeFunctions::reset(&d_timeStamp);
    bdlat_ValueTypeFunctions::reset(&d_configuration);
    bdlat_ValueTypeFunctions::reset(&d_directValues);
    bdlat_ValueTypeFunctions::reset(&d_expiredValues);
    bdlat_ValueTypeFunctions::reset(&d_subcontexts);
}

// ACCESSORS

bsl::ostream& StatContextUpdate::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("id", this->id());
    printer.printAttribute("flags", this->flags());
    printer.printAttribute("timeStamp", this->timeStamp());
    printer.printAttribute("configuration", this->configuration());
    printer.printAttribute("directValues", this->directValues());
    printer.printAttribute("expiredValues", this->expiredValues());
    printer.printAttribute("subcontexts", this->subcontexts());
    printer.end();
    return stream;
}

// ---------------------------
// class StatContextUpdateList
// ---------------------------

// CONSTANTS

const char StatContextUpdateList::CLASS_NAME[] = "StatContextUpdateList";

const bdlat_AttributeInfo StatContextUpdateList::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_CONTEXTS,
     "contexts",
     sizeof("contexts") - 1,
     "",
     bdlat_FormattingMode::e_DEFAULT}};

// CLASS METHODS

const bdlat_AttributeInfo*
StatContextUpdateList::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 1; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            StatContextUpdateList::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* StatContextUpdateList::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_CONTEXTS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONTEXTS];
    default: return 0;
    }
}

// CREATORS

StatContextUpdateList::StatContextUpdateList(bslma::Allocator* basicAllocator)
: d_contexts(basicAllocator)
{
}

StatContextUpdateList::StatContextUpdateList(
    const StatContextUpdateList& original,
    bslma::Allocator*            basicAllocator)
: d_contexts(original.d_contexts, basicAllocator)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatContextUpdateList::StatContextUpdateList(StatContextUpdateList&& original)
    noexcept : d_contexts(bsl::move(original.d_contexts))
{
}

StatContextUpdateList::StatContextUpdateList(StatContextUpdateList&& original,
                                             bslma::Allocator* basicAllocator)
: d_contexts(bsl::move(original.d_contexts), basicAllocator)
{
}
#endif

StatContextUpdateList::~StatContextUpdateList()
{
}

// MANIPULATORS

StatContextUpdateList&
StatContextUpdateList::operator=(const StatContextUpdateList& rhs)
{
    if (this != &rhs) {
        d_contexts = rhs.d_contexts;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
StatContextUpdateList&
StatContextUpdateList::operator=(StatContextUpdateList&& rhs)
{
    if (this != &rhs) {
        d_contexts = bsl::move(rhs.d_contexts);
    }

    return *this;
}
#endif

void StatContextUpdateList::reset()
{
    bdlat_ValueTypeFunctions::reset(&d_contexts);
}

// ACCESSORS

bsl::ostream& StatContextUpdateList::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("contexts", this->contexts());
    printer.end();
    return stream;
}

}  // close package namespace
}  // close enterprise namespace

// GENERATED BY BLP_BAS_CODEGEN_2024.05.02
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package mwcstm --msgComponent values mwcstm.xsd
