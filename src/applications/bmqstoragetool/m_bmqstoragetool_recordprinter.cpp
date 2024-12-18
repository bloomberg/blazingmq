#include "m_bmqstoragetool_recordprinter.h"

// MQB
#include <bmqu_alignedprinter.h>
#include <mqbs_filestoreprotocol.h>

// BDE
#include <bsl_vector.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

namespace {

// ==========================
// class RecordDetailsPrinter
// ==========================

template <typename PRINTER_TYPE>
class RecordDetailsPrinter {
  private:
    bsl::ostream&                   d_stream;
    bsl::vector<const char*>        d_fields;
    bslma::ManagedPtr<PRINTER_TYPE> d_printer_mp;
    bslma::Allocator*               d_allocator_p;

    // PRIVATE METHODS

    /// Print the specified message record `rec` to the specified
    /// `printer`.
    void
    printRecord(const MessageDetails::RecordDetails<mqbs::MessageRecord>& rec);
    /// Print the specified confirm record `rec` to the specified
    /// `printer`.
    void
    printRecord(const MessageDetails::RecordDetails<mqbs::ConfirmRecord>& rec);
    /// Print the specified delete record `rec` to the specified
    /// `printer`.
    void printRecord(
        const MessageDetails::RecordDetails<mqbs::DeletionRecord>& rec);

  public:
    // CREATORS
    RecordDetailsPrinter(bsl::ostream& stream, bslma::Allocator* allocator);

    // PUBLIC METHODS

    /// Print this object to the specified `os` stream.
    template <typename RECORD_TYPE>
    void printRecordDetails(
        const MessageDetails::RecordDetails<RECORD_TYPE>& details);
};

template <typename PRINTER_TYPE>
void printMessageDetails(bsl::ostream&         os,
                         const MessageDetails& details,
                         bslma::Allocator*     allocator)
{
    RecordDetailsPrinter<PRINTER_TYPE> printer(os, allocator);

    // Print message record
    printer.printRecordDetails(details.messageRecord());

    // Print confirmation records
    const bsl::vector<MessageDetails::RecordDetails<mqbs::ConfirmRecord> >&
        confirmRecords = details.confirmRecords();
    if (!confirmRecords.empty()) {
        typename bsl::vector<MessageDetails::RecordDetails<
            mqbs::ConfirmRecord> >::const_iterator it = confirmRecords.begin();
        for (; it != confirmRecords.end(); ++it) {
            printer.printRecordDetails(*it);
        }
    }

    // Print deletion record
    if (!details.deleteRecord().isNull()) {
        printer.printRecordDetails(details.deleteRecord().value());
    }
}

// =================
// class JsonPrinter
// =================

template <bool pretty = false, int braceIndent = 0, int fieldIndent = 4>
class JsonPrinter {
  private:
    // DATA
    bsl::ostream&                   d_ostream;
    const bsl::vector<const char*>* d_fields_p;
    unsigned int                    d_counter;

    // NOT IMPLEMENTED
    JsonPrinter(const JsonPrinter&);
    JsonPrinter& operator=(const JsonPrinter&);

  public:
    // CREATORS

    /// Create an instance that will print to the specified `stream` a JSON
    /// object with the specified `fields` with the optionally specified
    /// `indent`.  Behavior is undefined unless `indent` >= 0 and at least one
    /// field is present in the `fields`.
    JsonPrinter(bsl::ostream& stream, const bsl::vector<const char*>* fields);

    ~JsonPrinter();

    // MANIPULATORS

    /// Print the specified `value` to the stream held by this printer
    /// instance.  Behavior is undefined unless there exists a field
    /// corresponding to the `value`.
    template <typename TYPE>
    JsonPrinter& operator<<(const TYPE& value);
};

typedef m_bmqstoragetool::JsonPrinter<false, 0, 0> JsonLinePrinter;
typedef m_bmqstoragetool::JsonPrinter<true, 0, 4>  JsonPrettyPrinter;
typedef bmqu::AlignedPrinter                       HumanReadablePrinter;

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// --------------------
// RecordDetailsPrinter
// --------------------

template <typename PRINTER_TYPE>
RecordDetailsPrinter<PRINTER_TYPE>::RecordDetailsPrinter(
    std::ostream&     stream,
    bslma::Allocator* allocator)
: d_stream(stream)
, d_fields(allocator)
, d_printer_mp()
, d_allocator_p(allocator)
{
    // NOTHING
}

template <typename PRINTER_TYPE>
template <typename RECORD_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecordDetails(
    const MessageDetails::RecordDetails<RECORD_TYPE>& details)
{
    d_fields.clear();
    d_fields.reserve(16);  // max number of fields
    d_fields.push_back("RecordType");
    d_fields.push_back("Index");
    d_fields.push_back("Offset");
    d_fields.push_back("GUID");
    d_fields.push_back("PrimaryLeaseId");
    d_fields.push_back("SequenceNumber");
    d_fields.push_back("Timestamp");
    d_fields.push_back("Epoch");
    d_fields.push_back("QueueKey");

    // It's ok to pass a vector by pointer and push elements after that as
    // we've reserved it's capacity in advance. Hense, no reallocations will
    // happen and the pointer won't get invalidated.
    d_printer_mp.load(new (*d_allocator_p) PRINTER_TYPE(d_stream, &d_fields),
                      d_allocator_p);

    *d_printer_mp << details.d_record.header().type() << details.d_recordIndex
                  << details.d_recordOffset << details.d_record.messageGUID()
                  << details.d_record.header().primaryLeaseId()
                  << details.d_record.header().sequenceNumber();

    bsls::Types::Uint64 epochValue = details.d_record.header().timestamp();
    bdlt::Datetime      datetime;
    const int rc = bdlt::EpochUtil::convertFromTimeT64(&datetime, epochValue);
    if (0 != rc) {
        *d_printer_mp << 0;
    }
    else {
        *d_printer_mp << datetime;
    }
    *d_printer_mp << epochValue;

    bmqu::MemOutStream queueKeyStr(d_allocator_p);
    queueKeyStr << details.d_record.queueKey();
    *d_printer_mp << queueKeyStr.str();
    if (!details.d_queueUri.empty()) {
        d_fields.push_back("QueueUri");
        *d_printer_mp << details.d_queueUri;
    }

    printRecord(details);
    d_printer_mp.reset();
    d_stream << '\n';
}

template <typename PRINTER_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecord(
    const MessageDetails::RecordDetails<mqbs::MessageRecord>& rec)
{
    d_fields.push_back("FileKey");
    d_fields.push_back("RefCount");
    d_fields.push_back("MsgOffsetDwords");
    d_fields.push_back("Crc32c");

    bmqu::MemOutStream fileKeyStr(d_allocator_p);
    fileKeyStr << rec.d_record.fileKey();

    *d_printer_mp << fileKeyStr.str() << rec.d_record.refCount()
                  << rec.d_record.messageOffsetDwords()
                  << rec.d_record.crc32c();
}

template <typename PRINTER_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecord(
    const MessageDetails::RecordDetails<mqbs::ConfirmRecord>& rec)
{
    d_fields.push_back("AppKey");
    if (rec.d_record.appKey().isNull()) {
        *d_printer_mp << "** NULL **";
    }
    else {
        *d_printer_mp << rec.d_record.appKey();
    }

    if (!rec.d_appId.empty()) {
        d_fields.push_back("AppId");
        *d_printer_mp << rec.d_appId;
    }
}

template <typename PRINTER_TYPE>
void RecordDetailsPrinter<PRINTER_TYPE>::printRecord(
    const MessageDetails::RecordDetails<mqbs::DeletionRecord>& rec)
{
    d_fields.push_back("DeletionFlag");
    *d_printer_mp << rec.d_record.deletionRecordFlag();
}

// -----------
// JsonPrinter
// -----------

template <bool pretty, int braceIndent, int fieldIndent>
inline JsonPrinter<pretty, braceIndent, fieldIndent>::JsonPrinter(
    bsl::ostream&                   stream,
    const bsl::vector<const char*>* fields)
: d_ostream(stream)
, d_fields_p(fields)
, d_counter(0)
{
    BSLS_ASSERT_SAFE(0 < d_fields_p->size());
    d_ostream << bsl::setw(braceIndent) << ' ' << '{';
    if (pretty) {
        d_ostream << '\n';
    }
}

template <bool pretty, int braceIndent, int fieldIndent>
inline JsonPrinter<pretty, braceIndent, fieldIndent>::~JsonPrinter()
{
    d_ostream << bsl::setw(braceIndent) << ' ' << "},";
}

template <bool pretty, int braceIndent, int fieldIndent>
template <typename TYPE>
inline JsonPrinter<pretty, braceIndent, fieldIndent>&
JsonPrinter<pretty, braceIndent, fieldIndent>::operator<<(const TYPE& value)
{
    BSLS_ASSERT_SAFE(d_counter < d_fields_p->size());

    if (pretty) {
        d_ostream << bsl::setw(fieldIndent) << ' ';
    }
    d_ostream << '\"' << (*d_fields_p)[d_counter] << "\": \"" << value << '\"';

    ++d_counter;
    if (d_counter < d_fields_p->size()) {
        d_ostream << ',';
    }
    d_ostream << (pretty ? '\n' : ' ');
    return *this;
}

}  // close anonymous namespace

class JsonLinePrintManager : public PrintManager {
    bsl::ostream&     d_stream;
    bslma::Allocator* d_allocator_p;

  public:
    // CREATORS
    JsonLinePrintManager(bsl::ostream& os, bslma::Allocator* allocator);

    ~JsonLinePrintManager() BSLS_KEYWORD_OVERRIDE;

    // PUBLIC METHODS

    void printMessage(const MessageDetails& details) BSLS_KEYWORD_OVERRIDE;
    void printFooter(BSLS_ANNOTATION_UNUSED bsl::size_t foundMessagesCount)
        BSLS_KEYWORD_OVERRIDE
    {
    }
    void printError(const bmqt::MessageGUID& guid) BSLS_KEYWORD_OVERRIDE;
    void printGuid(const bmqt::MessageGUID& guid) BSLS_KEYWORD_OVERRIDE;
};

class JsonPrettyPrintManager : public PrintManager {
    bsl::ostream&     d_stream;
    bslma::Allocator* d_allocator_p;

  public:
    // CREATORS
    JsonPrettyPrintManager(bsl::ostream& os, bslma::Allocator* allocator);

    ~JsonPrettyPrintManager() BSLS_KEYWORD_OVERRIDE;

    // PUBLIC METHODS

    void printMessage(const MessageDetails& details) BSLS_KEYWORD_OVERRIDE;
    void printFooter(BSLS_ANNOTATION_UNUSED bsl::size_t foundMessagesCount)
        BSLS_KEYWORD_OVERRIDE
    {
    }
    void printError(const bmqt::MessageGUID& guid) BSLS_KEYWORD_OVERRIDE;
    void printGuid(const bmqt::MessageGUID& guid) BSLS_KEYWORD_OVERRIDE;
};

class HumanReadablePrintManager : public PrintManager {
    bsl::ostream&     d_stream;
    bslma::Allocator* d_allocator_p;

  public:
    // CREATORS
    HumanReadablePrintManager(bsl::ostream& os, bslma::Allocator* allocator);

    ~HumanReadablePrintManager() BSLS_KEYWORD_OVERRIDE;

    // PUBLIC METHODS

    void printMessage(const MessageDetails& details) BSLS_KEYWORD_OVERRIDE;
    void printFooter(bsl::size_t foundMessagesCount) BSLS_KEYWORD_OVERRIDE;
    void printError(const bmqt::MessageGUID& guid) BSLS_KEYWORD_OVERRIDE;
    void printGuid(const bmqt::MessageGUID& guid) BSLS_KEYWORD_OVERRIDE;
};

JsonLinePrintManager::JsonLinePrintManager(std::ostream&     os,
                                           bslma::Allocator* allocator)
: d_stream(os)
, d_allocator_p(allocator)
{
    d_stream << "[\n";
}

JsonLinePrintManager::~JsonLinePrintManager()
{
    d_stream << "]\n";
}

void JsonLinePrintManager::printMessage(const MessageDetails& details)
{
    printMessageDetails<JsonLinePrinter>(d_stream, details, d_allocator_p);
}

void JsonLinePrintManager::printError(const bmqt::MessageGUID& guid)
{
    d_stream << "{\"Logic error\" : \"guid " << guid << " not found\"},\n";
}

void JsonLinePrintManager::printGuid(const bmqt::MessageGUID& guid)
{
    d_stream << "\"" << guid << "\",\n";
}

JsonPrettyPrintManager::JsonPrettyPrintManager(std::ostream&     os,
                                               bslma::Allocator* allocator)
: d_stream(os)
, d_allocator_p(allocator)
{
    d_stream << "[\n";
}

void JsonPrettyPrintManager::printMessage(const MessageDetails& details)
{
    printMessageDetails<JsonPrettyPrinter>(d_stream, details, d_allocator_p);
}

void JsonPrettyPrintManager::printError(const bmqt::MessageGUID& guid)
{
    d_stream << "{\n\t\"Logic error\" : \"guid " << guid
             << " not found\"\n},\n";
}

void JsonPrettyPrintManager::printGuid(const bmqt::MessageGUID& guid)
{
    d_stream << "\"" << guid << "\",\n";
}

JsonPrettyPrintManager::~JsonPrettyPrintManager()
{
    d_stream << "]\n";
}

HumanReadablePrintManager::HumanReadablePrintManager(
    std::ostream&     os,
    bslma::Allocator* allocator)
: d_stream(os)
, d_allocator_p(allocator)
{
}

HumanReadablePrintManager::~HumanReadablePrintManager()
{
    d_stream << "\n";
}

void HumanReadablePrintManager::printMessage(const MessageDetails& details)
{
    d_stream << "==============================\n\n";
    printMessageDetails<HumanReadablePrinter>(d_stream,
                                              details,
                                              d_allocator_p);
}

void HumanReadablePrintManager::printFooter(bsl::size_t foundMessagesCount)
{
    const char* captionForFound    = " message GUID(s) found.";
    const char* captionForNotFound = "No message GUID found.";
    foundMessagesCount > 0
        ? (d_stream << foundMessagesCount << captionForFound)
        : d_stream << captionForNotFound;
    d_stream << '\n';
}

void HumanReadablePrintManager::printError(const bmqt::MessageGUID& guid)
{
    d_stream << "Logic error : guid " << guid << " not found\n";
}

void HumanReadablePrintManager::printGuid(const bmqt::MessageGUID& guid)
{
    d_stream << guid << '\n';
}

bslma::ManagedPtr<PrintManager> createPrintManager(Parameters::PrintMode mode,
                                                   std::ostream&     stream,
                                                   bslma::Allocator* allocator)
{
    bslma::ManagedPtr<PrintManager> printManager;
    if (mode == Parameters::e_HUMAN) {
        printManager.load(new (*allocator)
                              HumanReadablePrintManager(stream, allocator),
                          allocator);
    }
    else if (mode == Parameters::e_JSON_PRETTY) {
        printManager.load(new (*allocator)
                              JsonPrettyPrintManager(stream, allocator),
                          allocator);
    }
    else if (mode == Parameters::e_JSON_LINE) {
        printManager.load(new (*allocator)
                              JsonLinePrintManager(stream, allocator),
                          allocator);
    }
    return printManager;
}

}  // close package namespace
}  // close enterprise namespace
