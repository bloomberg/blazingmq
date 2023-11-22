// m_bmqstoragetool_messages.h        *DO NOT EDIT*        @generated -*-C++-*-
#ifndef INCLUDED_M_BMQSTORAGETOOL_MESSAGES
#define INCLUDED_M_BMQSTORAGETOOL_MESSAGES

//@PURPOSE: Provide value-semantic attribute classes

#include <bslalg_typetraits.h>

#include <bdlat_attributeinfo.h>

#include <bdlat_selectioninfo.h>

#include <bdlat_typetraits.h>

#include <bslh_hash.h>
#include <bsls_objectbuffer.h>

#include <bslma_default.h>

#include <bsls_assert.h>

#include <bsl_string.h>

#include <bsl_vector.h>

#include <bsls_types.h>

#include <bsl_iosfwd.h>
#include <bsl_limits.h>

namespace BloombergLP {

namespace bslma { class Allocator; }

namespace m_bmqstoragetool { class CommandLineParameters; }
namespace m_bmqstoragetool {

                        // ===========================
                        // class CommandLineParameters
                        // ===========================

class CommandLineParameters {

    // INSTANCE DATA
    bsls::Types::Int64        d_timestampGt;
    bsls::Types::Int64        d_timestampLt;
    bsl::vector<bsl::string>  d_guid;
    bsl::vector<bsl::string>  d_queue;
    bsl::string               d_path;
    bsl::string               d_journalFile;
    bsl::string               d_dataFile;
    bsl::string               d_qlistFile;
    int                       d_dumpLimit;
    bool                      d_details;
    bool                      d_dumpPayload;
    bool                      d_summary;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_PATH         = 0
      , ATTRIBUTE_ID_JOURNAL_FILE = 1
      , ATTRIBUTE_ID_DATA_FILE    = 2
      , ATTRIBUTE_ID_QLIST_FILE   = 3
      , ATTRIBUTE_ID_GUID         = 4
      , ATTRIBUTE_ID_QUEUE        = 5
      , ATTRIBUTE_ID_TIMESTAMP_GT = 6
      , ATTRIBUTE_ID_TIMESTAMP_LT = 7
      , ATTRIBUTE_ID_DETAILS      = 8
      , ATTRIBUTE_ID_DUMP_PAYLOAD = 9
      , ATTRIBUTE_ID_DUMP_LIMIT   = 10
      , ATTRIBUTE_ID_SUMMARY      = 11
    };

    enum {
        NUM_ATTRIBUTES = 12
    };

    enum {
        ATTRIBUTE_INDEX_PATH         = 0
      , ATTRIBUTE_INDEX_JOURNAL_FILE = 1
      , ATTRIBUTE_INDEX_DATA_FILE    = 2
      , ATTRIBUTE_INDEX_QLIST_FILE   = 3
      , ATTRIBUTE_INDEX_GUID         = 4
      , ATTRIBUTE_INDEX_QUEUE        = 5
      , ATTRIBUTE_INDEX_TIMESTAMP_GT = 6
      , ATTRIBUTE_INDEX_TIMESTAMP_LT = 7
      , ATTRIBUTE_INDEX_DETAILS      = 8
      , ATTRIBUTE_INDEX_DUMP_PAYLOAD = 9
      , ATTRIBUTE_INDEX_DUMP_LIMIT   = 10
      , ATTRIBUTE_INDEX_SUMMARY      = 11
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const char DEFAULT_INITIALIZER_PATH[];

    static const char DEFAULT_INITIALIZER_JOURNAL_FILE[];

    static const char DEFAULT_INITIALIZER_DATA_FILE[];

    static const char DEFAULT_INITIALIZER_QLIST_FILE[];

    static const bsls::Types::Int64 DEFAULT_INITIALIZER_TIMESTAMP_GT;

    static const bsls::Types::Int64 DEFAULT_INITIALIZER_TIMESTAMP_LT;

    static const bool DEFAULT_INITIALIZER_DETAILS;

    static const bool DEFAULT_INITIALIZER_DUMP_PAYLOAD;

    static const int DEFAULT_INITIALIZER_DUMP_LIMIT;

    static const bool DEFAULT_INITIALIZER_SUMMARY;

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo *lookupAttributeInfo(int id);
        // Return attribute information for the attribute indicated by the
        // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo *lookupAttributeInfo(
                                                       const char *name,
                                                       int         nameLength);
        // Return attribute information for the attribute indicated by the
        // specified 'name' of the specified 'nameLength' if the attribute
        // exists, and 0 otherwise.

    // CREATORS
    explicit CommandLineParameters(bslma::Allocator *basicAllocator = 0);
        // Create an object of type 'CommandLineParameters' having the default
        // value.  Use the optionally specified 'basicAllocator' to supply
        // memory.  If 'basicAllocator' is 0, the currently installed default
        // allocator is used.

    CommandLineParameters(const CommandLineParameters& original,
                          bslma::Allocator *basicAllocator = 0);
        // Create an object of type 'CommandLineParameters' having the value of
        // the specified 'original' object.  Use the optionally specified
        // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
        // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) \
 && defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    CommandLineParameters(CommandLineParameters&& original) noexcept;
        // Create an object of type 'CommandLineParameters' having the value of
        // the specified 'original' object.  After performing this action, the
        // 'original' object will be left in a valid, but unspecified state.

    CommandLineParameters(CommandLineParameters&& original,
                          bslma::Allocator *basicAllocator);
        // Create an object of type 'CommandLineParameters' having the value of
        // the specified 'original' object.  After performing this action, the
        // 'original' object will be left in a valid, but unspecified state.
        // Use the optionally specified 'basicAllocator' to supply memory.  If
        // 'basicAllocator' is 0, the currently installed default allocator is
        // used.
#endif

    ~CommandLineParameters();
        // Destroy this object.

    // MANIPULATORS
    CommandLineParameters& operator=(const CommandLineParameters& rhs);
        // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) \
 && defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    CommandLineParameters& operator=(CommandLineParameters&& rhs);
        // Assign to this object the value of the specified 'rhs' object.
        // After performing this action, the 'rhs' object will be left in a
        // valid, but unspecified state.
#endif

    void reset();
        // Reset this object to the default value (i.e., its value upon
        // default construction).

    template <typename t_MANIPULATOR>
    int manipulateAttributes(t_MANIPULATOR& manipulator);
        // Invoke the specified 'manipulator' sequentially on the address of
        // each (modifiable) attribute of this object, supplying 'manipulator'
        // with the corresponding attribute information structure until such
        // invocation returns a non-zero value.  Return the value from the
        // last invocation of 'manipulator' (i.e., the invocation that
        // terminated the sequence).

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR& manipulator, int id);
        // Invoke the specified 'manipulator' on the address of
        // the (modifiable) attribute indicated by the specified 'id',
        // supplying 'manipulator' with the corresponding attribute
        // information structure.  Return the value returned from the
        // invocation of 'manipulator' if 'id' identifies an attribute of this
        // class, and -1 otherwise.

    template <typename t_MANIPULATOR>
    int manipulateAttribute(t_MANIPULATOR&  manipulator,
                            const char   *name,
                            int           nameLength);
        // Invoke the specified 'manipulator' on the address of
        // the (modifiable) attribute indicated by the specified 'name' of the
        // specified 'nameLength', supplying 'manipulator' with the
        // corresponding attribute information structure.  Return the value
        // returned from the invocation of 'manipulator' if 'name' identifies
        // an attribute of this class, and -1 otherwise.

    bsl::string& path();
        // Return a reference to the modifiable "Path" attribute of this
        // object.

    bsl::string& journalFile();
        // Return a reference to the modifiable "JournalFile" attribute of this
        // object.

    bsl::string& dataFile();
        // Return a reference to the modifiable "DataFile" attribute of this
        // object.

    bsl::string& qlistFile();
        // Return a reference to the modifiable "QlistFile" attribute of this
        // object.

    bsl::vector<bsl::string>& guid();
        // Return a reference to the modifiable "Guid" attribute of this
        // object.

    bsl::vector<bsl::string>& queue();
        // Return a reference to the modifiable "Queue" attribute of this
        // object.

    bsls::Types::Int64& timestampGt();
        // Return a reference to the modifiable "TimestampGt" attribute of this
        // object.

    bsls::Types::Int64& timestampLt();
        // Return a reference to the modifiable "TimestampLt" attribute of this
        // object.

    bool& details();
        // Return a reference to the modifiable "Details" attribute of this
        // object.

    bool& dumpPayload();
        // Return a reference to the modifiable "DumpPayload" attribute of this
        // object.

    int& dumpLimit();
        // Return a reference to the modifiable "DumpLimit" attribute of this
        // object.

    bool& summary();
        // Return a reference to the modifiable "Summary" attribute of this
        // object.

    // ACCESSORS
    bsl::ostream& print(bsl::ostream& stream,
                        int           level          = 0,
                        int           spacesPerLevel = 4) const;
        // Format this object to the specified output 'stream' at the
        // optionally specified indentation 'level' and return a reference to
        // the modifiable 'stream'.  If 'level' is specified, optionally
        // specify 'spacesPerLevel', the number of spaces per indentation level
        // for this and all of its nested objects.  Each line is indented by
        // the absolute value of 'level * spacesPerLevel'.  If 'level' is
        // negative, suppress indentation of the first line.  If
        // 'spacesPerLevel' is negative, suppress line breaks and format the
        // entire output on one line.  If 'stream' is initially invalid, this
        // operation has no effect.  Note that a trailing newline is provided
        // in multiline mode only.

    template <typename t_ACCESSOR>
    int accessAttributes(t_ACCESSOR& accessor) const;
        // Invoke the specified 'accessor' sequentially on each
        // (non-modifiable) attribute of this object, supplying 'accessor'
        // with the corresponding attribute information structure until such
        // invocation returns a non-zero value.  Return the value from the
        // last invocation of 'accessor' (i.e., the invocation that terminated
        // the sequence).

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR& accessor, int id) const;
        // Invoke the specified 'accessor' on the (non-modifiable) attribute
        // of this object indicated by the specified 'id', supplying 'accessor'
        // with the corresponding attribute information structure.  Return the
        // value returned from the invocation of 'accessor' if 'id' identifies
        // an attribute of this class, and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessAttribute(t_ACCESSOR&   accessor,
                        const char *name,
                        int         nameLength) const;
        // Invoke the specified 'accessor' on the (non-modifiable) attribute
        // of this object indicated by the specified 'name' of the specified
        // 'nameLength', supplying 'accessor' with the corresponding attribute
        // information structure.  Return the value returned from the
        // invocation of 'accessor' if 'name' identifies an attribute of this
        // class, and -1 otherwise.

    const bsl::string& path() const;
        // Return a reference offering non-modifiable access to the "Path"
        // attribute of this object.

    const bsl::string& journalFile() const;
        // Return a reference offering non-modifiable access to the
        // "JournalFile" attribute of this object.

    const bsl::string& dataFile() const;
        // Return a reference offering non-modifiable access to the "DataFile"
        // attribute of this object.

    const bsl::string& qlistFile() const;
        // Return a reference offering non-modifiable access to the "QlistFile"
        // attribute of this object.

    const bsl::vector<bsl::string>& guid() const;
        // Return a reference offering non-modifiable access to the "Guid"
        // attribute of this object.

    const bsl::vector<bsl::string>& queue() const;
        // Return a reference offering non-modifiable access to the "Queue"
        // attribute of this object.

    bsls::Types::Int64 timestampGt() const;
        // Return the value of the "TimestampGt" attribute of this object.

    bsls::Types::Int64 timestampLt() const;
        // Return the value of the "TimestampLt" attribute of this object.

    bool details() const;
        // Return the value of the "Details" attribute of this object.

    bool dumpPayload() const;
        // Return the value of the "DumpPayload" attribute of this object.

    int dumpLimit() const;
        // Return the value of the "DumpLimit" attribute of this object.

    bool summary() const;
        // Return the value of the "Summary" attribute of this object.
};

// FREE OPERATORS
inline
bool operator==(const CommandLineParameters& lhs, const CommandLineParameters& rhs);
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects have
    // the same value, and 'false' otherwise.  Two attribute objects have the
    // same value if each respective attribute has the same value.

inline
bool operator!=(const CommandLineParameters& lhs, const CommandLineParameters& rhs);
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects do not
    // have the same value, and 'false' otherwise.  Two attribute objects do
    // not have the same value if one or more respective attributes differ in
    // values.

inline
bsl::ostream& operator<<(bsl::ostream& stream, const CommandLineParameters& rhs);
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.

template <typename t_HASH_ALGORITHM>
void hashAppend(t_HASH_ALGORITHM& hashAlg, const CommandLineParameters& object);
    // Pass the specified 'object' to the specified 'hashAlg'.  This function
    // integrates with the 'bslh' modular hashing system and effectively
    // provides a 'bsl::hash' specialization for 'CommandLineParameters'.

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(m_bmqstoragetool::CommandLineParameters)

// ============================================================================
//                         INLINE FUNCTION DEFINITIONS
// ============================================================================

namespace m_bmqstoragetool {

                        // ---------------------------
                        // class CommandLineParameters
                        // ---------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int CommandLineParameters::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_path, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PATH]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_journalFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_JOURNAL_FILE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_dataFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_FILE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_qlistFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QLIST_FILE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_guid, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_queue, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_timestampGt, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMESTAMP_GT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_timestampLt, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMESTAMP_LT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_details, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DETAILS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_dumpPayload, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_PAYLOAD]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_dumpLimit, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_LIMIT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_summary, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUMMARY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int CommandLineParameters::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
      case ATTRIBUTE_ID_PATH: {
        return manipulator(&d_path, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PATH]);
      }
      case ATTRIBUTE_ID_JOURNAL_FILE: {
        return manipulator(&d_journalFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_JOURNAL_FILE]);
      }
      case ATTRIBUTE_ID_DATA_FILE: {
        return manipulator(&d_dataFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_FILE]);
      }
      case ATTRIBUTE_ID_QLIST_FILE: {
        return manipulator(&d_qlistFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QLIST_FILE]);
      }
      case ATTRIBUTE_ID_GUID: {
        return manipulator(&d_guid, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID]);
      }
      case ATTRIBUTE_ID_QUEUE: {
        return manipulator(&d_queue, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE]);
      }
      case ATTRIBUTE_ID_TIMESTAMP_GT: {
        return manipulator(&d_timestampGt, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMESTAMP_GT]);
      }
      case ATTRIBUTE_ID_TIMESTAMP_LT: {
        return manipulator(&d_timestampLt, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMESTAMP_LT]);
      }
      case ATTRIBUTE_ID_DETAILS: {
        return manipulator(&d_details, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DETAILS]);
      }
      case ATTRIBUTE_ID_DUMP_PAYLOAD: {
        return manipulator(&d_dumpPayload, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_PAYLOAD]);
      }
      case ATTRIBUTE_ID_DUMP_LIMIT: {
        return manipulator(&d_dumpLimit, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_LIMIT]);
      }
      case ATTRIBUTE_ID_SUMMARY: {
        return manipulator(&d_summary, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUMMARY]);
      }
      default:
        return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int CommandLineParameters::manipulateAttribute(
        t_MANIPULATOR& manipulator,
        const char    *name,
        int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo *attributeInfo =
                                         lookupAttributeInfo(name, nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline
bsl::string& CommandLineParameters::path()
{
    return d_path;
}

inline
bsl::string& CommandLineParameters::journalFile()
{
    return d_journalFile;
}

inline
bsl::string& CommandLineParameters::dataFile()
{
    return d_dataFile;
}

inline
bsl::string& CommandLineParameters::qlistFile()
{
    return d_qlistFile;
}

inline
bsl::vector<bsl::string>& CommandLineParameters::guid()
{
    return d_guid;
}

inline
bsl::vector<bsl::string>& CommandLineParameters::queue()
{
    return d_queue;
}

inline
bsls::Types::Int64& CommandLineParameters::timestampGt()
{
    return d_timestampGt;
}

inline
bsls::Types::Int64& CommandLineParameters::timestampLt()
{
    return d_timestampLt;
}

inline
bool& CommandLineParameters::details()
{
    return d_details;
}

inline
bool& CommandLineParameters::dumpPayload()
{
    return d_dumpPayload;
}

inline
int& CommandLineParameters::dumpLimit()
{
    return d_dumpLimit;
}

inline
bool& CommandLineParameters::summary()
{
    return d_summary;
}

// ACCESSORS
template <typename t_ACCESSOR>
int CommandLineParameters::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_path, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PATH]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_journalFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_JOURNAL_FILE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_dataFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_FILE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_qlistFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QLIST_FILE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_guid, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_queue, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_timestampGt, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMESTAMP_GT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_timestampLt, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMESTAMP_LT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_details, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DETAILS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_dumpPayload, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_PAYLOAD]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_dumpLimit, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_LIMIT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_summary, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUMMARY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int CommandLineParameters::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
      case ATTRIBUTE_ID_PATH: {
        return accessor(d_path, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PATH]);
      }
      case ATTRIBUTE_ID_JOURNAL_FILE: {
        return accessor(d_journalFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_JOURNAL_FILE]);
      }
      case ATTRIBUTE_ID_DATA_FILE: {
        return accessor(d_dataFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DATA_FILE]);
      }
      case ATTRIBUTE_ID_QLIST_FILE: {
        return accessor(d_qlistFile, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QLIST_FILE]);
      }
      case ATTRIBUTE_ID_GUID: {
        return accessor(d_guid, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID]);
      }
      case ATTRIBUTE_ID_QUEUE: {
        return accessor(d_queue, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE]);
      }
      case ATTRIBUTE_ID_TIMESTAMP_GT: {
        return accessor(d_timestampGt, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMESTAMP_GT]);
      }
      case ATTRIBUTE_ID_TIMESTAMP_LT: {
        return accessor(d_timestampLt, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TIMESTAMP_LT]);
      }
      case ATTRIBUTE_ID_DETAILS: {
        return accessor(d_details, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DETAILS]);
      }
      case ATTRIBUTE_ID_DUMP_PAYLOAD: {
        return accessor(d_dumpPayload, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_PAYLOAD]);
      }
      case ATTRIBUTE_ID_DUMP_LIMIT: {
        return accessor(d_dumpLimit, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_LIMIT]);
      }
      case ATTRIBUTE_ID_SUMMARY: {
        return accessor(d_summary, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUMMARY]);
      }
      default:
        return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int CommandLineParameters::accessAttribute(
        t_ACCESSOR&  accessor,
        const char  *name,
        int          nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo *attributeInfo =
          lookupAttributeInfo(name, nameLength);
    if (0 == attributeInfo) {
       return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline
const bsl::string& CommandLineParameters::path() const
{
    return d_path;
}

inline
const bsl::string& CommandLineParameters::journalFile() const
{
    return d_journalFile;
}

inline
const bsl::string& CommandLineParameters::dataFile() const
{
    return d_dataFile;
}

inline
const bsl::string& CommandLineParameters::qlistFile() const
{
    return d_qlistFile;
}

inline
const bsl::vector<bsl::string>& CommandLineParameters::guid() const
{
    return d_guid;
}

inline
const bsl::vector<bsl::string>& CommandLineParameters::queue() const
{
    return d_queue;
}

inline
bsls::Types::Int64 CommandLineParameters::timestampGt() const
{
    return d_timestampGt;
}

inline
bsls::Types::Int64 CommandLineParameters::timestampLt() const
{
    return d_timestampLt;
}

inline
bool CommandLineParameters::details() const
{
    return d_details;
}

inline
bool CommandLineParameters::dumpPayload() const
{
    return d_dumpPayload;
}

inline
int CommandLineParameters::dumpLimit() const
{
    return d_dumpLimit;
}

inline
bool CommandLineParameters::summary() const
{
    return d_summary;
}

}  // close package namespace

// FREE FUNCTIONS

inline
bool m_bmqstoragetool::operator==(
        const m_bmqstoragetool::CommandLineParameters& lhs,
        const m_bmqstoragetool::CommandLineParameters& rhs)
{
    return  lhs.path() == rhs.path()
         && lhs.journalFile() == rhs.journalFile()
         && lhs.dataFile() == rhs.dataFile()
         && lhs.qlistFile() == rhs.qlistFile()
         && lhs.guid() == rhs.guid()
         && lhs.queue() == rhs.queue()
         && lhs.timestampGt() == rhs.timestampGt()
         && lhs.timestampLt() == rhs.timestampLt()
         && lhs.details() == rhs.details()
         && lhs.dumpPayload() == rhs.dumpPayload()
         && lhs.dumpLimit() == rhs.dumpLimit()
         && lhs.summary() == rhs.summary();
}

inline
bool m_bmqstoragetool::operator!=(
        const m_bmqstoragetool::CommandLineParameters& lhs,
        const m_bmqstoragetool::CommandLineParameters& rhs)
{
    return !(lhs == rhs);
}

inline
bsl::ostream& m_bmqstoragetool::operator<<(
        bsl::ostream& stream,
        const m_bmqstoragetool::CommandLineParameters& rhs)
{
    return rhs.print(stream, 0, -1);
}

template <typename t_HASH_ALGORITHM>
void m_bmqstoragetool::hashAppend(t_HASH_ALGORITHM& hashAlg, const m_bmqstoragetool::CommandLineParameters& object)
{
    using bslh::hashAppend;
    hashAppend(hashAlg, object.path());
    hashAppend(hashAlg, object.journalFile());
    hashAppend(hashAlg, object.dataFile());
    hashAppend(hashAlg, object.qlistFile());
    hashAppend(hashAlg, object.guid());
    hashAppend(hashAlg, object.queue());
    hashAppend(hashAlg, object.timestampGt());
    hashAppend(hashAlg, object.timestampLt());
    hashAppend(hashAlg, object.details());
    hashAppend(hashAlg, object.dumpPayload());
    hashAppend(hashAlg, object.dumpLimit());
    hashAppend(hashAlg, object.summary());
}

}  // close enterprise namespace
#endif

// GENERATED BY BLP_BAS_CODEGEN_2023.11.11
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization --noIdent --package m_bmqstoragetool --msgComponent messages bmqstoragetoolcmd.xsd
// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright 2023 Bloomberg Finance L.P. All rights reserved.
//      Property of Bloomberg Finance L.P. (BFLP)
//      This software is made available solely pursuant to the
//      terms of a BFLP license agreement which governs its use.
// ------------------------------- END-OF-FILE --------------------------------
