// m_bmqstoragetool_messages.cpp       *DO NOT EDIT*       @generated -*-C++-*-

#include <m_bmqstoragetool_messages.h>

#include <bdlat_formattingmode.h>
#include <bdlat_valuetypefunctions.h>
#include <bdlb_print.h>
#include <bdlb_printmethods.h>
#include <bdlb_string.h>

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
namespace m_bmqstoragetool {

// ---------------------------
// class CommandLineParameters
// ---------------------------

// CONSTANTS

const char CommandLineParameters::CLASS_NAME[] = "CommandLineParameters";

const char CommandLineParameters::DEFAULT_INITIALIZER_PATH[] = "./*";

const char CommandLineParameters::DEFAULT_INITIALIZER_JOURNAL_FILE[] = "";

const char CommandLineParameters::DEFAULT_INITIALIZER_DATA_FILE[] = "";

const char CommandLineParameters::DEFAULT_INITIALIZER_QLIST_FILE[] = "";

const bsls::Types::Int64
    CommandLineParameters::DEFAULT_INITIALIZER_TIMESTAMP_GT = 0;

const bsls::Types::Int64
    CommandLineParameters::DEFAULT_INITIALIZER_TIMESTAMP_LT = 0;

const bool CommandLineParameters::DEFAULT_INITIALIZER_DETAILS = false;

const bool CommandLineParameters::DEFAULT_INITIALIZER_DUMP_PAYLOAD = false;

const int CommandLineParameters::DEFAULT_INITIALIZER_DUMP_LIMIT = 0;

const bool CommandLineParameters::DEFAULT_INITIALIZER_SUMMARY = false;

const bdlat_AttributeInfo CommandLineParameters::ATTRIBUTE_INFO_ARRAY[] = {
    {ATTRIBUTE_ID_PATH,
     "path",
     sizeof("path") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_JOURNAL_FILE,
     "journal-file",
     sizeof("journal-file") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_DATA_FILE,
     "data-file",
     sizeof("data-file") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_QLIST_FILE,
     "qlist-file",
     sizeof("qlist-file") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_GUID,
     "guid",
     sizeof("guid") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_QUEUE,
     "queue",
     sizeof("queue") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_TIMESTAMP_GT,
     "timestamp-gt",
     sizeof("timestamp-gt") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_TIMESTAMP_LT,
     "timestamp-lt",
     sizeof("timestamp-lt") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_DETAILS,
     "details",
     sizeof("details") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_DUMP_PAYLOAD,
     "dump-payload",
     sizeof("dump-payload") - 1,
     "",
     bdlat_FormattingMode::e_TEXT},
    {ATTRIBUTE_ID_DUMP_LIMIT,
     "dump-limit",
     sizeof("dump-limit") - 1,
     "",
     bdlat_FormattingMode::e_DEC},
    {ATTRIBUTE_ID_SUMMARY,
     "summary",
     sizeof("summary") - 1,
     "",
     bdlat_FormattingMode::e_TEXT}};

// CLASS METHODS

const bdlat_AttributeInfo*
CommandLineParameters::lookupAttributeInfo(const char* name, int nameLength)
{
    for (int i = 0; i < 12; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
            CommandLineParameters::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength &&
            0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength)) {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo* CommandLineParameters::lookupAttributeInfo(int id)
{
    switch (id) {
    case ATTRIBUTE_ID_PATH: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PATH];
    case ATTRIBUTE_ID_JOURNAL_FILE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_JOURNAL_FILE];
    case ATTRIBUTE_ID_DATA_FILE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_FILE];
    case ATTRIBUTE_ID_QLIST_FILE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QLIST_FILE];
    case ATTRIBUTE_ID_GUID: return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID];
    case ATTRIBUTE_ID_QUEUE:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE];
    case ATTRIBUTE_ID_TIMESTAMP_GT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMESTAMP_GT];
    case ATTRIBUTE_ID_TIMESTAMP_LT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMESTAMP_LT];
    case ATTRIBUTE_ID_DETAILS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DETAILS];
    case ATTRIBUTE_ID_DUMP_PAYLOAD:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_PAYLOAD];
    case ATTRIBUTE_ID_DUMP_LIMIT:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_LIMIT];
    case ATTRIBUTE_ID_SUMMARY:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUMMARY];
    default: return 0;
    }
}

// CREATORS

CommandLineParameters::CommandLineParameters(bslma::Allocator* basicAllocator)
: d_timestampGt(DEFAULT_INITIALIZER_TIMESTAMP_GT)
, d_timestampLt(DEFAULT_INITIALIZER_TIMESTAMP_LT)
, d_guid(basicAllocator)
, d_queue(basicAllocator)
, d_path(DEFAULT_INITIALIZER_PATH, basicAllocator)
, d_journalFile(DEFAULT_INITIALIZER_JOURNAL_FILE, basicAllocator)
, d_dataFile(DEFAULT_INITIALIZER_DATA_FILE, basicAllocator)
, d_qlistFile(DEFAULT_INITIALIZER_QLIST_FILE, basicAllocator)
, d_dumpLimit(DEFAULT_INITIALIZER_DUMP_LIMIT)
, d_details(DEFAULT_INITIALIZER_DETAILS)
, d_dumpPayload(DEFAULT_INITIALIZER_DUMP_PAYLOAD)
, d_summary(DEFAULT_INITIALIZER_SUMMARY)
{
}

CommandLineParameters::CommandLineParameters(
    const CommandLineParameters& original,
    bslma::Allocator*            basicAllocator)
: d_timestampGt(original.d_timestampGt)
, d_timestampLt(original.d_timestampLt)
, d_guid(original.d_guid, basicAllocator)
, d_queue(original.d_queue, basicAllocator)
, d_path(original.d_path, basicAllocator)
, d_journalFile(original.d_journalFile, basicAllocator)
, d_dataFile(original.d_dataFile, basicAllocator)
, d_qlistFile(original.d_qlistFile, basicAllocator)
, d_dumpLimit(original.d_dumpLimit)
, d_details(original.d_details)
, d_dumpPayload(original.d_dumpPayload)
, d_summary(original.d_summary)
{
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CommandLineParameters::CommandLineParameters(CommandLineParameters&& original)
    noexcept : d_timestampGt(bsl::move(original.d_timestampGt)),
               d_timestampLt(bsl::move(original.d_timestampLt)),
               d_guid(bsl::move(original.d_guid)),
               d_queue(bsl::move(original.d_queue)),
               d_path(bsl::move(original.d_path)),
               d_journalFile(bsl::move(original.d_journalFile)),
               d_dataFile(bsl::move(original.d_dataFile)),
               d_qlistFile(bsl::move(original.d_qlistFile)),
               d_dumpLimit(bsl::move(original.d_dumpLimit)),
               d_details(bsl::move(original.d_details)),
               d_dumpPayload(bsl::move(original.d_dumpPayload)),
               d_summary(bsl::move(original.d_summary))
{
}

CommandLineParameters::CommandLineParameters(CommandLineParameters&& original,
                                             bslma::Allocator* basicAllocator)
: d_timestampGt(bsl::move(original.d_timestampGt))
, d_timestampLt(bsl::move(original.d_timestampLt))
, d_guid(bsl::move(original.d_guid), basicAllocator)
, d_queue(bsl::move(original.d_queue), basicAllocator)
, d_path(bsl::move(original.d_path), basicAllocator)
, d_journalFile(bsl::move(original.d_journalFile), basicAllocator)
, d_dataFile(bsl::move(original.d_dataFile), basicAllocator)
, d_qlistFile(bsl::move(original.d_qlistFile), basicAllocator)
, d_dumpLimit(bsl::move(original.d_dumpLimit))
, d_details(bsl::move(original.d_details))
, d_dumpPayload(bsl::move(original.d_dumpPayload))
, d_summary(bsl::move(original.d_summary))
{
}
#endif

CommandLineParameters::~CommandLineParameters()
{
}

// MANIPULATORS

CommandLineParameters&
CommandLineParameters::operator=(const CommandLineParameters& rhs)
{
    if (this != &rhs) {
        d_path        = rhs.d_path;
        d_journalFile = rhs.d_journalFile;
        d_dataFile    = rhs.d_dataFile;
        d_qlistFile   = rhs.d_qlistFile;
        d_guid        = rhs.d_guid;
        d_queue       = rhs.d_queue;
        d_timestampGt = rhs.d_timestampGt;
        d_timestampLt = rhs.d_timestampLt;
        d_details     = rhs.d_details;
        d_dumpPayload = rhs.d_dumpPayload;
        d_dumpLimit   = rhs.d_dumpLimit;
        d_summary     = rhs.d_summary;
    }

    return *this;
}

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
CommandLineParameters&
CommandLineParameters::operator=(CommandLineParameters&& rhs)
{
    if (this != &rhs) {
        d_path        = bsl::move(rhs.d_path);
        d_journalFile = bsl::move(rhs.d_journalFile);
        d_dataFile    = bsl::move(rhs.d_dataFile);
        d_qlistFile   = bsl::move(rhs.d_qlistFile);
        d_guid        = bsl::move(rhs.d_guid);
        d_queue       = bsl::move(rhs.d_queue);
        d_timestampGt = bsl::move(rhs.d_timestampGt);
        d_timestampLt = bsl::move(rhs.d_timestampLt);
        d_details     = bsl::move(rhs.d_details);
        d_dumpPayload = bsl::move(rhs.d_dumpPayload);
        d_dumpLimit   = bsl::move(rhs.d_dumpLimit);
        d_summary     = bsl::move(rhs.d_summary);
    }

    return *this;
}
#endif

void CommandLineParameters::reset()
{
    d_path        = DEFAULT_INITIALIZER_PATH;
    d_journalFile = DEFAULT_INITIALIZER_JOURNAL_FILE;
    d_dataFile    = DEFAULT_INITIALIZER_DATA_FILE;
    d_qlistFile   = DEFAULT_INITIALIZER_QLIST_FILE;
    bdlat_ValueTypeFunctions::reset(&d_guid);
    bdlat_ValueTypeFunctions::reset(&d_queue);
    d_timestampGt = DEFAULT_INITIALIZER_TIMESTAMP_GT;
    d_timestampLt = DEFAULT_INITIALIZER_TIMESTAMP_LT;
    d_details     = DEFAULT_INITIALIZER_DETAILS;
    d_dumpPayload = DEFAULT_INITIALIZER_DUMP_PAYLOAD;
    d_dumpLimit   = DEFAULT_INITIALIZER_DUMP_LIMIT;
    d_summary     = DEFAULT_INITIALIZER_SUMMARY;
}

// ACCESSORS

bsl::ostream& CommandLineParameters::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("path", this->path());
    printer.printAttribute("journalFile", this->journalFile());
    printer.printAttribute("dataFile", this->dataFile());
    printer.printAttribute("qlistFile", this->qlistFile());
    printer.printAttribute("guid", this->guid());
    printer.printAttribute("queue", this->queue());
    printer.printAttribute("timestampGt", this->timestampGt());
    printer.printAttribute("timestampLt", this->timestampLt());
    printer.printAttribute("details", this->details());
    printer.printAttribute("dumpPayload", this->dumpPayload());
    printer.printAttribute("dumpLimit", this->dumpLimit());
    printer.printAttribute("summary", this->summary());
    printer.end();
    return stream;
}

}  // close package namespace
}  // close enterprise namespace

// GENERATED BY BLP_BAS_CODEGEN_2023.11.11
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package m_bmqstoragetool --msgComponent messages
// bmqstoragetoolcmd.xsd
// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright 2023 Bloomberg Finance L.P. All rights reserved.
//      Property of Bloomberg Finance L.P. (BFLP)
//      This software is made available solely pursuant to the
//      terms of a BFLP license agreement which governs its use.
// ------------------------------- END-OF-FILE --------------------------------
