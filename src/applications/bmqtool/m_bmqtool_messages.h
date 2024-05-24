// Copyright 2014-2024 Bloomberg Finance L.P.
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

// m_bmqtool_messages.h           *DO NOT EDIT*            @generated -*-C++-*-
#ifndef INCLUDED_M_BMQTOOL_MESSAGES
#define INCLUDED_M_BMQTOOL_MESSAGES

//@PURPOSE: Provide value-semantic attribute classes

#include <bslalg_typetraits.h>

#include <bdlat_attributeinfo.h>

#include <bdlat_enumeratorinfo.h>

#include <bdlat_selectioninfo.h>

#include <bdlat_typetraits.h>

#include <bslh_hash.h>
#include <bsls_objectbuffer.h>

#include <bslma_default.h>

#include <bsls_assert.h>

#include <bdlb_nullablevalue.h>

#include <bsl_string.h>

#include <bsl_vector.h>

#include <bsls_types.h>

#include <bsl_iosfwd.h>
#include <bsl_limits.h>

#include <bsl_ostream.h>
#include <bsl_string.h>

namespace BloombergLP {

namespace bslma {
class Allocator;
}

namespace m_bmqtool {
class BatchPostCommand;
}
namespace m_bmqtool {
class CloseQueueCommand;
}
namespace m_bmqtool {
class CloseStorageCommand;
}
namespace m_bmqtool {
class ConfirmCommand;
}
namespace m_bmqtool {
class DataCommandChoice;
}
namespace m_bmqtool {
class DumpQueueCommand;
}
namespace m_bmqtool {
class ListCommand;
}
namespace m_bmqtool {
class ListQueuesCommand;
}
namespace m_bmqtool {
class LoadPostCommand;
}
namespace m_bmqtool {
class MetadataCommand;
}
namespace m_bmqtool {
class OpenStorageCommand;
}
namespace m_bmqtool {
class QlistCommandChoice;
}
namespace m_bmqtool {
class StartCommand;
}
namespace m_bmqtool {
class StopCommand;
}
namespace m_bmqtool {
class Subscription;
}
namespace m_bmqtool {
class ConfigureQueueCommand;
}
namespace m_bmqtool {
class DataCommand;
}
namespace m_bmqtool {
class JournalCommandChoice;
}
namespace m_bmqtool {
class MessageProperty;
}
namespace m_bmqtool {
class OpenQueueCommand;
}
namespace m_bmqtool {
class QlistCommand;
}
namespace m_bmqtool {
class CommandLineParameters;
}
namespace m_bmqtool {
class JournalCommand;
}
namespace m_bmqtool {
class PostCommand;
}
namespace m_bmqtool {
class Command;
}
namespace m_bmqtool {

// ======================
// class BatchPostCommand
// ======================

class BatchPostCommand {
    // INSTANCE DATA
    bsls::Types::Int64       d_eventSize;
    bsls::Types::Int64       d_eventsCount;
    bsl::vector<bsl::string> d_payload;
    bsl::string              d_uri;
    int                      d_msgSize;
    int                      d_postInterval;
    int                      d_postRate;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const BatchPostCommand& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_URI           = 0,
        ATTRIBUTE_ID_PAYLOAD       = 1,
        ATTRIBUTE_ID_MSG_SIZE      = 2,
        ATTRIBUTE_ID_EVENT_SIZE    = 3,
        ATTRIBUTE_ID_EVENTS_COUNT  = 4,
        ATTRIBUTE_ID_POST_INTERVAL = 5,
        ATTRIBUTE_ID_POST_RATE     = 6
    };

    enum { NUM_ATTRIBUTES = 7 };

    enum {
        ATTRIBUTE_INDEX_URI           = 0,
        ATTRIBUTE_INDEX_PAYLOAD       = 1,
        ATTRIBUTE_INDEX_MSG_SIZE      = 2,
        ATTRIBUTE_INDEX_EVENT_SIZE    = 3,
        ATTRIBUTE_INDEX_EVENTS_COUNT  = 4,
        ATTRIBUTE_INDEX_POST_INTERVAL = 5,
        ATTRIBUTE_INDEX_POST_RATE     = 6
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const int DEFAULT_INITIALIZER_MSG_SIZE;

    static const bsls::Types::Int64 DEFAULT_INITIALIZER_EVENT_SIZE;

    static const bsls::Types::Int64 DEFAULT_INITIALIZER_EVENTS_COUNT;

    static const int DEFAULT_INITIALIZER_POST_INTERVAL;

    static const int DEFAULT_INITIALIZER_POST_RATE;

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit BatchPostCommand(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'BatchPostCommand' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    BatchPostCommand(const BatchPostCommand& original,
                     bslma::Allocator*       basicAllocator = 0);
    // Create an object of type 'BatchPostCommand' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    BatchPostCommand(BatchPostCommand&& original) noexcept;
    // Create an object of type 'BatchPostCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    BatchPostCommand(BatchPostCommand&& original,
                     bslma::Allocator*  basicAllocator);
    // Create an object of type 'BatchPostCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~BatchPostCommand();
    // Destroy this object.

    // MANIPULATORS
    BatchPostCommand& operator=(const BatchPostCommand& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    BatchPostCommand& operator=(BatchPostCommand&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& uri();
    // Return a reference to the modifiable "Uri" attribute of this object.

    bsl::vector<bsl::string>& payload();
    // Return a reference to the modifiable "Payload" attribute of this
    // object.

    int& msgSize();
    // Return a reference to the modifiable "MsgSize" attribute of this
    // object.

    bsls::Types::Int64& eventSize();
    // Return a reference to the modifiable "EventSize" attribute of this
    // object.

    bsls::Types::Int64& eventsCount();
    // Return a reference to the modifiable "EventsCount" attribute of this
    // object.

    int& postInterval();
    // Return a reference to the modifiable "PostInterval" attribute of
    // this object.

    int& postRate();
    // Return a reference to the modifiable "PostRate" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& uri() const;
    // Return a reference offering non-modifiable access to the "Uri"
    // attribute of this object.

    const bsl::vector<bsl::string>& payload() const;
    // Return a reference offering non-modifiable access to the "Payload"
    // attribute of this object.

    int msgSize() const;
    // Return the value of the "MsgSize" attribute of this object.

    bsls::Types::Int64 eventSize() const;
    // Return the value of the "EventSize" attribute of this object.

    bsls::Types::Int64 eventsCount() const;
    // Return the value of the "EventsCount" attribute of this object.

    int postInterval() const;
    // Return the value of the "PostInterval" attribute of this object.

    int postRate() const;
    // Return the value of the "PostRate" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const BatchPostCommand& lhs,
                           const BatchPostCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const BatchPostCommand& lhs,
                           const BatchPostCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&           stream,
                                    const BatchPostCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&       hashAlg,
                           const BatchPostCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'BatchPostCommand'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::BatchPostCommand)

namespace m_bmqtool {

// =======================
// class CloseQueueCommand
// =======================

class CloseQueueCommand {
    // INSTANCE DATA
    bsl::string d_uri;
    bool        d_async;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_URI = 0, ATTRIBUTE_ID_ASYNC = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_URI = 0, ATTRIBUTE_INDEX_ASYNC = 1 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_ASYNC;

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit CloseQueueCommand(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'CloseQueueCommand' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    CloseQueueCommand(const CloseQueueCommand& original,
                      bslma::Allocator*        basicAllocator = 0);
    // Create an object of type 'CloseQueueCommand' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    CloseQueueCommand(CloseQueueCommand&& original) noexcept;
    // Create an object of type 'CloseQueueCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    CloseQueueCommand(CloseQueueCommand&& original,
                      bslma::Allocator*   basicAllocator);
    // Create an object of type 'CloseQueueCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~CloseQueueCommand();
    // Destroy this object.

    // MANIPULATORS
    CloseQueueCommand& operator=(const CloseQueueCommand& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    CloseQueueCommand& operator=(CloseQueueCommand&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& uri();
    // Return a reference to the modifiable "Uri" attribute of this object.

    bool& async();
    // Return a reference to the modifiable "Async" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& uri() const;
    // Return a reference offering non-modifiable access to the "Uri"
    // attribute of this object.

    bool async() const;
    // Return the value of the "Async" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const CloseQueueCommand& lhs,
                           const CloseQueueCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.uri() == rhs.uri() && lhs.async() == rhs.async();
    }

    friend bool operator!=(const CloseQueueCommand& lhs,
                           const CloseQueueCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const CloseQueueCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const CloseQueueCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'CloseQueueCommand'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.uri());
        hashAppend(hashAlg, object.async());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::CloseQueueCommand)

namespace m_bmqtool {

// =========================
// class CloseStorageCommand
// =========================

class CloseStorageCommand {
    // INSTANCE DATA

  public:
    // TYPES
    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS

    // MANIPULATORS
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    // HIDDEN FRIENDS
    friend bool operator==(const CloseStorageCommand&,
                           const CloseStorageCommand&)
    // Returns 'true' as this type has no attributes and so all objects of
    // this type are considered equal.
    {
        return true;
    }

    friend bool operator!=(const CloseStorageCommand& lhs,
                           const CloseStorageCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&              stream,
                                    const CloseStorageCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&, const CloseStorageCommand&)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'CloseStorageCommand'.
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(m_bmqtool::CloseStorageCommand)

namespace m_bmqtool {

// ====================
// class ConfirmCommand
// ====================

class ConfirmCommand {
    // INSTANCE DATA
    bsl::string d_uri;
    bsl::string d_guid;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_URI = 0, ATTRIBUTE_ID_GUID = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_URI = 0, ATTRIBUTE_INDEX_GUID = 1 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit ConfirmCommand(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'ConfirmCommand' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    ConfirmCommand(const ConfirmCommand& original,
                   bslma::Allocator*     basicAllocator = 0);
    // Create an object of type 'ConfirmCommand' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ConfirmCommand(ConfirmCommand&& original) noexcept;
    // Create an object of type 'ConfirmCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    ConfirmCommand(ConfirmCommand&&  original,
                   bslma::Allocator* basicAllocator);
    // Create an object of type 'ConfirmCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~ConfirmCommand();
    // Destroy this object.

    // MANIPULATORS
    ConfirmCommand& operator=(const ConfirmCommand& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ConfirmCommand& operator=(ConfirmCommand&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& uri();
    // Return a reference to the modifiable "Uri" attribute of this object.

    bsl::string& guid();
    // Return a reference to the modifiable "Guid" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& uri() const;
    // Return a reference offering non-modifiable access to the "Uri"
    // attribute of this object.

    const bsl::string& guid() const;
    // Return a reference offering non-modifiable access to the "Guid"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const ConfirmCommand& lhs,
                           const ConfirmCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.uri() == rhs.uri() && lhs.guid() == rhs.guid();
    }

    friend bool operator!=(const ConfirmCommand& lhs,
                           const ConfirmCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&         stream,
                                    const ConfirmCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&     hashAlg,
                           const ConfirmCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ConfirmCommand'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.uri());
        hashAppend(hashAlg, object.guid());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::ConfirmCommand)

namespace m_bmqtool {

// =======================
// class DataCommandChoice
// =======================

class DataCommandChoice {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<bsls::Types::Uint64> d_n;
        bsls::ObjectBuffer<bsls::Types::Uint64> d_next;
        bsls::ObjectBuffer<bsls::Types::Uint64> d_p;
        bsls::ObjectBuffer<bsls::Types::Uint64> d_prev;
        bsls::ObjectBuffer<bsls::Types::Uint64> d_r;
        bsls::ObjectBuffer<bsls::Types::Uint64> d_record;
        bsls::ObjectBuffer<int>                 d_list;
        bsls::ObjectBuffer<int>                 d_l;
    };

    int d_selectionId;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const DataCommandChoice& rhs) const;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED = -1,
        SELECTION_ID_N         = 0,
        SELECTION_ID_NEXT      = 1,
        SELECTION_ID_P         = 2,
        SELECTION_ID_PREV      = 3,
        SELECTION_ID_R         = 4,
        SELECTION_ID_RECORD    = 5,
        SELECTION_ID_LIST      = 6,
        SELECTION_ID_L         = 7
    };

    enum { NUM_SELECTIONS = 8 };

    enum {
        SELECTION_INDEX_N      = 0,
        SELECTION_INDEX_NEXT   = 1,
        SELECTION_INDEX_P      = 2,
        SELECTION_INDEX_PREV   = 3,
        SELECTION_INDEX_R      = 4,
        SELECTION_INDEX_RECORD = 5,
        SELECTION_INDEX_LIST   = 6,
        SELECTION_INDEX_L      = 7
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_SelectionInfo SELECTION_INFO_ARRAY[];

    // CLASS METHODS
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);
    // Return selection information for the selection indicated by the
    // specified 'id' if the selection exists, and 0 otherwise.

    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);
    // Return selection information for the selection indicated by the
    // specified 'name' of the specified 'nameLength' if the selection
    // exists, and 0 otherwise.

    // CREATORS
    DataCommandChoice();
    // Create an object of type 'DataCommandChoice' having the default
    // value.

    DataCommandChoice(const DataCommandChoice& original);
    // Create an object of type 'DataCommandChoice' having the value of the
    // specified 'original' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DataCommandChoice(DataCommandChoice&& original) noexcept;
    // Create an object of type 'DataCommandChoice' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
#endif

    ~DataCommandChoice();
    // Destroy this object.

    // MANIPULATORS
    DataCommandChoice& operator=(const DataCommandChoice& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DataCommandChoice& operator=(DataCommandChoice&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon default
    // construction).

    int makeSelection(int selectionId);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'selectionId'.  Return 0 on success, and
    // non-zero value otherwise (i.e., the selection is not found).

    int makeSelection(const char* name, int nameLength);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'name' of the specified 'nameLength'.
    // Return 0 on success, and non-zero value otherwise (i.e., the
    // selection is not found).

    bsls::Types::Uint64& makeN();
    bsls::Types::Uint64& makeN(bsls::Types::Uint64 value);
    // Set the value of this object to be a "N" value.  Optionally specify
    // the 'value' of the "N".  If 'value' is not specified, the default
    // "N" value is used.

    bsls::Types::Uint64& makeNext();
    bsls::Types::Uint64& makeNext(bsls::Types::Uint64 value);
    // Set the value of this object to be a "Next" value.  Optionally
    // specify the 'value' of the "Next".  If 'value' is not specified, the
    // default "Next" value is used.

    bsls::Types::Uint64& makeP();
    bsls::Types::Uint64& makeP(bsls::Types::Uint64 value);
    // Set the value of this object to be a "P" value.  Optionally specify
    // the 'value' of the "P".  If 'value' is not specified, the default
    // "P" value is used.

    bsls::Types::Uint64& makePrev();
    bsls::Types::Uint64& makePrev(bsls::Types::Uint64 value);
    // Set the value of this object to be a "Prev" value.  Optionally
    // specify the 'value' of the "Prev".  If 'value' is not specified, the
    // default "Prev" value is used.

    bsls::Types::Uint64& makeR();
    bsls::Types::Uint64& makeR(bsls::Types::Uint64 value);
    // Set the value of this object to be a "R" value.  Optionally specify
    // the 'value' of the "R".  If 'value' is not specified, the default
    // "R" value is used.

    bsls::Types::Uint64& makeRecord();
    bsls::Types::Uint64& makeRecord(bsls::Types::Uint64 value);
    // Set the value of this object to be a "Record" value.  Optionally
    // specify the 'value' of the "Record".  If 'value' is not specified,
    // the default "Record" value is used.

    int& makeList();
    int& makeList(int value);
    // Set the value of this object to be a "List" value.  Optionally
    // specify the 'value' of the "List".  If 'value' is not specified, the
    // default "List" value is used.

    int& makeL();
    int& makeL(int value);
    // Set the value of this object to be a "L" value.  Optionally specify
    // the 'value' of the "L".  If 'value' is not specified, the default
    // "L" value is used.

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    bsls::Types::Uint64& n();
    // Return a reference to the modifiable "N" selection of this object if
    // "N" is the current selection.  The behavior is undefined unless "N"
    // is the selection of this object.

    bsls::Types::Uint64& next();
    // Return a reference to the modifiable "Next" selection of this object
    // if "Next" is the current selection.  The behavior is undefined
    // unless "Next" is the selection of this object.

    bsls::Types::Uint64& p();
    // Return a reference to the modifiable "P" selection of this object if
    // "P" is the current selection.  The behavior is undefined unless "P"
    // is the selection of this object.

    bsls::Types::Uint64& prev();
    // Return a reference to the modifiable "Prev" selection of this object
    // if "Prev" is the current selection.  The behavior is undefined
    // unless "Prev" is the selection of this object.

    bsls::Types::Uint64& r();
    // Return a reference to the modifiable "R" selection of this object if
    // "R" is the current selection.  The behavior is undefined unless "R"
    // is the selection of this object.

    bsls::Types::Uint64& record();
    // Return a reference to the modifiable "Record" selection of this
    // object if "Record" is the current selection.  The behavior is
    // undefined unless "Record" is the selection of this object.

    int& list();
    // Return a reference to the modifiable "List" selection of this object
    // if "List" is the current selection.  The behavior is undefined
    // unless "List" is the selection of this object.

    int& l();
    // Return a reference to the modifiable "L" selection of this object if
    // "L" is the current selection.  The behavior is undefined unless "L"
    // is the selection of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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

    int selectionId() const;
    // Return the id of the current selection if the selection is defined,
    // and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' on the non-modifiable selection,
    // supplying 'accessor' with the corresponding selection information
    // structure.  Return the value returned from the invocation of
    // 'accessor' if this object has a defined selection, and -1 otherwise.

    const bsls::Types::Uint64& n() const;
    // Return a reference to the non-modifiable "N" selection of this
    // object if "N" is the current selection.  The behavior is undefined
    // unless "N" is the selection of this object.

    const bsls::Types::Uint64& next() const;
    // Return a reference to the non-modifiable "Next" selection of this
    // object if "Next" is the current selection.  The behavior is
    // undefined unless "Next" is the selection of this object.

    const bsls::Types::Uint64& p() const;
    // Return a reference to the non-modifiable "P" selection of this
    // object if "P" is the current selection.  The behavior is undefined
    // unless "P" is the selection of this object.

    const bsls::Types::Uint64& prev() const;
    // Return a reference to the non-modifiable "Prev" selection of this
    // object if "Prev" is the current selection.  The behavior is
    // undefined unless "Prev" is the selection of this object.

    const bsls::Types::Uint64& r() const;
    // Return a reference to the non-modifiable "R" selection of this
    // object if "R" is the current selection.  The behavior is undefined
    // unless "R" is the selection of this object.

    const bsls::Types::Uint64& record() const;
    // Return a reference to the non-modifiable "Record" selection of this
    // object if "Record" is the current selection.  The behavior is
    // undefined unless "Record" is the selection of this object.

    const int& list() const;
    // Return a reference to the non-modifiable "List" selection of this
    // object if "List" is the current selection.  The behavior is
    // undefined unless "List" is the selection of this object.

    const int& l() const;
    // Return a reference to the non-modifiable "L" selection of this
    // object if "L" is the current selection.  The behavior is undefined
    // unless "L" is the selection of this object.

    bool isNValue() const;
    // Return 'true' if the value of this object is a "N" value, and return
    // 'false' otherwise.

    bool isNextValue() const;
    // Return 'true' if the value of this object is a "Next" value, and
    // return 'false' otherwise.

    bool isPValue() const;
    // Return 'true' if the value of this object is a "P" value, and return
    // 'false' otherwise.

    bool isPrevValue() const;
    // Return 'true' if the value of this object is a "Prev" value, and
    // return 'false' otherwise.

    bool isRValue() const;
    // Return 'true' if the value of this object is a "R" value, and return
    // 'false' otherwise.

    bool isRecordValue() const;
    // Return 'true' if the value of this object is a "Record" value, and
    // return 'false' otherwise.

    bool isListValue() const;
    // Return 'true' if the value of this object is a "List" value, and
    // return 'false' otherwise.

    bool isLValue() const;
    // Return 'true' if the value of this object is a "L" value, and return
    // 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const DataCommandChoice& lhs,
                           const DataCommandChoice& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'DataCommandChoice' objects have
    // the same value if either the selections in both objects have the
    // same ids and the same values, or both selections are undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const DataCommandChoice& lhs,
                           const DataCommandChoice& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const DataCommandChoice& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&        hashAlg,
                           const DataCommandChoice& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'DataCommandChoice'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_BITWISEMOVEABLE_TRAITS(m_bmqtool::DataCommandChoice)

namespace m_bmqtool {

// ======================
// class DumpQueueCommand
// ======================

class DumpQueueCommand {
    // INSTANCE DATA
    bsl::string d_uri;
    bsl::string d_key;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_URI = 0, ATTRIBUTE_ID_KEY = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_URI = 0, ATTRIBUTE_INDEX_KEY = 1 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit DumpQueueCommand(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'DumpQueueCommand' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    DumpQueueCommand(const DumpQueueCommand& original,
                     bslma::Allocator*       basicAllocator = 0);
    // Create an object of type 'DumpQueueCommand' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DumpQueueCommand(DumpQueueCommand&& original) noexcept;
    // Create an object of type 'DumpQueueCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    DumpQueueCommand(DumpQueueCommand&& original,
                     bslma::Allocator*  basicAllocator);
    // Create an object of type 'DumpQueueCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~DumpQueueCommand();
    // Destroy this object.

    // MANIPULATORS
    DumpQueueCommand& operator=(const DumpQueueCommand& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DumpQueueCommand& operator=(DumpQueueCommand&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& uri();
    // Return a reference to the modifiable "Uri" attribute of this object.

    bsl::string& key();
    // Return a reference to the modifiable "Key" attribute of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& uri() const;
    // Return a reference offering non-modifiable access to the "Uri"
    // attribute of this object.

    const bsl::string& key() const;
    // Return a reference offering non-modifiable access to the "Key"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const DumpQueueCommand& lhs,
                           const DumpQueueCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.uri() == rhs.uri() && lhs.key() == rhs.key();
    }

    friend bool operator!=(const DumpQueueCommand& lhs,
                           const DumpQueueCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&           stream,
                                    const DumpQueueCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&       hashAlg,
                           const DumpQueueCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'DumpQueueCommand'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.uri());
        hashAppend(hashAlg, object.key());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::DumpQueueCommand)

namespace m_bmqtool {

// ==============================
// class JournalCommandChoiceType
// ==============================

struct JournalCommandChoiceType {
  public:
    // TYPES
    enum Value { CONFIRM = 0, DELETE = 1, JOP = 2, MESSAGE = 3, QOP = 4 };

    enum { NUM_ENUMERATORS = 5 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS
    static const char* toString(Value value);
    // Return the string representation exactly matching the enumerator
    // name corresponding to the specified enumeration 'value'.

    static int fromString(Value* result, const char* string, int stringLength);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string' of the specified 'stringLength'.  Return 0 on
    // success, and a non-zero value with no effect on 'result' otherwise
    // (i.e., 'string' does not match any enumerator).

    static int fromString(Value* result, const bsl::string& string);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'string' does not match any
    // enumerator).

    static int fromInt(Value* result, int number);
    // Load into the specified 'result' the enumerator matching the
    // specified 'number'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'number' does not match any
    // enumerator).

    static bsl::ostream& print(bsl::ostream& stream, Value value);
    // Write to the specified 'stream' the string representation of
    // the specified enumeration 'value'.  Return a reference to
    // the modifiable 'stream'.

    // HIDDEN FRIENDS
    friend bsl::ostream& operator<<(bsl::ostream& stream, Value rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return JournalCommandChoiceType::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(m_bmqtool::JournalCommandChoiceType)

namespace m_bmqtool {

// =================
// class ListCommand
// =================

class ListCommand {
    // INSTANCE DATA
    bdlb::NullableValue<bsl::string> d_uri;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_URI = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_URI = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit ListCommand(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'ListCommand' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    ListCommand(const ListCommand& original,
                bslma::Allocator*  basicAllocator = 0);
    // Create an object of type 'ListCommand' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ListCommand(ListCommand&& original) noexcept;
    // Create an object of type 'ListCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    ListCommand(ListCommand&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'ListCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~ListCommand();
    // Destroy this object.

    // MANIPULATORS
    ListCommand& operator=(const ListCommand& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ListCommand& operator=(ListCommand&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bdlb::NullableValue<bsl::string>& uri();
    // Return a reference to the modifiable "Uri" attribute of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bdlb::NullableValue<bsl::string>& uri() const;
    // Return a reference offering non-modifiable access to the "Uri"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const ListCommand& lhs, const ListCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.uri() == rhs.uri();
    }

    friend bool operator!=(const ListCommand& lhs, const ListCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&      stream,
                                    const ListCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&  hashAlg,
                           const ListCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'ListCommand'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.uri());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::ListCommand)

namespace m_bmqtool {

// =======================
// class ListQueuesCommand
// =======================

class ListQueuesCommand {
    // INSTANCE DATA

  public:
    // TYPES
    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS

    // MANIPULATORS
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    // HIDDEN FRIENDS
    friend bool operator==(const ListQueuesCommand&, const ListQueuesCommand&)
    // Returns 'true' as this type has no attributes and so all objects of
    // this type are considered equal.
    {
        return true;
    }

    friend bool operator!=(const ListQueuesCommand& lhs,
                           const ListQueuesCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&            stream,
                                    const ListQueuesCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&, const ListQueuesCommand&)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ListQueuesCommand'.
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(m_bmqtool::ListQueuesCommand)

namespace m_bmqtool {

// =====================
// class LoadPostCommand
// =====================

class LoadPostCommand {
    // INSTANCE DATA
    bsl::string d_uri;
    bsl::string d_file;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_URI = 0, ATTRIBUTE_ID_FILE = 1 };

    enum { NUM_ATTRIBUTES = 2 };

    enum { ATTRIBUTE_INDEX_URI = 0, ATTRIBUTE_INDEX_FILE = 1 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit LoadPostCommand(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'LoadPostCommand' having the default value.
    //  Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    LoadPostCommand(const LoadPostCommand& original,
                    bslma::Allocator*      basicAllocator = 0);
    // Create an object of type 'LoadPostCommand' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    LoadPostCommand(LoadPostCommand&& original) noexcept;
    // Create an object of type 'LoadPostCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    LoadPostCommand(LoadPostCommand&& original,
                    bslma::Allocator* basicAllocator);
    // Create an object of type 'LoadPostCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~LoadPostCommand();
    // Destroy this object.

    // MANIPULATORS
    LoadPostCommand& operator=(const LoadPostCommand& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    LoadPostCommand& operator=(LoadPostCommand&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& uri();
    // Return a reference to the modifiable "Uri" attribute of this object.

    bsl::string& file();
    // Return a reference to the modifiable "File" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& uri() const;
    // Return a reference offering non-modifiable access to the "Uri"
    // attribute of this object.

    const bsl::string& file() const;
    // Return a reference offering non-modifiable access to the "File"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const LoadPostCommand& lhs,
                           const LoadPostCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.uri() == rhs.uri() && lhs.file() == rhs.file();
    }

    friend bool operator!=(const LoadPostCommand& lhs,
                           const LoadPostCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&          stream,
                                    const LoadPostCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&      hashAlg,
                           const LoadPostCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'LoadPostCommand'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.uri());
        hashAppend(hashAlg, object.file());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::LoadPostCommand)

namespace m_bmqtool {

// =========================
// class MessagePropertyType
// =========================

struct MessagePropertyType {
    // Enumeration of supported MessageProperty types.

  public:
    // TYPES
    enum Value { E_STRING = 0, E_INT = 1 };

    enum { NUM_ENUMERATORS = 2 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_EnumeratorInfo ENUMERATOR_INFO_ARRAY[];

    // CLASS METHODS
    static const char* toString(Value value);
    // Return the string representation exactly matching the enumerator
    // name corresponding to the specified enumeration 'value'.

    static int fromString(Value* result, const char* string, int stringLength);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string' of the specified 'stringLength'.  Return 0 on
    // success, and a non-zero value with no effect on 'result' otherwise
    // (i.e., 'string' does not match any enumerator).

    static int fromString(Value* result, const bsl::string& string);
    // Load into the specified 'result' the enumerator matching the
    // specified 'string'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'string' does not match any
    // enumerator).

    static int fromInt(Value* result, int number);
    // Load into the specified 'result' the enumerator matching the
    // specified 'number'.  Return 0 on success, and a non-zero value with
    // no effect on 'result' otherwise (i.e., 'number' does not match any
    // enumerator).

    static bsl::ostream& print(bsl::ostream& stream, Value value);
    // Write to the specified 'stream' the string representation of
    // the specified enumeration 'value'.  Return a reference to
    // the modifiable 'stream'.

    // HIDDEN FRIENDS
    friend bsl::ostream& operator<<(bsl::ostream& stream, Value rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return MessagePropertyType::print(stream, rhs);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_ENUMERATION_TRAITS(m_bmqtool::MessagePropertyType)

namespace m_bmqtool {

// =====================
// class MetadataCommand
// =====================

class MetadataCommand {
    // INSTANCE DATA

  public:
    // TYPES
    enum { NUM_ATTRIBUTES = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS

    // MANIPULATORS
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    // HIDDEN FRIENDS
    friend bool operator==(const MetadataCommand&, const MetadataCommand&)
    // Returns 'true' as this type has no attributes and so all objects of
    // this type are considered equal.
    {
        return true;
    }

    friend bool operator!=(const MetadataCommand& lhs,
                           const MetadataCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&          stream,
                                    const MetadataCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&, const MetadataCommand&)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'MetadataCommand'.
    {
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(m_bmqtool::MetadataCommand)

namespace m_bmqtool {

// ========================
// class OpenStorageCommand
// ========================

class OpenStorageCommand {
    // INSTANCE DATA
    bsl::string d_path;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_PATH = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_PATH = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit OpenStorageCommand(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'OpenStorageCommand' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    OpenStorageCommand(const OpenStorageCommand& original,
                       bslma::Allocator*         basicAllocator = 0);
    // Create an object of type 'OpenStorageCommand' having the value of
    // the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    OpenStorageCommand(OpenStorageCommand&& original) noexcept;
    // Create an object of type 'OpenStorageCommand' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    OpenStorageCommand(OpenStorageCommand&& original,
                       bslma::Allocator*    basicAllocator);
    // Create an object of type 'OpenStorageCommand' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~OpenStorageCommand();
    // Destroy this object.

    // MANIPULATORS
    OpenStorageCommand& operator=(const OpenStorageCommand& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    OpenStorageCommand& operator=(OpenStorageCommand&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& path();
    // Return a reference to the modifiable "Path" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
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

    // HIDDEN FRIENDS
    friend bool operator==(const OpenStorageCommand& lhs,
                           const OpenStorageCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.path() == rhs.path();
    }

    friend bool operator!=(const OpenStorageCommand& lhs,
                           const OpenStorageCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&             stream,
                                    const OpenStorageCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&         hashAlg,
                           const OpenStorageCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'OpenStorageCommand'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.path());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::OpenStorageCommand)

namespace m_bmqtool {

// ========================
// class QlistCommandChoice
// ========================

class QlistCommandChoice {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<bsls::Types::Uint64> d_n;
        bsls::ObjectBuffer<bsls::Types::Uint64> d_next;
        bsls::ObjectBuffer<bsls::Types::Uint64> d_p;
        bsls::ObjectBuffer<bsls::Types::Uint64> d_prev;
        bsls::ObjectBuffer<bsls::Types::Uint64> d_r;
        bsls::ObjectBuffer<bsls::Types::Uint64> d_record;
        bsls::ObjectBuffer<int>                 d_list;
        bsls::ObjectBuffer<int>                 d_l;
    };

    int d_selectionId;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const QlistCommandChoice& rhs) const;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED = -1,
        SELECTION_ID_N         = 0,
        SELECTION_ID_NEXT      = 1,
        SELECTION_ID_P         = 2,
        SELECTION_ID_PREV      = 3,
        SELECTION_ID_R         = 4,
        SELECTION_ID_RECORD    = 5,
        SELECTION_ID_LIST      = 6,
        SELECTION_ID_L         = 7
    };

    enum { NUM_SELECTIONS = 8 };

    enum {
        SELECTION_INDEX_N      = 0,
        SELECTION_INDEX_NEXT   = 1,
        SELECTION_INDEX_P      = 2,
        SELECTION_INDEX_PREV   = 3,
        SELECTION_INDEX_R      = 4,
        SELECTION_INDEX_RECORD = 5,
        SELECTION_INDEX_LIST   = 6,
        SELECTION_INDEX_L      = 7
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_SelectionInfo SELECTION_INFO_ARRAY[];

    // CLASS METHODS
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);
    // Return selection information for the selection indicated by the
    // specified 'id' if the selection exists, and 0 otherwise.

    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);
    // Return selection information for the selection indicated by the
    // specified 'name' of the specified 'nameLength' if the selection
    // exists, and 0 otherwise.

    // CREATORS
    QlistCommandChoice();
    // Create an object of type 'QlistCommandChoice' having the default
    // value.

    QlistCommandChoice(const QlistCommandChoice& original);
    // Create an object of type 'QlistCommandChoice' having the value of
    // the specified 'original' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QlistCommandChoice(QlistCommandChoice&& original) noexcept;
    // Create an object of type 'QlistCommandChoice' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
#endif

    ~QlistCommandChoice();
    // Destroy this object.

    // MANIPULATORS
    QlistCommandChoice& operator=(const QlistCommandChoice& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QlistCommandChoice& operator=(QlistCommandChoice&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon default
    // construction).

    int makeSelection(int selectionId);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'selectionId'.  Return 0 on success, and
    // non-zero value otherwise (i.e., the selection is not found).

    int makeSelection(const char* name, int nameLength);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'name' of the specified 'nameLength'.
    // Return 0 on success, and non-zero value otherwise (i.e., the
    // selection is not found).

    bsls::Types::Uint64& makeN();
    bsls::Types::Uint64& makeN(bsls::Types::Uint64 value);
    // Set the value of this object to be a "N" value.  Optionally specify
    // the 'value' of the "N".  If 'value' is not specified, the default
    // "N" value is used.

    bsls::Types::Uint64& makeNext();
    bsls::Types::Uint64& makeNext(bsls::Types::Uint64 value);
    // Set the value of this object to be a "Next" value.  Optionally
    // specify the 'value' of the "Next".  If 'value' is not specified, the
    // default "Next" value is used.

    bsls::Types::Uint64& makeP();
    bsls::Types::Uint64& makeP(bsls::Types::Uint64 value);
    // Set the value of this object to be a "P" value.  Optionally specify
    // the 'value' of the "P".  If 'value' is not specified, the default
    // "P" value is used.

    bsls::Types::Uint64& makePrev();
    bsls::Types::Uint64& makePrev(bsls::Types::Uint64 value);
    // Set the value of this object to be a "Prev" value.  Optionally
    // specify the 'value' of the "Prev".  If 'value' is not specified, the
    // default "Prev" value is used.

    bsls::Types::Uint64& makeR();
    bsls::Types::Uint64& makeR(bsls::Types::Uint64 value);
    // Set the value of this object to be a "R" value.  Optionally specify
    // the 'value' of the "R".  If 'value' is not specified, the default
    // "R" value is used.

    bsls::Types::Uint64& makeRecord();
    bsls::Types::Uint64& makeRecord(bsls::Types::Uint64 value);
    // Set the value of this object to be a "Record" value.  Optionally
    // specify the 'value' of the "Record".  If 'value' is not specified,
    // the default "Record" value is used.

    int& makeList();
    int& makeList(int value);
    // Set the value of this object to be a "List" value.  Optionally
    // specify the 'value' of the "List".  If 'value' is not specified, the
    // default "List" value is used.

    int& makeL();
    int& makeL(int value);
    // Set the value of this object to be a "L" value.  Optionally specify
    // the 'value' of the "L".  If 'value' is not specified, the default
    // "L" value is used.

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    bsls::Types::Uint64& n();
    // Return a reference to the modifiable "N" selection of this object if
    // "N" is the current selection.  The behavior is undefined unless "N"
    // is the selection of this object.

    bsls::Types::Uint64& next();
    // Return a reference to the modifiable "Next" selection of this object
    // if "Next" is the current selection.  The behavior is undefined
    // unless "Next" is the selection of this object.

    bsls::Types::Uint64& p();
    // Return a reference to the modifiable "P" selection of this object if
    // "P" is the current selection.  The behavior is undefined unless "P"
    // is the selection of this object.

    bsls::Types::Uint64& prev();
    // Return a reference to the modifiable "Prev" selection of this object
    // if "Prev" is the current selection.  The behavior is undefined
    // unless "Prev" is the selection of this object.

    bsls::Types::Uint64& r();
    // Return a reference to the modifiable "R" selection of this object if
    // "R" is the current selection.  The behavior is undefined unless "R"
    // is the selection of this object.

    bsls::Types::Uint64& record();
    // Return a reference to the modifiable "Record" selection of this
    // object if "Record" is the current selection.  The behavior is
    // undefined unless "Record" is the selection of this object.

    int& list();
    // Return a reference to the modifiable "List" selection of this object
    // if "List" is the current selection.  The behavior is undefined
    // unless "List" is the selection of this object.

    int& l();
    // Return a reference to the modifiable "L" selection of this object if
    // "L" is the current selection.  The behavior is undefined unless "L"
    // is the selection of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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

    int selectionId() const;
    // Return the id of the current selection if the selection is defined,
    // and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' on the non-modifiable selection,
    // supplying 'accessor' with the corresponding selection information
    // structure.  Return the value returned from the invocation of
    // 'accessor' if this object has a defined selection, and -1 otherwise.

    const bsls::Types::Uint64& n() const;
    // Return a reference to the non-modifiable "N" selection of this
    // object if "N" is the current selection.  The behavior is undefined
    // unless "N" is the selection of this object.

    const bsls::Types::Uint64& next() const;
    // Return a reference to the non-modifiable "Next" selection of this
    // object if "Next" is the current selection.  The behavior is
    // undefined unless "Next" is the selection of this object.

    const bsls::Types::Uint64& p() const;
    // Return a reference to the non-modifiable "P" selection of this
    // object if "P" is the current selection.  The behavior is undefined
    // unless "P" is the selection of this object.

    const bsls::Types::Uint64& prev() const;
    // Return a reference to the non-modifiable "Prev" selection of this
    // object if "Prev" is the current selection.  The behavior is
    // undefined unless "Prev" is the selection of this object.

    const bsls::Types::Uint64& r() const;
    // Return a reference to the non-modifiable "R" selection of this
    // object if "R" is the current selection.  The behavior is undefined
    // unless "R" is the selection of this object.

    const bsls::Types::Uint64& record() const;
    // Return a reference to the non-modifiable "Record" selection of this
    // object if "Record" is the current selection.  The behavior is
    // undefined unless "Record" is the selection of this object.

    const int& list() const;
    // Return a reference to the non-modifiable "List" selection of this
    // object if "List" is the current selection.  The behavior is
    // undefined unless "List" is the selection of this object.

    const int& l() const;
    // Return a reference to the non-modifiable "L" selection of this
    // object if "L" is the current selection.  The behavior is undefined
    // unless "L" is the selection of this object.

    bool isNValue() const;
    // Return 'true' if the value of this object is a "N" value, and return
    // 'false' otherwise.

    bool isNextValue() const;
    // Return 'true' if the value of this object is a "Next" value, and
    // return 'false' otherwise.

    bool isPValue() const;
    // Return 'true' if the value of this object is a "P" value, and return
    // 'false' otherwise.

    bool isPrevValue() const;
    // Return 'true' if the value of this object is a "Prev" value, and
    // return 'false' otherwise.

    bool isRValue() const;
    // Return 'true' if the value of this object is a "R" value, and return
    // 'false' otherwise.

    bool isRecordValue() const;
    // Return 'true' if the value of this object is a "Record" value, and
    // return 'false' otherwise.

    bool isListValue() const;
    // Return 'true' if the value of this object is a "List" value, and
    // return 'false' otherwise.

    bool isLValue() const;
    // Return 'true' if the value of this object is a "L" value, and return
    // 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const QlistCommandChoice& lhs,
                           const QlistCommandChoice& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'QlistCommandChoice' objects have
    // the same value if either the selections in both objects have the
    // same ids and the same values, or both selections are undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const QlistCommandChoice& lhs,
                           const QlistCommandChoice& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&             stream,
                                    const QlistCommandChoice& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&         hashAlg,
                           const QlistCommandChoice& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'QlistCommandChoice'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_BITWISEMOVEABLE_TRAITS(m_bmqtool::QlistCommandChoice)

namespace m_bmqtool {

// ==================
// class StartCommand
// ==================

class StartCommand {
    // INSTANCE DATA
    bool d_async;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_ASYNC = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_ASYNC = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_ASYNC;

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    StartCommand();
    // Create an object of type 'StartCommand' having the default value.

    // MANIPULATORS
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bool& async();
    // Return a reference to the modifiable "Async" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    bool async() const;
    // Return the value of the "Async" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StartCommand& lhs, const StartCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.async() == rhs.async();
    }

    friend bool operator!=(const StartCommand& lhs, const StartCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&       stream,
                                    const StartCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&   hashAlg,
                           const StartCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'StartCommand'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.async());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(m_bmqtool::StartCommand)

namespace m_bmqtool {

// =================
// class StopCommand
// =================

class StopCommand {
    // INSTANCE DATA
    bool d_async;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_ASYNC = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_ASYNC = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_ASYNC;

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    StopCommand();
    // Create an object of type 'StopCommand' having the default value.

    // MANIPULATORS
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bool& async();
    // Return a reference to the modifiable "Async" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    bool async() const;
    // Return the value of the "Async" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const StopCommand& lhs, const StopCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.async() == rhs.async();
    }

    friend bool operator!=(const StopCommand& lhs, const StopCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&      stream,
                                    const StopCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&  hashAlg,
                           const StopCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'StopCommand'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.async());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(m_bmqtool::StopCommand)

namespace m_bmqtool {

// ==================
// class Subscription
// ==================

class Subscription {
    // INSTANCE DATA
    bdlb::NullableValue<bsl::string>  d_expression;
    bdlb::NullableValue<unsigned int> d_correlationId;
    bdlb::NullableValue<int>          d_maxUnconfirmedMessages;
    bdlb::NullableValue<int>          d_maxUnconfirmedBytes;
    bdlb::NullableValue<int>          d_consumerPriority;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const Subscription& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_CORRELATION_ID           = 0,
        ATTRIBUTE_ID_EXPRESSION               = 1,
        ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES = 2,
        ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES    = 3,
        ATTRIBUTE_ID_CONSUMER_PRIORITY        = 4
    };

    enum { NUM_ATTRIBUTES = 5 };

    enum {
        ATTRIBUTE_INDEX_CORRELATION_ID           = 0,
        ATTRIBUTE_INDEX_EXPRESSION               = 1,
        ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES = 2,
        ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES    = 3,
        ATTRIBUTE_INDEX_CONSUMER_PRIORITY        = 4
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit Subscription(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Subscription' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    Subscription(const Subscription& original,
                 bslma::Allocator*   basicAllocator = 0);
    // Create an object of type 'Subscription' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Subscription(Subscription&& original) noexcept;
    // Create an object of type 'Subscription' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    Subscription(Subscription&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'Subscription' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~Subscription();
    // Destroy this object.

    // MANIPULATORS
    Subscription& operator=(const Subscription& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Subscription& operator=(Subscription&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bdlb::NullableValue<unsigned int>& correlationId();
    // Return a reference to the modifiable "CorrelationId" attribute of
    // this object.

    bdlb::NullableValue<bsl::string>& expression();
    // Return a reference to the modifiable "Expression" attribute of this
    // object.

    bdlb::NullableValue<int>& maxUnconfirmedMessages();
    // Return a reference to the modifiable "MaxUnconfirmedMessages"
    // attribute of this object.

    bdlb::NullableValue<int>& maxUnconfirmedBytes();
    // Return a reference to the modifiable "MaxUnconfirmedBytes" attribute
    // of this object.

    bdlb::NullableValue<int>& consumerPriority();
    // Return a reference to the modifiable "ConsumerPriority" attribute of
    // this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bdlb::NullableValue<unsigned int>& correlationId() const;
    // Return a reference offering non-modifiable access to the
    // "CorrelationId" attribute of this object.

    const bdlb::NullableValue<bsl::string>& expression() const;
    // Return a reference offering non-modifiable access to the
    // "Expression" attribute of this object.

    const bdlb::NullableValue<int>& maxUnconfirmedMessages() const;
    // Return a reference offering non-modifiable access to the
    // "MaxUnconfirmedMessages" attribute of this object.

    const bdlb::NullableValue<int>& maxUnconfirmedBytes() const;
    // Return a reference offering non-modifiable access to the
    // "MaxUnconfirmedBytes" attribute of this object.

    const bdlb::NullableValue<int>& consumerPriority() const;
    // Return a reference offering non-modifiable access to the
    // "ConsumerPriority" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Subscription& lhs, const Subscription& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const Subscription& lhs, const Subscription& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&       stream,
                                    const Subscription& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&   hashAlg,
                           const Subscription& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'Subscription'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::Subscription)

namespace m_bmqtool {

// ===========================
// class ConfigureQueueCommand
// ===========================

class ConfigureQueueCommand {
    // INSTANCE DATA
    bsl::vector<Subscription> d_subscriptions;
    bsl::string               d_uri;
    int                       d_maxUnconfirmedMessages;
    int                       d_maxUnconfirmedBytes;
    int                       d_consumerPriority;
    bool                      d_async;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const ConfigureQueueCommand& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_URI                      = 0,
        ATTRIBUTE_ID_ASYNC                    = 1,
        ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES = 2,
        ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES    = 3,
        ATTRIBUTE_ID_CONSUMER_PRIORITY        = 4,
        ATTRIBUTE_ID_SUBSCRIPTIONS            = 5
    };

    enum { NUM_ATTRIBUTES = 6 };

    enum {
        ATTRIBUTE_INDEX_URI                      = 0,
        ATTRIBUTE_INDEX_ASYNC                    = 1,
        ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES = 2,
        ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES    = 3,
        ATTRIBUTE_INDEX_CONSUMER_PRIORITY        = 4,
        ATTRIBUTE_INDEX_SUBSCRIPTIONS            = 5
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_ASYNC;

    static const int DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES;

    static const int DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES;

    static const int DEFAULT_INITIALIZER_CONSUMER_PRIORITY;

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit ConfigureQueueCommand(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'ConfigureQueueCommand' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    ConfigureQueueCommand(const ConfigureQueueCommand& original,
                          bslma::Allocator*            basicAllocator = 0);
    // Create an object of type 'ConfigureQueueCommand' having the value of
    // the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ConfigureQueueCommand(ConfigureQueueCommand&& original) noexcept;
    // Create an object of type 'ConfigureQueueCommand' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    ConfigureQueueCommand(ConfigureQueueCommand&& original,
                          bslma::Allocator*       basicAllocator);
    // Create an object of type 'ConfigureQueueCommand' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~ConfigureQueueCommand();
    // Destroy this object.

    // MANIPULATORS
    ConfigureQueueCommand& operator=(const ConfigureQueueCommand& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ConfigureQueueCommand& operator=(ConfigureQueueCommand&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& uri();
    // Return a reference to the modifiable "Uri" attribute of this object.

    bool& async();
    // Return a reference to the modifiable "Async" attribute of this
    // object.

    int& maxUnconfirmedMessages();
    // Return a reference to the modifiable "MaxUnconfirmedMessages"
    // attribute of this object.

    int& maxUnconfirmedBytes();
    // Return a reference to the modifiable "MaxUnconfirmedBytes" attribute
    // of this object.

    int& consumerPriority();
    // Return a reference to the modifiable "ConsumerPriority" attribute of
    // this object.

    bsl::vector<Subscription>& subscriptions();
    // Return a reference to the modifiable "Subscriptions" attribute of
    // this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& uri() const;
    // Return a reference offering non-modifiable access to the "Uri"
    // attribute of this object.

    bool async() const;
    // Return the value of the "Async" attribute of this object.

    int maxUnconfirmedMessages() const;
    // Return the value of the "MaxUnconfirmedMessages" attribute of this
    // object.

    int maxUnconfirmedBytes() const;
    // Return the value of the "MaxUnconfirmedBytes" attribute of this
    // object.

    int consumerPriority() const;
    // Return the value of the "ConsumerPriority" attribute of this object.

    const bsl::vector<Subscription>& subscriptions() const;
    // Return a reference offering non-modifiable access to the
    // "Subscriptions" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const ConfigureQueueCommand& lhs,
                           const ConfigureQueueCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const ConfigureQueueCommand& lhs,
                           const ConfigureQueueCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                stream,
                                    const ConfigureQueueCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&            hashAlg,
                           const ConfigureQueueCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'ConfigureQueueCommand'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::ConfigureQueueCommand)

namespace m_bmqtool {

// =================
// class DataCommand
// =================

class DataCommand {
    // INSTANCE DATA
    DataCommandChoice d_choice;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_CHOICE = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_CHOICE = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    DataCommand();
    // Create an object of type 'DataCommand' having the default value.

    // MANIPULATORS
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    DataCommandChoice& choice();
    // Return a reference to the modifiable "Choice" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const DataCommandChoice& choice() const;
    // Return a reference offering non-modifiable access to the "Choice"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const DataCommand& lhs, const DataCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.choice() == rhs.choice();
    }

    friend bool operator!=(const DataCommand& lhs, const DataCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&      stream,
                                    const DataCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&  hashAlg,
                           const DataCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'DataCommand'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.choice());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(m_bmqtool::DataCommand)

namespace m_bmqtool {

// ==========================
// class JournalCommandChoice
// ==========================

class JournalCommandChoice {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<bsls::Types::Uint64>             d_n;
        bsls::ObjectBuffer<bsls::Types::Uint64>             d_next;
        bsls::ObjectBuffer<bsls::Types::Uint64>             d_p;
        bsls::ObjectBuffer<bsls::Types::Uint64>             d_prev;
        bsls::ObjectBuffer<bsls::Types::Uint64>             d_r;
        bsls::ObjectBuffer<bsls::Types::Uint64>             d_record;
        bsls::ObjectBuffer<int>                             d_list;
        bsls::ObjectBuffer<int>                             d_l;
        bsls::ObjectBuffer<bsl::string>                     d_dump;
        bsls::ObjectBuffer<JournalCommandChoiceType::Value> d_type;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const JournalCommandChoice& rhs) const;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED = -1,
        SELECTION_ID_N         = 0,
        SELECTION_ID_NEXT      = 1,
        SELECTION_ID_P         = 2,
        SELECTION_ID_PREV      = 3,
        SELECTION_ID_R         = 4,
        SELECTION_ID_RECORD    = 5,
        SELECTION_ID_LIST      = 6,
        SELECTION_ID_L         = 7,
        SELECTION_ID_DUMP      = 8,
        SELECTION_ID_TYPE      = 9
    };

    enum { NUM_SELECTIONS = 10 };

    enum {
        SELECTION_INDEX_N      = 0,
        SELECTION_INDEX_NEXT   = 1,
        SELECTION_INDEX_P      = 2,
        SELECTION_INDEX_PREV   = 3,
        SELECTION_INDEX_R      = 4,
        SELECTION_INDEX_RECORD = 5,
        SELECTION_INDEX_LIST   = 6,
        SELECTION_INDEX_L      = 7,
        SELECTION_INDEX_DUMP   = 8,
        SELECTION_INDEX_TYPE   = 9
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_SelectionInfo SELECTION_INFO_ARRAY[];

    // CLASS METHODS
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);
    // Return selection information for the selection indicated by the
    // specified 'id' if the selection exists, and 0 otherwise.

    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);
    // Return selection information for the selection indicated by the
    // specified 'name' of the specified 'nameLength' if the selection
    // exists, and 0 otherwise.

    // CREATORS
    explicit JournalCommandChoice(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'JournalCommandChoice' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    JournalCommandChoice(const JournalCommandChoice& original,
                         bslma::Allocator*           basicAllocator = 0);
    // Create an object of type 'JournalCommandChoice' having the value of
    // the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    JournalCommandChoice(JournalCommandChoice&& original) noexcept;
    // Create an object of type 'JournalCommandChoice' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    JournalCommandChoice(JournalCommandChoice&& original,
                         bslma::Allocator*      basicAllocator);
    // Create an object of type 'JournalCommandChoice' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~JournalCommandChoice();
    // Destroy this object.

    // MANIPULATORS
    JournalCommandChoice& operator=(const JournalCommandChoice& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    JournalCommandChoice& operator=(JournalCommandChoice&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon default
    // construction).

    int makeSelection(int selectionId);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'selectionId'.  Return 0 on success, and
    // non-zero value otherwise (i.e., the selection is not found).

    int makeSelection(const char* name, int nameLength);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'name' of the specified 'nameLength'.
    // Return 0 on success, and non-zero value otherwise (i.e., the
    // selection is not found).

    bsls::Types::Uint64& makeN();
    bsls::Types::Uint64& makeN(bsls::Types::Uint64 value);
    // Set the value of this object to be a "N" value.  Optionally specify
    // the 'value' of the "N".  If 'value' is not specified, the default
    // "N" value is used.

    bsls::Types::Uint64& makeNext();
    bsls::Types::Uint64& makeNext(bsls::Types::Uint64 value);
    // Set the value of this object to be a "Next" value.  Optionally
    // specify the 'value' of the "Next".  If 'value' is not specified, the
    // default "Next" value is used.

    bsls::Types::Uint64& makeP();
    bsls::Types::Uint64& makeP(bsls::Types::Uint64 value);
    // Set the value of this object to be a "P" value.  Optionally specify
    // the 'value' of the "P".  If 'value' is not specified, the default
    // "P" value is used.

    bsls::Types::Uint64& makePrev();
    bsls::Types::Uint64& makePrev(bsls::Types::Uint64 value);
    // Set the value of this object to be a "Prev" value.  Optionally
    // specify the 'value' of the "Prev".  If 'value' is not specified, the
    // default "Prev" value is used.

    bsls::Types::Uint64& makeR();
    bsls::Types::Uint64& makeR(bsls::Types::Uint64 value);
    // Set the value of this object to be a "R" value.  Optionally specify
    // the 'value' of the "R".  If 'value' is not specified, the default
    // "R" value is used.

    bsls::Types::Uint64& makeRecord();
    bsls::Types::Uint64& makeRecord(bsls::Types::Uint64 value);
    // Set the value of this object to be a "Record" value.  Optionally
    // specify the 'value' of the "Record".  If 'value' is not specified,
    // the default "Record" value is used.

    int& makeList();
    int& makeList(int value);
    // Set the value of this object to be a "List" value.  Optionally
    // specify the 'value' of the "List".  If 'value' is not specified, the
    // default "List" value is used.

    int& makeL();
    int& makeL(int value);
    // Set the value of this object to be a "L" value.  Optionally specify
    // the 'value' of the "L".  If 'value' is not specified, the default
    // "L" value is used.

    bsl::string& makeDump();
    bsl::string& makeDump(const bsl::string& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    bsl::string& makeDump(bsl::string&& value);
#endif
    // Set the value of this object to be a "Dump" value.  Optionally
    // specify the 'value' of the "Dump".  If 'value' is not specified, the
    // default "Dump" value is used.

    JournalCommandChoiceType::Value& makeType();
    JournalCommandChoiceType::Value&
    makeType(JournalCommandChoiceType::Value value);
    // Set the value of this object to be a "Type" value.  Optionally
    // specify the 'value' of the "Type".  If 'value' is not specified, the
    // default "Type" value is used.

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    bsls::Types::Uint64& n();
    // Return a reference to the modifiable "N" selection of this object if
    // "N" is the current selection.  The behavior is undefined unless "N"
    // is the selection of this object.

    bsls::Types::Uint64& next();
    // Return a reference to the modifiable "Next" selection of this object
    // if "Next" is the current selection.  The behavior is undefined
    // unless "Next" is the selection of this object.

    bsls::Types::Uint64& p();
    // Return a reference to the modifiable "P" selection of this object if
    // "P" is the current selection.  The behavior is undefined unless "P"
    // is the selection of this object.

    bsls::Types::Uint64& prev();
    // Return a reference to the modifiable "Prev" selection of this object
    // if "Prev" is the current selection.  The behavior is undefined
    // unless "Prev" is the selection of this object.

    bsls::Types::Uint64& r();
    // Return a reference to the modifiable "R" selection of this object if
    // "R" is the current selection.  The behavior is undefined unless "R"
    // is the selection of this object.

    bsls::Types::Uint64& record();
    // Return a reference to the modifiable "Record" selection of this
    // object if "Record" is the current selection.  The behavior is
    // undefined unless "Record" is the selection of this object.

    int& list();
    // Return a reference to the modifiable "List" selection of this object
    // if "List" is the current selection.  The behavior is undefined
    // unless "List" is the selection of this object.

    int& l();
    // Return a reference to the modifiable "L" selection of this object if
    // "L" is the current selection.  The behavior is undefined unless "L"
    // is the selection of this object.

    bsl::string& dump();
    // Return a reference to the modifiable "Dump" selection of this object
    // if "Dump" is the current selection.  The behavior is undefined
    // unless "Dump" is the selection of this object.

    JournalCommandChoiceType::Value& type();
    // Return a reference to the modifiable "Type" selection of this object
    // if "Type" is the current selection.  The behavior is undefined
    // unless "Type" is the selection of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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

    int selectionId() const;
    // Return the id of the current selection if the selection is defined,
    // and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' on the non-modifiable selection,
    // supplying 'accessor' with the corresponding selection information
    // structure.  Return the value returned from the invocation of
    // 'accessor' if this object has a defined selection, and -1 otherwise.

    const bsls::Types::Uint64& n() const;
    // Return a reference to the non-modifiable "N" selection of this
    // object if "N" is the current selection.  The behavior is undefined
    // unless "N" is the selection of this object.

    const bsls::Types::Uint64& next() const;
    // Return a reference to the non-modifiable "Next" selection of this
    // object if "Next" is the current selection.  The behavior is
    // undefined unless "Next" is the selection of this object.

    const bsls::Types::Uint64& p() const;
    // Return a reference to the non-modifiable "P" selection of this
    // object if "P" is the current selection.  The behavior is undefined
    // unless "P" is the selection of this object.

    const bsls::Types::Uint64& prev() const;
    // Return a reference to the non-modifiable "Prev" selection of this
    // object if "Prev" is the current selection.  The behavior is
    // undefined unless "Prev" is the selection of this object.

    const bsls::Types::Uint64& r() const;
    // Return a reference to the non-modifiable "R" selection of this
    // object if "R" is the current selection.  The behavior is undefined
    // unless "R" is the selection of this object.

    const bsls::Types::Uint64& record() const;
    // Return a reference to the non-modifiable "Record" selection of this
    // object if "Record" is the current selection.  The behavior is
    // undefined unless "Record" is the selection of this object.

    const int& list() const;
    // Return a reference to the non-modifiable "List" selection of this
    // object if "List" is the current selection.  The behavior is
    // undefined unless "List" is the selection of this object.

    const int& l() const;
    // Return a reference to the non-modifiable "L" selection of this
    // object if "L" is the current selection.  The behavior is undefined
    // unless "L" is the selection of this object.

    const bsl::string& dump() const;
    // Return a reference to the non-modifiable "Dump" selection of this
    // object if "Dump" is the current selection.  The behavior is
    // undefined unless "Dump" is the selection of this object.

    const JournalCommandChoiceType::Value& type() const;
    // Return a reference to the non-modifiable "Type" selection of this
    // object if "Type" is the current selection.  The behavior is
    // undefined unless "Type" is the selection of this object.

    bool isNValue() const;
    // Return 'true' if the value of this object is a "N" value, and return
    // 'false' otherwise.

    bool isNextValue() const;
    // Return 'true' if the value of this object is a "Next" value, and
    // return 'false' otherwise.

    bool isPValue() const;
    // Return 'true' if the value of this object is a "P" value, and return
    // 'false' otherwise.

    bool isPrevValue() const;
    // Return 'true' if the value of this object is a "Prev" value, and
    // return 'false' otherwise.

    bool isRValue() const;
    // Return 'true' if the value of this object is a "R" value, and return
    // 'false' otherwise.

    bool isRecordValue() const;
    // Return 'true' if the value of this object is a "Record" value, and
    // return 'false' otherwise.

    bool isListValue() const;
    // Return 'true' if the value of this object is a "List" value, and
    // return 'false' otherwise.

    bool isLValue() const;
    // Return 'true' if the value of this object is a "L" value, and return
    // 'false' otherwise.

    bool isDumpValue() const;
    // Return 'true' if the value of this object is a "Dump" value, and
    // return 'false' otherwise.

    bool isTypeValue() const;
    // Return 'true' if the value of this object is a "Type" value, and
    // return 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const JournalCommandChoice& lhs,
                           const JournalCommandChoice& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'JournalCommandChoice' objects
    // have the same value if either the selections in both objects have
    // the same ids and the same values, or both selections are undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const JournalCommandChoice& lhs,
                           const JournalCommandChoice& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&               stream,
                                    const JournalCommandChoice& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&           hashAlg,
                           const JournalCommandChoice& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'JournalCommandChoice'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::JournalCommandChoice)

namespace m_bmqtool {

// =====================
// class MessageProperty
// =====================

class MessageProperty {
    // INSTANCE DATA
    bsl::string                d_name;
    bsl::string                d_value;
    MessagePropertyType::Value d_type;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_NAME  = 0,
        ATTRIBUTE_ID_VALUE = 1,
        ATTRIBUTE_ID_TYPE  = 2
    };

    enum { NUM_ATTRIBUTES = 3 };

    enum {
        ATTRIBUTE_INDEX_NAME  = 0,
        ATTRIBUTE_INDEX_VALUE = 1,
        ATTRIBUTE_INDEX_TYPE  = 2
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const MessagePropertyType::Value DEFAULT_INITIALIZER_TYPE;

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit MessageProperty(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'MessageProperty' having the default value.
    //  Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    MessageProperty(const MessageProperty& original,
                    bslma::Allocator*      basicAllocator = 0);
    // Create an object of type 'MessageProperty' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    MessageProperty(MessageProperty&& original) noexcept;
    // Create an object of type 'MessageProperty' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    MessageProperty(MessageProperty&& original,
                    bslma::Allocator* basicAllocator);
    // Create an object of type 'MessageProperty' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~MessageProperty();
    // Destroy this object.

    // MANIPULATORS
    MessageProperty& operator=(const MessageProperty& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    MessageProperty& operator=(MessageProperty&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& name();
    // Return a reference to the modifiable "Name" attribute of this
    // object.

    bsl::string& value();
    // Return a reference to the modifiable "Value" attribute of this
    // object.

    MessagePropertyType::Value& type();
    // Return a reference to the modifiable "Type" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& name() const;
    // Return a reference offering non-modifiable access to the "Name"
    // attribute of this object.

    const bsl::string& value() const;
    // Return a reference offering non-modifiable access to the "Value"
    // attribute of this object.

    MessagePropertyType::Value type() const;
    // Return the value of the "Type" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const MessageProperty& lhs,
                           const MessageProperty& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.name() == rhs.name() && lhs.value() == rhs.value() &&
               lhs.type() == rhs.type();
    }

    friend bool operator!=(const MessageProperty& lhs,
                           const MessageProperty& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&          stream,
                                    const MessageProperty& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&      hashAlg,
                           const MessageProperty& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'MessageProperty'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::MessageProperty)

namespace m_bmqtool {

// ======================
// class OpenQueueCommand
// ======================

class OpenQueueCommand {
    // INSTANCE DATA
    bsl::vector<Subscription> d_subscriptions;
    bsl::string               d_uri;
    bsl::string               d_flags;
    int                       d_maxUnconfirmedMessages;
    int                       d_maxUnconfirmedBytes;
    int                       d_consumerPriority;
    bool                      d_async;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const OpenQueueCommand& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_URI                      = 0,
        ATTRIBUTE_ID_FLAGS                    = 1,
        ATTRIBUTE_ID_ASYNC                    = 2,
        ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES = 3,
        ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES    = 4,
        ATTRIBUTE_ID_CONSUMER_PRIORITY        = 5,
        ATTRIBUTE_ID_SUBSCRIPTIONS            = 6
    };

    enum { NUM_ATTRIBUTES = 7 };

    enum {
        ATTRIBUTE_INDEX_URI                      = 0,
        ATTRIBUTE_INDEX_FLAGS                    = 1,
        ATTRIBUTE_INDEX_ASYNC                    = 2,
        ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES = 3,
        ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES    = 4,
        ATTRIBUTE_INDEX_CONSUMER_PRIORITY        = 5,
        ATTRIBUTE_INDEX_SUBSCRIPTIONS            = 6
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_ASYNC;

    static const int DEFAULT_INITIALIZER_MAX_UNCONFIRMED_MESSAGES;

    static const int DEFAULT_INITIALIZER_MAX_UNCONFIRMED_BYTES;

    static const int DEFAULT_INITIALIZER_CONSUMER_PRIORITY;

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit OpenQueueCommand(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'OpenQueueCommand' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    OpenQueueCommand(const OpenQueueCommand& original,
                     bslma::Allocator*       basicAllocator = 0);
    // Create an object of type 'OpenQueueCommand' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    OpenQueueCommand(OpenQueueCommand&& original) noexcept;
    // Create an object of type 'OpenQueueCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    OpenQueueCommand(OpenQueueCommand&& original,
                     bslma::Allocator*  basicAllocator);
    // Create an object of type 'OpenQueueCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~OpenQueueCommand();
    // Destroy this object.

    // MANIPULATORS
    OpenQueueCommand& operator=(const OpenQueueCommand& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    OpenQueueCommand& operator=(OpenQueueCommand&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& uri();
    // Return a reference to the modifiable "Uri" attribute of this object.

    bsl::string& flags();
    // Return a reference to the modifiable "Flags" attribute of this
    // object.

    bool& async();
    // Return a reference to the modifiable "Async" attribute of this
    // object.

    int& maxUnconfirmedMessages();
    // Return a reference to the modifiable "MaxUnconfirmedMessages"
    // attribute of this object.

    int& maxUnconfirmedBytes();
    // Return a reference to the modifiable "MaxUnconfirmedBytes" attribute
    // of this object.

    int& consumerPriority();
    // Return a reference to the modifiable "ConsumerPriority" attribute of
    // this object.

    bsl::vector<Subscription>& subscriptions();
    // Return a reference to the modifiable "Subscriptions" attribute of
    // this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& uri() const;
    // Return a reference offering non-modifiable access to the "Uri"
    // attribute of this object.

    const bsl::string& flags() const;
    // Return a reference offering non-modifiable access to the "Flags"
    // attribute of this object.

    bool async() const;
    // Return the value of the "Async" attribute of this object.

    int maxUnconfirmedMessages() const;
    // Return the value of the "MaxUnconfirmedMessages" attribute of this
    // object.

    int maxUnconfirmedBytes() const;
    // Return the value of the "MaxUnconfirmedBytes" attribute of this
    // object.

    int consumerPriority() const;
    // Return the value of the "ConsumerPriority" attribute of this object.

    const bsl::vector<Subscription>& subscriptions() const;
    // Return a reference offering non-modifiable access to the
    // "Subscriptions" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const OpenQueueCommand& lhs,
                           const OpenQueueCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const OpenQueueCommand& lhs,
                           const OpenQueueCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&           stream,
                                    const OpenQueueCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&       hashAlg,
                           const OpenQueueCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'OpenQueueCommand'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::OpenQueueCommand)

namespace m_bmqtool {

// ==================
// class QlistCommand
// ==================

class QlistCommand {
    // INSTANCE DATA
    QlistCommandChoice d_choice;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_CHOICE = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_CHOICE = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    QlistCommand();
    // Create an object of type 'QlistCommand' having the default value.

    // MANIPULATORS
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    QlistCommandChoice& choice();
    // Return a reference to the modifiable "Choice" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const QlistCommandChoice& choice() const;
    // Return a reference offering non-modifiable access to the "Choice"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const QlistCommand& lhs, const QlistCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.choice() == rhs.choice();
    }

    friend bool operator!=(const QlistCommand& lhs, const QlistCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&       stream,
                                    const QlistCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&   hashAlg,
                           const QlistCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'QlistCommand'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.choice());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_BITWISEMOVEABLE_TRAITS(m_bmqtool::QlistCommand)

namespace m_bmqtool {

// ===========================
// class CommandLineParameters
// ===========================

class CommandLineParameters {
    // INSTANCE DATA
    bsls::Types::Int64           d_eventSize;
    bsl::vector<Subscription>    d_subscriptions;
    bsl::vector<MessageProperty> d_messageProperties;
    bsl::string                  d_mode;
    bsl::string                  d_broker;
    bsl::string                  d_queueUri;
    bsl::string                  d_queueFlags;
    bsl::string                  d_latency;
    bsl::string                  d_latencyReport;
    bsl::string                  d_eventsCount;
    bsl::string                  d_maxUnconfirmed;
    bsl::string                  d_verbosity;
    bsl::string                  d_logFormat;
    bsl::string                  d_storage;
    bsl::string                  d_log;
    bsl::string                  d_sequentialMessagePattern;
    int                          d_msgSize;
    int                          d_postRate;
    int                          d_postInterval;
    int                          d_threads;
    int                          d_shutdownGrace;
    bool                         d_dumpMsg;
    bool                         d_confirmMsg;
    bool                         d_memoryDebug;
    bool                         d_noSessionEventHandler;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const CommandLineParameters& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_MODE                       = 0,
        ATTRIBUTE_ID_BROKER                     = 1,
        ATTRIBUTE_ID_QUEUE_URI                  = 2,
        ATTRIBUTE_ID_QUEUE_FLAGS                = 3,
        ATTRIBUTE_ID_LATENCY                    = 4,
        ATTRIBUTE_ID_LATENCY_REPORT             = 5,
        ATTRIBUTE_ID_DUMP_MSG                   = 6,
        ATTRIBUTE_ID_CONFIRM_MSG                = 7,
        ATTRIBUTE_ID_EVENT_SIZE                 = 8,
        ATTRIBUTE_ID_MSG_SIZE                   = 9,
        ATTRIBUTE_ID_POST_RATE                  = 10,
        ATTRIBUTE_ID_EVENTS_COUNT               = 11,
        ATTRIBUTE_ID_MAX_UNCONFIRMED            = 12,
        ATTRIBUTE_ID_POST_INTERVAL              = 13,
        ATTRIBUTE_ID_VERBOSITY                  = 14,
        ATTRIBUTE_ID_LOG_FORMAT                 = 15,
        ATTRIBUTE_ID_MEMORY_DEBUG               = 16,
        ATTRIBUTE_ID_THREADS                    = 17,
        ATTRIBUTE_ID_SHUTDOWN_GRACE             = 18,
        ATTRIBUTE_ID_NO_SESSION_EVENT_HANDLER   = 19,
        ATTRIBUTE_ID_STORAGE                    = 20,
        ATTRIBUTE_ID_LOG                        = 21,
        ATTRIBUTE_ID_SEQUENTIAL_MESSAGE_PATTERN = 22,
        ATTRIBUTE_ID_MESSAGE_PROPERTIES         = 23,
        ATTRIBUTE_ID_SUBSCRIPTIONS              = 24
    };

    enum { NUM_ATTRIBUTES = 25 };

    enum {
        ATTRIBUTE_INDEX_MODE                       = 0,
        ATTRIBUTE_INDEX_BROKER                     = 1,
        ATTRIBUTE_INDEX_QUEUE_URI                  = 2,
        ATTRIBUTE_INDEX_QUEUE_FLAGS                = 3,
        ATTRIBUTE_INDEX_LATENCY                    = 4,
        ATTRIBUTE_INDEX_LATENCY_REPORT             = 5,
        ATTRIBUTE_INDEX_DUMP_MSG                   = 6,
        ATTRIBUTE_INDEX_CONFIRM_MSG                = 7,
        ATTRIBUTE_INDEX_EVENT_SIZE                 = 8,
        ATTRIBUTE_INDEX_MSG_SIZE                   = 9,
        ATTRIBUTE_INDEX_POST_RATE                  = 10,
        ATTRIBUTE_INDEX_EVENTS_COUNT               = 11,
        ATTRIBUTE_INDEX_MAX_UNCONFIRMED            = 12,
        ATTRIBUTE_INDEX_POST_INTERVAL              = 13,
        ATTRIBUTE_INDEX_VERBOSITY                  = 14,
        ATTRIBUTE_INDEX_LOG_FORMAT                 = 15,
        ATTRIBUTE_INDEX_MEMORY_DEBUG               = 16,
        ATTRIBUTE_INDEX_THREADS                    = 17,
        ATTRIBUTE_INDEX_SHUTDOWN_GRACE             = 18,
        ATTRIBUTE_INDEX_NO_SESSION_EVENT_HANDLER   = 19,
        ATTRIBUTE_INDEX_STORAGE                    = 20,
        ATTRIBUTE_INDEX_LOG                        = 21,
        ATTRIBUTE_INDEX_SEQUENTIAL_MESSAGE_PATTERN = 22,
        ATTRIBUTE_INDEX_MESSAGE_PROPERTIES         = 23,
        ATTRIBUTE_INDEX_SUBSCRIPTIONS              = 24
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const char DEFAULT_INITIALIZER_MODE[];

    static const char DEFAULT_INITIALIZER_BROKER[];

    static const char DEFAULT_INITIALIZER_QUEUE_URI[];

    static const char DEFAULT_INITIALIZER_QUEUE_FLAGS[];

    static const char DEFAULT_INITIALIZER_LATENCY[];

    static const char DEFAULT_INITIALIZER_LATENCY_REPORT[];

    static const bool DEFAULT_INITIALIZER_DUMP_MSG;

    static const bool DEFAULT_INITIALIZER_CONFIRM_MSG;

    static const bsls::Types::Int64 DEFAULT_INITIALIZER_EVENT_SIZE;

    static const int DEFAULT_INITIALIZER_MSG_SIZE;

    static const int DEFAULT_INITIALIZER_POST_RATE;

    static const char DEFAULT_INITIALIZER_EVENTS_COUNT[];

    static const char DEFAULT_INITIALIZER_MAX_UNCONFIRMED[];

    static const int DEFAULT_INITIALIZER_POST_INTERVAL;

    static const char DEFAULT_INITIALIZER_VERBOSITY[];

    static const char DEFAULT_INITIALIZER_LOG_FORMAT[];

    static const bool DEFAULT_INITIALIZER_MEMORY_DEBUG;

    static const int DEFAULT_INITIALIZER_THREADS;

    static const int DEFAULT_INITIALIZER_SHUTDOWN_GRACE;

    static const bool DEFAULT_INITIALIZER_NO_SESSION_EVENT_HANDLER;

    static const char DEFAULT_INITIALIZER_STORAGE[];

    static const char DEFAULT_INITIALIZER_LOG[];

    static const char DEFAULT_INITIALIZER_SEQUENTIAL_MESSAGE_PATTERN[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit CommandLineParameters(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'CommandLineParameters' having the default
    // value.  Use the optionally specified 'basicAllocator' to supply
    // memory.  If 'basicAllocator' is 0, the currently installed default
    // allocator is used.

    CommandLineParameters(const CommandLineParameters& original,
                          bslma::Allocator*            basicAllocator = 0);
    // Create an object of type 'CommandLineParameters' having the value of
    // the specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    CommandLineParameters(CommandLineParameters&& original) noexcept;
    // Create an object of type 'CommandLineParameters' having the value of
    // the specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    CommandLineParameters(CommandLineParameters&& original,
                          bslma::Allocator*       basicAllocator);
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

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& mode();
    // Return a reference to the modifiable "Mode" attribute of this
    // object.

    bsl::string& broker();
    // Return a reference to the modifiable "Broker" attribute of this
    // object.

    bsl::string& queueUri();
    // Return a reference to the modifiable "QueueUri" attribute of this
    // object.

    bsl::string& queueFlags();
    // Return a reference to the modifiable "QueueFlags" attribute of this
    // object.

    bsl::string& latency();
    // Return a reference to the modifiable "Latency" attribute of this
    // object.

    bsl::string& latencyReport();
    // Return a reference to the modifiable "LatencyReport" attribute of
    // this object.

    bool& dumpMsg();
    // Return a reference to the modifiable "DumpMsg" attribute of this
    // object.

    bool& confirmMsg();
    // Return a reference to the modifiable "ConfirmMsg" attribute of this
    // object.

    bsls::Types::Int64& eventSize();
    // Return a reference to the modifiable "EventSize" attribute of this
    // object.

    int& msgSize();
    // Return a reference to the modifiable "MsgSize" attribute of this
    // object.

    int& postRate();
    // Return a reference to the modifiable "PostRate" attribute of this
    // object.

    bsl::string& eventsCount();
    // Return a reference to the modifiable "EventsCount" attribute of this
    // object.

    bsl::string& maxUnconfirmed();
    // Return a reference to the modifiable "MaxUnconfirmed" attribute of
    // this object.

    int& postInterval();
    // Return a reference to the modifiable "PostInterval" attribute of
    // this object.

    bsl::string& verbosity();
    // Return a reference to the modifiable "Verbosity" attribute of this
    // object.

    bsl::string& logFormat();
    // Return a reference to the modifiable "LogFormat" attribute of this
    // object.

    bool& memoryDebug();
    // Return a reference to the modifiable "MemoryDebug" attribute of this
    // object.

    int& threads();
    // Return a reference to the modifiable "Threads" attribute of this
    // object.

    int& shutdownGrace();
    // Return a reference to the modifiable "ShutdownGrace" attribute of
    // this object.

    bool& noSessionEventHandler();
    // Return a reference to the modifiable "NoSessionEventHandler"
    // attribute of this object.

    bsl::string& storage();
    // Return a reference to the modifiable "Storage" attribute of this
    // object.

    bsl::string& log();
    // Return a reference to the modifiable "Log" attribute of this object.

    bsl::string& sequentialMessagePattern();
    // Return a reference to the modifiable "SequentialMessagePattern"
    // attribute of this object.

    bsl::vector<MessageProperty>& messageProperties();
    // Return a reference to the modifiable "MessageProperties" attribute
    // of this object.

    bsl::vector<Subscription>& subscriptions();
    // Return a reference to the modifiable "Subscriptions" attribute of
    // this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& mode() const;
    // Return a reference offering non-modifiable access to the "Mode"
    // attribute of this object.

    const bsl::string& broker() const;
    // Return a reference offering non-modifiable access to the "Broker"
    // attribute of this object.

    const bsl::string& queueUri() const;
    // Return a reference offering non-modifiable access to the "QueueUri"
    // attribute of this object.

    const bsl::string& queueFlags() const;
    // Return a reference offering non-modifiable access to the
    // "QueueFlags" attribute of this object.

    const bsl::string& latency() const;
    // Return a reference offering non-modifiable access to the "Latency"
    // attribute of this object.

    const bsl::string& latencyReport() const;
    // Return a reference offering non-modifiable access to the
    // "LatencyReport" attribute of this object.

    bool dumpMsg() const;
    // Return the value of the "DumpMsg" attribute of this object.

    bool confirmMsg() const;
    // Return the value of the "ConfirmMsg" attribute of this object.

    bsls::Types::Int64 eventSize() const;
    // Return the value of the "EventSize" attribute of this object.

    int msgSize() const;
    // Return the value of the "MsgSize" attribute of this object.

    int postRate() const;
    // Return the value of the "PostRate" attribute of this object.

    const bsl::string& eventsCount() const;
    // Return a reference offering non-modifiable access to the
    // "EventsCount" attribute of this object.

    const bsl::string& maxUnconfirmed() const;
    // Return a reference offering non-modifiable access to the
    // "MaxUnconfirmed" attribute of this object.

    int postInterval() const;
    // Return the value of the "PostInterval" attribute of this object.

    const bsl::string& verbosity() const;
    // Return a reference offering non-modifiable access to the "Verbosity"
    // attribute of this object.

    const bsl::string& logFormat() const;
    // Return a reference offering non-modifiable access to the "LogFormat"
    // attribute of this object.

    bool memoryDebug() const;
    // Return the value of the "MemoryDebug" attribute of this object.

    int threads() const;
    // Return the value of the "Threads" attribute of this object.

    int shutdownGrace() const;
    // Return the value of the "ShutdownGrace" attribute of this object.

    bool noSessionEventHandler() const;
    // Return the value of the "NoSessionEventHandler" attribute of this
    // object.

    const bsl::string& storage() const;
    // Return a reference offering non-modifiable access to the "Storage"
    // attribute of this object.

    const bsl::string& log() const;
    // Return a reference offering non-modifiable access to the "Log"
    // attribute of this object.

    const bsl::string& sequentialMessagePattern() const;
    // Return a reference offering non-modifiable access to the
    // "SequentialMessagePattern" attribute of this object.

    const bsl::vector<MessageProperty>& messageProperties() const;
    // Return a reference offering non-modifiable access to the
    // "MessageProperties" attribute of this object.

    const bsl::vector<Subscription>& subscriptions() const;
    // Return a reference offering non-modifiable access to the
    // "Subscriptions" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const CommandLineParameters& lhs,
                           const CommandLineParameters& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const CommandLineParameters& lhs,
                           const CommandLineParameters& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&                stream,
                                    const CommandLineParameters& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&            hashAlg,
                           const CommandLineParameters& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'CommandLineParameters'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::CommandLineParameters)

namespace m_bmqtool {

// ====================
// class JournalCommand
// ====================

class JournalCommand {
    // INSTANCE DATA
    JournalCommandChoice d_choice;

  public:
    // TYPES
    enum { ATTRIBUTE_ID_CHOICE = 0 };

    enum { NUM_ATTRIBUTES = 1 };

    enum { ATTRIBUTE_INDEX_CHOICE = 0 };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit JournalCommand(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'JournalCommand' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    JournalCommand(const JournalCommand& original,
                   bslma::Allocator*     basicAllocator = 0);
    // Create an object of type 'JournalCommand' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    JournalCommand(JournalCommand&& original) noexcept;
    // Create an object of type 'JournalCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    JournalCommand(JournalCommand&&  original,
                   bslma::Allocator* basicAllocator);
    // Create an object of type 'JournalCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~JournalCommand();
    // Destroy this object.

    // MANIPULATORS
    JournalCommand& operator=(const JournalCommand& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    JournalCommand& operator=(JournalCommand&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    JournalCommandChoice& choice();
    // Return a reference to the modifiable "Choice" attribute of this
    // object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const JournalCommandChoice& choice() const;
    // Return a reference offering non-modifiable access to the "Choice"
    // attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const JournalCommand& lhs,
                           const JournalCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.choice() == rhs.choice();
    }

    friend bool operator!=(const JournalCommand& lhs,
                           const JournalCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&         stream,
                                    const JournalCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&     hashAlg,
                           const JournalCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for
    // 'JournalCommand'.
    {
        using bslh::hashAppend;
        hashAppend(hashAlg, object.choice());
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::JournalCommand)

namespace m_bmqtool {

// =================
// class PostCommand
// =================

class PostCommand {
    // INSTANCE DATA
    bsl::vector<bsl::string>     d_payload;
    bsl::vector<MessageProperty> d_messageProperties;
    bsl::string                  d_uri;
    bsl::string                  d_groupid;
    bsl::string                  d_compressionAlgorithmType;
    bool                         d_async;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const PostCommand& rhs) const;

  public:
    // TYPES
    enum {
        ATTRIBUTE_ID_URI                        = 0,
        ATTRIBUTE_ID_PAYLOAD                    = 1,
        ATTRIBUTE_ID_ASYNC                      = 2,
        ATTRIBUTE_ID_GROUPID                    = 3,
        ATTRIBUTE_ID_COMPRESSION_ALGORITHM_TYPE = 4,
        ATTRIBUTE_ID_MESSAGE_PROPERTIES         = 5
    };

    enum { NUM_ATTRIBUTES = 6 };

    enum {
        ATTRIBUTE_INDEX_URI                        = 0,
        ATTRIBUTE_INDEX_PAYLOAD                    = 1,
        ATTRIBUTE_INDEX_ASYNC                      = 2,
        ATTRIBUTE_INDEX_GROUPID                    = 3,
        ATTRIBUTE_INDEX_COMPRESSION_ALGORITHM_TYPE = 4,
        ATTRIBUTE_INDEX_MESSAGE_PROPERTIES         = 5
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bool DEFAULT_INITIALIZER_ASYNC;

    static const char DEFAULT_INITIALIZER_GROUPID[];

    static const char DEFAULT_INITIALIZER_COMPRESSION_ALGORITHM_TYPE[];

    static const bdlat_AttributeInfo ATTRIBUTE_INFO_ARRAY[];

  public:
    // CLASS METHODS
    static const bdlat_AttributeInfo* lookupAttributeInfo(int id);
    // Return attribute information for the attribute indicated by the
    // specified 'id' if the attribute exists, and 0 otherwise.

    static const bdlat_AttributeInfo* lookupAttributeInfo(const char* name,
                                                          int nameLength);
    // Return attribute information for the attribute indicated by the
    // specified 'name' of the specified 'nameLength' if the attribute
    // exists, and 0 otherwise.

    // CREATORS
    explicit PostCommand(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'PostCommand' having the default value.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    PostCommand(const PostCommand& original,
                bslma::Allocator*  basicAllocator = 0);
    // Create an object of type 'PostCommand' having the value of the
    // specified 'original' object.  Use the optionally specified
    // 'basicAllocator' to supply memory.  If 'basicAllocator' is 0, the
    // currently installed default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    PostCommand(PostCommand&& original) noexcept;
    // Create an object of type 'PostCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.

    PostCommand(PostCommand&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'PostCommand' having the value of the
    // specified 'original' object.  After performing this action, the
    // 'original' object will be left in a valid, but unspecified state.
    // Use the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~PostCommand();
    // Destroy this object.

    // MANIPULATORS
    PostCommand& operator=(const PostCommand& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    PostCommand& operator=(PostCommand&& rhs);
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
    int manipulateAttribute(t_MANIPULATOR& manipulator,
                            const char*    name,
                            int            nameLength);
    // Invoke the specified 'manipulator' on the address of
    // the (modifiable) attribute indicated by the specified 'name' of the
    // specified 'nameLength', supplying 'manipulator' with the
    // corresponding attribute information structure.  Return the value
    // returned from the invocation of 'manipulator' if 'name' identifies
    // an attribute of this class, and -1 otherwise.

    bsl::string& uri();
    // Return a reference to the modifiable "Uri" attribute of this object.

    bsl::vector<bsl::string>& payload();
    // Return a reference to the modifiable "Payload" attribute of this
    // object.

    bool& async();
    // Return a reference to the modifiable "Async" attribute of this
    // object.

    bsl::string& groupid();
    // Return a reference to the modifiable "Groupid" attribute of this
    // object.

    bsl::string& compressionAlgorithmType();
    // Return a reference to the modifiable "CompressionAlgorithmType"
    // attribute of this object.

    bsl::vector<MessageProperty>& messageProperties();
    // Return a reference to the modifiable "MessageProperties" attribute
    // of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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
    int accessAttribute(t_ACCESSOR& accessor,
                        const char* name,
                        int         nameLength) const;
    // Invoke the specified 'accessor' on the (non-modifiable) attribute
    // of this object indicated by the specified 'name' of the specified
    // 'nameLength', supplying 'accessor' with the corresponding attribute
    // information structure.  Return the value returned from the
    // invocation of 'accessor' if 'name' identifies an attribute of this
    // class, and -1 otherwise.

    const bsl::string& uri() const;
    // Return a reference offering non-modifiable access to the "Uri"
    // attribute of this object.

    const bsl::vector<bsl::string>& payload() const;
    // Return a reference offering non-modifiable access to the "Payload"
    // attribute of this object.

    bool async() const;
    // Return the value of the "Async" attribute of this object.

    const bsl::string& groupid() const;
    // Return a reference offering non-modifiable access to the "Groupid"
    // attribute of this object.

    const bsl::string& compressionAlgorithmType() const;
    // Return a reference offering non-modifiable access to the
    // "CompressionAlgorithmType" attribute of this object.

    const bsl::vector<MessageProperty>& messageProperties() const;
    // Return a reference offering non-modifiable access to the
    // "MessageProperties" attribute of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const PostCommand& lhs, const PostCommand& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' attribute objects
    // have the same value, and 'false' otherwise.  Two attribute objects
    // have the same value if each respective attribute has the same value.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const PostCommand& lhs, const PostCommand& rhs)
    // Returns '!(lhs == rhs)'
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream&      stream,
                                    const PostCommand& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM&  hashAlg,
                           const PostCommand& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'PostCommand'.
    {
        object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_SEQUENCE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(
    m_bmqtool::PostCommand)

namespace m_bmqtool {

// =============
// class Command
// =============

class Command {
    // INSTANCE DATA
    union {
        bsls::ObjectBuffer<StartCommand>          d_start;
        bsls::ObjectBuffer<StopCommand>           d_stop;
        bsls::ObjectBuffer<OpenQueueCommand>      d_openQueue;
        bsls::ObjectBuffer<ConfigureQueueCommand> d_configureQueue;
        bsls::ObjectBuffer<CloseQueueCommand>     d_closeQueue;
        bsls::ObjectBuffer<PostCommand>           d_post;
        bsls::ObjectBuffer<ListCommand>           d_list;
        bsls::ObjectBuffer<ConfirmCommand>        d_confirm;
        bsls::ObjectBuffer<BatchPostCommand>      d_batchPost;
        bsls::ObjectBuffer<LoadPostCommand>       d_loadPost;
        bsls::ObjectBuffer<OpenStorageCommand>    d_openStorage;
        bsls::ObjectBuffer<CloseStorageCommand>   d_closeStorage;
        bsls::ObjectBuffer<MetadataCommand>       d_metadata;
        bsls::ObjectBuffer<ListQueuesCommand>     d_listQueues;
        bsls::ObjectBuffer<DumpQueueCommand>      d_dumpQueue;
        bsls::ObjectBuffer<DataCommand>           d_data;
        bsls::ObjectBuffer<QlistCommand>          d_qlist;
        bsls::ObjectBuffer<JournalCommand>        d_journal;
    };

    int               d_selectionId;
    bslma::Allocator* d_allocator_p;

    // PRIVATE ACCESSORS
    template <typename t_HASH_ALGORITHM>
    void hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const;

    bool isEqualTo(const Command& rhs) const;

  public:
    // TYPES

    enum {
        SELECTION_ID_UNDEFINED       = -1,
        SELECTION_ID_START           = 0,
        SELECTION_ID_STOP            = 1,
        SELECTION_ID_OPEN_QUEUE      = 2,
        SELECTION_ID_CONFIGURE_QUEUE = 3,
        SELECTION_ID_CLOSE_QUEUE     = 4,
        SELECTION_ID_POST            = 5,
        SELECTION_ID_LIST            = 6,
        SELECTION_ID_CONFIRM         = 7,
        SELECTION_ID_BATCH_POST      = 8,
        SELECTION_ID_LOAD_POST       = 9,
        SELECTION_ID_OPEN_STORAGE    = 10,
        SELECTION_ID_CLOSE_STORAGE   = 11,
        SELECTION_ID_METADATA        = 12,
        SELECTION_ID_LIST_QUEUES     = 13,
        SELECTION_ID_DUMP_QUEUE      = 14,
        SELECTION_ID_DATA            = 15,
        SELECTION_ID_QLIST           = 16,
        SELECTION_ID_JOURNAL         = 17
    };

    enum { NUM_SELECTIONS = 18 };

    enum {
        SELECTION_INDEX_START           = 0,
        SELECTION_INDEX_STOP            = 1,
        SELECTION_INDEX_OPEN_QUEUE      = 2,
        SELECTION_INDEX_CONFIGURE_QUEUE = 3,
        SELECTION_INDEX_CLOSE_QUEUE     = 4,
        SELECTION_INDEX_POST            = 5,
        SELECTION_INDEX_LIST            = 6,
        SELECTION_INDEX_CONFIRM         = 7,
        SELECTION_INDEX_BATCH_POST      = 8,
        SELECTION_INDEX_LOAD_POST       = 9,
        SELECTION_INDEX_OPEN_STORAGE    = 10,
        SELECTION_INDEX_CLOSE_STORAGE   = 11,
        SELECTION_INDEX_METADATA        = 12,
        SELECTION_INDEX_LIST_QUEUES     = 13,
        SELECTION_INDEX_DUMP_QUEUE      = 14,
        SELECTION_INDEX_DATA            = 15,
        SELECTION_INDEX_QLIST           = 16,
        SELECTION_INDEX_JOURNAL         = 17
    };

    // CONSTANTS
    static const char CLASS_NAME[];

    static const bdlat_SelectionInfo SELECTION_INFO_ARRAY[];

    // CLASS METHODS
    static const bdlat_SelectionInfo* lookupSelectionInfo(int id);
    // Return selection information for the selection indicated by the
    // specified 'id' if the selection exists, and 0 otherwise.

    static const bdlat_SelectionInfo* lookupSelectionInfo(const char* name,
                                                          int nameLength);
    // Return selection information for the selection indicated by the
    // specified 'name' of the specified 'nameLength' if the selection
    // exists, and 0 otherwise.

    // CREATORS
    explicit Command(bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Command' having the default value.  Use
    // the optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.

    Command(const Command& original, bslma::Allocator* basicAllocator = 0);
    // Create an object of type 'Command' having the value of the specified
    // 'original' object.  Use the optionally specified 'basicAllocator' to
    // supply memory.  If 'basicAllocator' is 0, the currently installed
    // default allocator is used.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Command(Command&& original) noexcept;
    // Create an object of type 'Command' having the value of the specified
    // 'original' object.  After performing this action, the 'original'
    // object will be left in a valid, but unspecified state.

    Command(Command&& original, bslma::Allocator* basicAllocator);
    // Create an object of type 'Command' having the value of the specified
    // 'original' object.  After performing this action, the 'original'
    // object will be left in a valid, but unspecified state.  Use the
    // optionally specified 'basicAllocator' to supply memory.  If
    // 'basicAllocator' is 0, the currently installed default allocator is
    // used.
#endif

    ~Command();
    // Destroy this object.

    // MANIPULATORS
    Command& operator=(const Command& rhs);
    // Assign to this object the value of the specified 'rhs' object.

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    Command& operator=(Command&& rhs);
    // Assign to this object the value of the specified 'rhs' object.
    // After performing this action, the 'rhs' object will be left in a
    // valid, but unspecified state.
#endif

    void reset();
    // Reset this object to the default value (i.e., its value upon default
    // construction).

    int makeSelection(int selectionId);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'selectionId'.  Return 0 on success, and
    // non-zero value otherwise (i.e., the selection is not found).

    int makeSelection(const char* name, int nameLength);
    // Set the value of this object to be the default for the selection
    // indicated by the specified 'name' of the specified 'nameLength'.
    // Return 0 on success, and non-zero value otherwise (i.e., the
    // selection is not found).

    StartCommand& makeStart();
    StartCommand& makeStart(const StartCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StartCommand& makeStart(StartCommand&& value);
#endif
    // Set the value of this object to be a "Start" value.  Optionally
    // specify the 'value' of the "Start".  If 'value' is not specified,
    // the default "Start" value is used.

    StopCommand& makeStop();
    StopCommand& makeStop(const StopCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    StopCommand& makeStop(StopCommand&& value);
#endif
    // Set the value of this object to be a "Stop" value.  Optionally
    // specify the 'value' of the "Stop".  If 'value' is not specified, the
    // default "Stop" value is used.

    OpenQueueCommand& makeOpenQueue();
    OpenQueueCommand& makeOpenQueue(const OpenQueueCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    OpenQueueCommand& makeOpenQueue(OpenQueueCommand&& value);
#endif
    // Set the value of this object to be a "OpenQueue" value.  Optionally
    // specify the 'value' of the "OpenQueue".  If 'value' is not
    // specified, the default "OpenQueue" value is used.

    ConfigureQueueCommand& makeConfigureQueue();
    ConfigureQueueCommand&
    makeConfigureQueue(const ConfigureQueueCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ConfigureQueueCommand& makeConfigureQueue(ConfigureQueueCommand&& value);
#endif
    // Set the value of this object to be a "ConfigureQueue" value.
    // Optionally specify the 'value' of the "ConfigureQueue".  If 'value'
    // is not specified, the default "ConfigureQueue" value is used.

    CloseQueueCommand& makeCloseQueue();
    CloseQueueCommand& makeCloseQueue(const CloseQueueCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    CloseQueueCommand& makeCloseQueue(CloseQueueCommand&& value);
#endif
    // Set the value of this object to be a "CloseQueue" value.  Optionally
    // specify the 'value' of the "CloseQueue".  If 'value' is not
    // specified, the default "CloseQueue" value is used.

    PostCommand& makePost();
    PostCommand& makePost(const PostCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    PostCommand& makePost(PostCommand&& value);
#endif
    // Set the value of this object to be a "Post" value.  Optionally
    // specify the 'value' of the "Post".  If 'value' is not specified, the
    // default "Post" value is used.

    ListCommand& makeList();
    ListCommand& makeList(const ListCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ListCommand& makeList(ListCommand&& value);
#endif
    // Set the value of this object to be a "List" value.  Optionally
    // specify the 'value' of the "List".  If 'value' is not specified, the
    // default "List" value is used.

    ConfirmCommand& makeConfirm();
    ConfirmCommand& makeConfirm(const ConfirmCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ConfirmCommand& makeConfirm(ConfirmCommand&& value);
#endif
    // Set the value of this object to be a "Confirm" value.  Optionally
    // specify the 'value' of the "Confirm".  If 'value' is not specified,
    // the default "Confirm" value is used.

    BatchPostCommand& makeBatchPost();
    BatchPostCommand& makeBatchPost(const BatchPostCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    BatchPostCommand& makeBatchPost(BatchPostCommand&& value);
#endif
    // Set the value of this object to be a "BatchPost" value.  Optionally
    // specify the 'value' of the "BatchPost".  If 'value' is not
    // specified, the default "BatchPost" value is used.

    LoadPostCommand& makeLoadPost();
    LoadPostCommand& makeLoadPost(const LoadPostCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    LoadPostCommand& makeLoadPost(LoadPostCommand&& value);
#endif
    // Set the value of this object to be a "LoadPost" value.  Optionally
    // specify the 'value' of the "LoadPost".  If 'value' is not specified,
    // the default "LoadPost" value is used.

    OpenStorageCommand& makeOpenStorage();
    OpenStorageCommand& makeOpenStorage(const OpenStorageCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    OpenStorageCommand& makeOpenStorage(OpenStorageCommand&& value);
#endif
    // Set the value of this object to be a "OpenStorage" value.
    // Optionally specify the 'value' of the "OpenStorage".  If 'value' is
    // not specified, the default "OpenStorage" value is used.

    CloseStorageCommand& makeCloseStorage();
    CloseStorageCommand& makeCloseStorage(const CloseStorageCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    CloseStorageCommand& makeCloseStorage(CloseStorageCommand&& value);
#endif
    // Set the value of this object to be a "CloseStorage" value.
    // Optionally specify the 'value' of the "CloseStorage".  If 'value' is
    // not specified, the default "CloseStorage" value is used.

    MetadataCommand& makeMetadata();
    MetadataCommand& makeMetadata(const MetadataCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    MetadataCommand& makeMetadata(MetadataCommand&& value);
#endif
    // Set the value of this object to be a "Metadata" value.  Optionally
    // specify the 'value' of the "Metadata".  If 'value' is not specified,
    // the default "Metadata" value is used.

    ListQueuesCommand& makeListQueues();
    ListQueuesCommand& makeListQueues(const ListQueuesCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    ListQueuesCommand& makeListQueues(ListQueuesCommand&& value);
#endif
    // Set the value of this object to be a "ListQueues" value.  Optionally
    // specify the 'value' of the "ListQueues".  If 'value' is not
    // specified, the default "ListQueues" value is used.

    DumpQueueCommand& makeDumpQueue();
    DumpQueueCommand& makeDumpQueue(const DumpQueueCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DumpQueueCommand& makeDumpQueue(DumpQueueCommand&& value);
#endif
    // Set the value of this object to be a "DumpQueue" value.  Optionally
    // specify the 'value' of the "DumpQueue".  If 'value' is not
    // specified, the default "DumpQueue" value is used.

    DataCommand& makeData();
    DataCommand& makeData(const DataCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    DataCommand& makeData(DataCommand&& value);
#endif
    // Set the value of this object to be a "Data" value.  Optionally
    // specify the 'value' of the "Data".  If 'value' is not specified, the
    // default "Data" value is used.

    QlistCommand& makeQlist();
    QlistCommand& makeQlist(const QlistCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    QlistCommand& makeQlist(QlistCommand&& value);
#endif
    // Set the value of this object to be a "Qlist" value.  Optionally
    // specify the 'value' of the "Qlist".  If 'value' is not specified,
    // the default "Qlist" value is used.

    JournalCommand& makeJournal();
    JournalCommand& makeJournal(const JournalCommand& value);
#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES) &&               \
    defined(BSLS_COMPILERFEATURES_SUPPORT_NOEXCEPT)
    JournalCommand& makeJournal(JournalCommand&& value);
#endif
    // Set the value of this object to be a "Journal" value.  Optionally
    // specify the 'value' of the "Journal".  If 'value' is not specified,
    // the default "Journal" value is used.

    template <typename t_MANIPULATOR>
    int manipulateSelection(t_MANIPULATOR& manipulator);
    // Invoke the specified 'manipulator' on the address of the modifiable
    // selection, supplying 'manipulator' with the corresponding selection
    // information structure.  Return the value returned from the
    // invocation of 'manipulator' if this object has a defined selection,
    // and -1 otherwise.

    StartCommand& start();
    // Return a reference to the modifiable "Start" selection of this
    // object if "Start" is the current selection.  The behavior is
    // undefined unless "Start" is the selection of this object.

    StopCommand& stop();
    // Return a reference to the modifiable "Stop" selection of this object
    // if "Stop" is the current selection.  The behavior is undefined
    // unless "Stop" is the selection of this object.

    OpenQueueCommand& openQueue();
    // Return a reference to the modifiable "OpenQueue" selection of this
    // object if "OpenQueue" is the current selection.  The behavior is
    // undefined unless "OpenQueue" is the selection of this object.

    ConfigureQueueCommand& configureQueue();
    // Return a reference to the modifiable "ConfigureQueue" selection of
    // this object if "ConfigureQueue" is the current selection.  The
    // behavior is undefined unless "ConfigureQueue" is the selection of
    // this object.

    CloseQueueCommand& closeQueue();
    // Return a reference to the modifiable "CloseQueue" selection of this
    // object if "CloseQueue" is the current selection.  The behavior is
    // undefined unless "CloseQueue" is the selection of this object.

    PostCommand& post();
    // Return a reference to the modifiable "Post" selection of this object
    // if "Post" is the current selection.  The behavior is undefined
    // unless "Post" is the selection of this object.

    ListCommand& list();
    // Return a reference to the modifiable "List" selection of this object
    // if "List" is the current selection.  The behavior is undefined
    // unless "List" is the selection of this object.

    ConfirmCommand& confirm();
    // Return a reference to the modifiable "Confirm" selection of this
    // object if "Confirm" is the current selection.  The behavior is
    // undefined unless "Confirm" is the selection of this object.

    BatchPostCommand& batchPost();
    // Return a reference to the modifiable "BatchPost" selection of this
    // object if "BatchPost" is the current selection.  The behavior is
    // undefined unless "BatchPost" is the selection of this object.

    LoadPostCommand& loadPost();
    // Return a reference to the modifiable "LoadPost" selection of this
    // object if "LoadPost" is the current selection.  The behavior is
    // undefined unless "LoadPost" is the selection of this object.

    OpenStorageCommand& openStorage();
    // Return a reference to the modifiable "OpenStorage" selection of this
    // object if "OpenStorage" is the current selection.  The behavior is
    // undefined unless "OpenStorage" is the selection of this object.

    CloseStorageCommand& closeStorage();
    // Return a reference to the modifiable "CloseStorage" selection of
    // this object if "CloseStorage" is the current selection.  The
    // behavior is undefined unless "CloseStorage" is the selection of this
    // object.

    MetadataCommand& metadata();
    // Return a reference to the modifiable "Metadata" selection of this
    // object if "Metadata" is the current selection.  The behavior is
    // undefined unless "Metadata" is the selection of this object.

    ListQueuesCommand& listQueues();
    // Return a reference to the modifiable "ListQueues" selection of this
    // object if "ListQueues" is the current selection.  The behavior is
    // undefined unless "ListQueues" is the selection of this object.

    DumpQueueCommand& dumpQueue();
    // Return a reference to the modifiable "DumpQueue" selection of this
    // object if "DumpQueue" is the current selection.  The behavior is
    // undefined unless "DumpQueue" is the selection of this object.

    DataCommand& data();
    // Return a reference to the modifiable "Data" selection of this object
    // if "Data" is the current selection.  The behavior is undefined
    // unless "Data" is the selection of this object.

    QlistCommand& qlist();
    // Return a reference to the modifiable "Qlist" selection of this
    // object if "Qlist" is the current selection.  The behavior is
    // undefined unless "Qlist" is the selection of this object.

    JournalCommand& journal();
    // Return a reference to the modifiable "Journal" selection of this
    // object if "Journal" is the current selection.  The behavior is
    // undefined unless "Journal" is the selection of this object.

    // ACCESSORS
    bsl::ostream&
    print(bsl::ostream& stream, int level = 0, int spacesPerLevel = 4) const;
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

    int selectionId() const;
    // Return the id of the current selection if the selection is defined,
    // and -1 otherwise.

    template <typename t_ACCESSOR>
    int accessSelection(t_ACCESSOR& accessor) const;
    // Invoke the specified 'accessor' on the non-modifiable selection,
    // supplying 'accessor' with the corresponding selection information
    // structure.  Return the value returned from the invocation of
    // 'accessor' if this object has a defined selection, and -1 otherwise.

    const StartCommand& start() const;
    // Return a reference to the non-modifiable "Start" selection of this
    // object if "Start" is the current selection.  The behavior is
    // undefined unless "Start" is the selection of this object.

    const StopCommand& stop() const;
    // Return a reference to the non-modifiable "Stop" selection of this
    // object if "Stop" is the current selection.  The behavior is
    // undefined unless "Stop" is the selection of this object.

    const OpenQueueCommand& openQueue() const;
    // Return a reference to the non-modifiable "OpenQueue" selection of
    // this object if "OpenQueue" is the current selection.  The behavior
    // is undefined unless "OpenQueue" is the selection of this object.

    const ConfigureQueueCommand& configureQueue() const;
    // Return a reference to the non-modifiable "ConfigureQueue" selection
    // of this object if "ConfigureQueue" is the current selection.  The
    // behavior is undefined unless "ConfigureQueue" is the selection of
    // this object.

    const CloseQueueCommand& closeQueue() const;
    // Return a reference to the non-modifiable "CloseQueue" selection of
    // this object if "CloseQueue" is the current selection.  The behavior
    // is undefined unless "CloseQueue" is the selection of this object.

    const PostCommand& post() const;
    // Return a reference to the non-modifiable "Post" selection of this
    // object if "Post" is the current selection.  The behavior is
    // undefined unless "Post" is the selection of this object.

    const ListCommand& list() const;
    // Return a reference to the non-modifiable "List" selection of this
    // object if "List" is the current selection.  The behavior is
    // undefined unless "List" is the selection of this object.

    const ConfirmCommand& confirm() const;
    // Return a reference to the non-modifiable "Confirm" selection of this
    // object if "Confirm" is the current selection.  The behavior is
    // undefined unless "Confirm" is the selection of this object.

    const BatchPostCommand& batchPost() const;
    // Return a reference to the non-modifiable "BatchPost" selection of
    // this object if "BatchPost" is the current selection.  The behavior
    // is undefined unless "BatchPost" is the selection of this object.

    const LoadPostCommand& loadPost() const;
    // Return a reference to the non-modifiable "LoadPost" selection of
    // this object if "LoadPost" is the current selection.  The behavior is
    // undefined unless "LoadPost" is the selection of this object.

    const OpenStorageCommand& openStorage() const;
    // Return a reference to the non-modifiable "OpenStorage" selection of
    // this object if "OpenStorage" is the current selection.  The behavior
    // is undefined unless "OpenStorage" is the selection of this object.

    const CloseStorageCommand& closeStorage() const;
    // Return a reference to the non-modifiable "CloseStorage" selection of
    // this object if "CloseStorage" is the current selection.  The
    // behavior is undefined unless "CloseStorage" is the selection of this
    // object.

    const MetadataCommand& metadata() const;
    // Return a reference to the non-modifiable "Metadata" selection of
    // this object if "Metadata" is the current selection.  The behavior is
    // undefined unless "Metadata" is the selection of this object.

    const ListQueuesCommand& listQueues() const;
    // Return a reference to the non-modifiable "ListQueues" selection of
    // this object if "ListQueues" is the current selection.  The behavior
    // is undefined unless "ListQueues" is the selection of this object.

    const DumpQueueCommand& dumpQueue() const;
    // Return a reference to the non-modifiable "DumpQueue" selection of
    // this object if "DumpQueue" is the current selection.  The behavior
    // is undefined unless "DumpQueue" is the selection of this object.

    const DataCommand& data() const;
    // Return a reference to the non-modifiable "Data" selection of this
    // object if "Data" is the current selection.  The behavior is
    // undefined unless "Data" is the selection of this object.

    const QlistCommand& qlist() const;
    // Return a reference to the non-modifiable "Qlist" selection of this
    // object if "Qlist" is the current selection.  The behavior is
    // undefined unless "Qlist" is the selection of this object.

    const JournalCommand& journal() const;
    // Return a reference to the non-modifiable "Journal" selection of this
    // object if "Journal" is the current selection.  The behavior is
    // undefined unless "Journal" is the selection of this object.

    bool isStartValue() const;
    // Return 'true' if the value of this object is a "Start" value, and
    // return 'false' otherwise.

    bool isStopValue() const;
    // Return 'true' if the value of this object is a "Stop" value, and
    // return 'false' otherwise.

    bool isOpenQueueValue() const;
    // Return 'true' if the value of this object is a "OpenQueue" value,
    // and return 'false' otherwise.

    bool isConfigureQueueValue() const;
    // Return 'true' if the value of this object is a "ConfigureQueue"
    // value, and return 'false' otherwise.

    bool isCloseQueueValue() const;
    // Return 'true' if the value of this object is a "CloseQueue" value,
    // and return 'false' otherwise.

    bool isPostValue() const;
    // Return 'true' if the value of this object is a "Post" value, and
    // return 'false' otherwise.

    bool isListValue() const;
    // Return 'true' if the value of this object is a "List" value, and
    // return 'false' otherwise.

    bool isConfirmValue() const;
    // Return 'true' if the value of this object is a "Confirm" value, and
    // return 'false' otherwise.

    bool isBatchPostValue() const;
    // Return 'true' if the value of this object is a "BatchPost" value,
    // and return 'false' otherwise.

    bool isLoadPostValue() const;
    // Return 'true' if the value of this object is a "LoadPost" value, and
    // return 'false' otherwise.

    bool isOpenStorageValue() const;
    // Return 'true' if the value of this object is a "OpenStorage" value,
    // and return 'false' otherwise.

    bool isCloseStorageValue() const;
    // Return 'true' if the value of this object is a "CloseStorage" value,
    // and return 'false' otherwise.

    bool isMetadataValue() const;
    // Return 'true' if the value of this object is a "Metadata" value, and
    // return 'false' otherwise.

    bool isListQueuesValue() const;
    // Return 'true' if the value of this object is a "ListQueues" value,
    // and return 'false' otherwise.

    bool isDumpQueueValue() const;
    // Return 'true' if the value of this object is a "DumpQueue" value,
    // and return 'false' otherwise.

    bool isDataValue() const;
    // Return 'true' if the value of this object is a "Data" value, and
    // return 'false' otherwise.

    bool isQlistValue() const;
    // Return 'true' if the value of this object is a "Qlist" value, and
    // return 'false' otherwise.

    bool isJournalValue() const;
    // Return 'true' if the value of this object is a "Journal" value, and
    // return 'false' otherwise.

    bool isUndefinedValue() const;
    // Return 'true' if the value of this object is undefined, and 'false'
    // otherwise.

    const char* selectionName() const;
    // Return the symbolic name of the current selection of this object.

    // HIDDEN FRIENDS
    friend bool operator==(const Command& lhs, const Command& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects have the same
    // value, and 'false' otherwise.  Two 'Command' objects have the same
    // value if either the selections in both objects have the same ids and
    // the same values, or both selections are undefined.
    {
        return lhs.isEqualTo(rhs);
    }

    friend bool operator!=(const Command& lhs, const Command& rhs)
    // Return 'true' if the specified 'lhs' and 'rhs' objects do not have
    // the same values, as determined by 'operator==', and 'false'
    // otherwise.
    {
        return !(lhs == rhs);
    }

    friend bsl::ostream& operator<<(bsl::ostream& stream, const Command& rhs)
    // Format the specified 'rhs' to the specified output 'stream' and
    // return a reference to the modifiable 'stream'.
    {
        return rhs.print(stream, 0, -1);
    }

    template <typename t_HASH_ALGORITHM>
    friend void hashAppend(t_HASH_ALGORITHM& hashAlg, const Command& object)
    // Pass the specified 'object' to the specified 'hashAlg'.  This
    // function integrates with the 'bslh' modular hashing system and
    // effectively provides a 'bsl::hash' specialization for 'Command'.
    {
        return object.hashAppendImpl(hashAlg);
    }
};

}  // close package namespace

// TRAITS

BDLAT_DECL_CHOICE_WITH_ALLOCATOR_BITWISEMOVEABLE_TRAITS(m_bmqtool::Command)

//=============================================================================
//                          INLINE DEFINITIONS
//=============================================================================

namespace m_bmqtool {

// ----------------------
// class BatchPostCommand
// ----------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void BatchPostCommand::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->uri());
    hashAppend(hashAlgorithm, this->payload());
    hashAppend(hashAlgorithm, this->msgSize());
    hashAppend(hashAlgorithm, this->eventSize());
    hashAppend(hashAlgorithm, this->eventsCount());
    hashAppend(hashAlgorithm, this->postInterval());
    hashAppend(hashAlgorithm, this->postRate());
}

inline bool BatchPostCommand::isEqualTo(const BatchPostCommand& rhs) const
{
    return this->uri() == rhs.uri() && this->payload() == rhs.payload() &&
           this->msgSize() == rhs.msgSize() &&
           this->eventSize() == rhs.eventSize() &&
           this->eventsCount() == rhs.eventsCount() &&
           this->postInterval() == rhs.postInterval() &&
           this->postRate() == rhs.postRate();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int BatchPostCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_payload,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PAYLOAD]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_msgSize,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_eventSize,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENT_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_eventsCount,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENTS_COUNT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_postInterval,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_postRate,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_RATE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int BatchPostCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_PAYLOAD: {
        return manipulator(&d_payload,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PAYLOAD]);
    }
    case ATTRIBUTE_ID_MSG_SIZE: {
        return manipulator(&d_msgSize,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_SIZE]);
    }
    case ATTRIBUTE_ID_EVENT_SIZE: {
        return manipulator(&d_eventSize,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENT_SIZE]);
    }
    case ATTRIBUTE_ID_EVENTS_COUNT: {
        return manipulator(&d_eventsCount,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENTS_COUNT]);
    }
    case ATTRIBUTE_ID_POST_INTERVAL: {
        return manipulator(
            &d_postInterval,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_INTERVAL]);
    }
    case ATTRIBUTE_ID_POST_RATE: {
        return manipulator(&d_postRate,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_RATE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int BatchPostCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                          const char*    name,
                                          int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& BatchPostCommand::uri()
{
    return d_uri;
}

inline bsl::vector<bsl::string>& BatchPostCommand::payload()
{
    return d_payload;
}

inline int& BatchPostCommand::msgSize()
{
    return d_msgSize;
}

inline bsls::Types::Int64& BatchPostCommand::eventSize()
{
    return d_eventSize;
}

inline bsls::Types::Int64& BatchPostCommand::eventsCount()
{
    return d_eventsCount;
}

inline int& BatchPostCommand::postInterval()
{
    return d_postInterval;
}

inline int& BatchPostCommand::postRate()
{
    return d_postRate;
}

// ACCESSORS
template <typename t_ACCESSOR>
int BatchPostCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_payload, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PAYLOAD]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_msgSize, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_eventSize,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENT_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_eventsCount,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENTS_COUNT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_postInterval,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_postRate,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_RATE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int BatchPostCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_PAYLOAD: {
        return accessor(d_payload,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PAYLOAD]);
    }
    case ATTRIBUTE_ID_MSG_SIZE: {
        return accessor(d_msgSize,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_SIZE]);
    }
    case ATTRIBUTE_ID_EVENT_SIZE: {
        return accessor(d_eventSize,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENT_SIZE]);
    }
    case ATTRIBUTE_ID_EVENTS_COUNT: {
        return accessor(d_eventsCount,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENTS_COUNT]);
    }
    case ATTRIBUTE_ID_POST_INTERVAL: {
        return accessor(d_postInterval,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_INTERVAL]);
    }
    case ATTRIBUTE_ID_POST_RATE: {
        return accessor(d_postRate,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_RATE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int BatchPostCommand::accessAttribute(t_ACCESSOR& accessor,
                                      const char* name,
                                      int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& BatchPostCommand::uri() const
{
    return d_uri;
}

inline const bsl::vector<bsl::string>& BatchPostCommand::payload() const
{
    return d_payload;
}

inline int BatchPostCommand::msgSize() const
{
    return d_msgSize;
}

inline bsls::Types::Int64 BatchPostCommand::eventSize() const
{
    return d_eventSize;
}

inline bsls::Types::Int64 BatchPostCommand::eventsCount() const
{
    return d_eventsCount;
}

inline int BatchPostCommand::postInterval() const
{
    return d_postInterval;
}

inline int BatchPostCommand::postRate() const
{
    return d_postRate;
}

// -----------------------
// class CloseQueueCommand
// -----------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int CloseQueueCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int CloseQueueCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_ASYNC: {
        return manipulator(&d_async,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int CloseQueueCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                           const char*    name,
                                           int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& CloseQueueCommand::uri()
{
    return d_uri;
}

inline bool& CloseQueueCommand::async()
{
    return d_async;
}

// ACCESSORS
template <typename t_ACCESSOR>
int CloseQueueCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int CloseQueueCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_ASYNC: {
        return accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int CloseQueueCommand::accessAttribute(t_ACCESSOR& accessor,
                                       const char* name,
                                       int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& CloseQueueCommand::uri() const
{
    return d_uri;
}

inline bool CloseQueueCommand::async() const
{
    return d_async;
}

// -------------------------
// class CloseStorageCommand
// -------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int CloseStorageCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int CloseStorageCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                             int            id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int CloseStorageCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                             const char*    name,
                                             int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

// ACCESSORS
template <typename t_ACCESSOR>
int CloseStorageCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int CloseStorageCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int CloseStorageCommand::accessAttribute(t_ACCESSOR& accessor,
                                         const char* name,
                                         int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

// --------------------
// class ConfirmCommand
// --------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ConfirmCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_guid, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ConfirmCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_GUID: {
        return manipulator(&d_guid,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ConfirmCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                        const char*    name,
                                        int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& ConfirmCommand::uri()
{
    return d_uri;
}

inline bsl::string& ConfirmCommand::guid()
{
    return d_guid;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ConfirmCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_guid, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ConfirmCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_GUID: {
        return accessor(d_guid, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GUID]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ConfirmCommand::accessAttribute(t_ACCESSOR& accessor,
                                    const char* name,
                                    int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& ConfirmCommand::uri() const
{
    return d_uri;
}

inline const bsl::string& ConfirmCommand::guid() const
{
    return d_guid;
}

// -----------------------
// class DataCommandChoice
// -----------------------

// CLASS METHODS
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void DataCommandChoice::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef DataCommandChoice Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_N: hashAppend(hashAlgorithm, this->n()); break;
    case Class::SELECTION_ID_NEXT:
        hashAppend(hashAlgorithm, this->next());
        break;
    case Class::SELECTION_ID_P: hashAppend(hashAlgorithm, this->p()); break;
    case Class::SELECTION_ID_PREV:
        hashAppend(hashAlgorithm, this->prev());
        break;
    case Class::SELECTION_ID_R: hashAppend(hashAlgorithm, this->r()); break;
    case Class::SELECTION_ID_RECORD:
        hashAppend(hashAlgorithm, this->record());
        break;
    case Class::SELECTION_ID_LIST:
        hashAppend(hashAlgorithm, this->list());
        break;
    case Class::SELECTION_ID_L: hashAppend(hashAlgorithm, this->l()); break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool DataCommandChoice::isEqualTo(const DataCommandChoice& rhs) const
{
    typedef DataCommandChoice Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_N: return this->n() == rhs.n();
        case Class::SELECTION_ID_NEXT: return this->next() == rhs.next();
        case Class::SELECTION_ID_P: return this->p() == rhs.p();
        case Class::SELECTION_ID_PREV: return this->prev() == rhs.prev();
        case Class::SELECTION_ID_R: return this->r() == rhs.r();
        case Class::SELECTION_ID_RECORD: return this->record() == rhs.record();
        case Class::SELECTION_ID_LIST: return this->list() == rhs.list();
        case Class::SELECTION_ID_L: return this->l() == rhs.l();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

// CREATORS
inline DataCommandChoice::DataCommandChoice()
: d_selectionId(SELECTION_ID_UNDEFINED)
{
}

inline DataCommandChoice::~DataCommandChoice()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int DataCommandChoice::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case DataCommandChoice::SELECTION_ID_N:
        return manipulator(&d_n.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_N]);
    case DataCommandChoice::SELECTION_ID_NEXT:
        return manipulator(&d_next.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT]);
    case DataCommandChoice::SELECTION_ID_P:
        return manipulator(&d_p.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_P]);
    case DataCommandChoice::SELECTION_ID_PREV:
        return manipulator(&d_prev.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV]);
    case DataCommandChoice::SELECTION_ID_R:
        return manipulator(&d_r.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_R]);
    case DataCommandChoice::SELECTION_ID_RECORD:
        return manipulator(&d_record.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD]);
    case DataCommandChoice::SELECTION_ID_LIST:
        return manipulator(&d_list.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST]);
    case DataCommandChoice::SELECTION_ID_L:
        return manipulator(&d_l.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_L]);
    default:
        BSLS_ASSERT(DataCommandChoice::SELECTION_ID_UNDEFINED ==
                    d_selectionId);
        return -1;
    }
}

inline bsls::Types::Uint64& DataCommandChoice::n()
{
    BSLS_ASSERT(SELECTION_ID_N == d_selectionId);
    return d_n.object();
}

inline bsls::Types::Uint64& DataCommandChoice::next()
{
    BSLS_ASSERT(SELECTION_ID_NEXT == d_selectionId);
    return d_next.object();
}

inline bsls::Types::Uint64& DataCommandChoice::p()
{
    BSLS_ASSERT(SELECTION_ID_P == d_selectionId);
    return d_p.object();
}

inline bsls::Types::Uint64& DataCommandChoice::prev()
{
    BSLS_ASSERT(SELECTION_ID_PREV == d_selectionId);
    return d_prev.object();
}

inline bsls::Types::Uint64& DataCommandChoice::r()
{
    BSLS_ASSERT(SELECTION_ID_R == d_selectionId);
    return d_r.object();
}

inline bsls::Types::Uint64& DataCommandChoice::record()
{
    BSLS_ASSERT(SELECTION_ID_RECORD == d_selectionId);
    return d_record.object();
}

inline int& DataCommandChoice::list()
{
    BSLS_ASSERT(SELECTION_ID_LIST == d_selectionId);
    return d_list.object();
}

inline int& DataCommandChoice::l()
{
    BSLS_ASSERT(SELECTION_ID_L == d_selectionId);
    return d_l.object();
}

// ACCESSORS
inline int DataCommandChoice::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int DataCommandChoice::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_N:
        return accessor(d_n.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_N]);
    case SELECTION_ID_NEXT:
        return accessor(d_next.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT]);
    case SELECTION_ID_P:
        return accessor(d_p.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_P]);
    case SELECTION_ID_PREV:
        return accessor(d_prev.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV]);
    case SELECTION_ID_R:
        return accessor(d_r.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_R]);
    case SELECTION_ID_RECORD:
        return accessor(d_record.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD]);
    case SELECTION_ID_LIST:
        return accessor(d_list.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST]);
    case SELECTION_ID_L:
        return accessor(d_l.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_L]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const bsls::Types::Uint64& DataCommandChoice::n() const
{
    BSLS_ASSERT(SELECTION_ID_N == d_selectionId);
    return d_n.object();
}

inline const bsls::Types::Uint64& DataCommandChoice::next() const
{
    BSLS_ASSERT(SELECTION_ID_NEXT == d_selectionId);
    return d_next.object();
}

inline const bsls::Types::Uint64& DataCommandChoice::p() const
{
    BSLS_ASSERT(SELECTION_ID_P == d_selectionId);
    return d_p.object();
}

inline const bsls::Types::Uint64& DataCommandChoice::prev() const
{
    BSLS_ASSERT(SELECTION_ID_PREV == d_selectionId);
    return d_prev.object();
}

inline const bsls::Types::Uint64& DataCommandChoice::r() const
{
    BSLS_ASSERT(SELECTION_ID_R == d_selectionId);
    return d_r.object();
}

inline const bsls::Types::Uint64& DataCommandChoice::record() const
{
    BSLS_ASSERT(SELECTION_ID_RECORD == d_selectionId);
    return d_record.object();
}

inline const int& DataCommandChoice::list() const
{
    BSLS_ASSERT(SELECTION_ID_LIST == d_selectionId);
    return d_list.object();
}

inline const int& DataCommandChoice::l() const
{
    BSLS_ASSERT(SELECTION_ID_L == d_selectionId);
    return d_l.object();
}

inline bool DataCommandChoice::isNValue() const
{
    return SELECTION_ID_N == d_selectionId;
}

inline bool DataCommandChoice::isNextValue() const
{
    return SELECTION_ID_NEXT == d_selectionId;
}

inline bool DataCommandChoice::isPValue() const
{
    return SELECTION_ID_P == d_selectionId;
}

inline bool DataCommandChoice::isPrevValue() const
{
    return SELECTION_ID_PREV == d_selectionId;
}

inline bool DataCommandChoice::isRValue() const
{
    return SELECTION_ID_R == d_selectionId;
}

inline bool DataCommandChoice::isRecordValue() const
{
    return SELECTION_ID_RECORD == d_selectionId;
}

inline bool DataCommandChoice::isListValue() const
{
    return SELECTION_ID_LIST == d_selectionId;
}

inline bool DataCommandChoice::isLValue() const
{
    return SELECTION_ID_L == d_selectionId;
}

inline bool DataCommandChoice::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}

// ----------------------
// class DumpQueueCommand
// ----------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int DumpQueueCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int DumpQueueCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_KEY: {
        return manipulator(&d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int DumpQueueCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                          const char*    name,
                                          int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& DumpQueueCommand::uri()
{
    return d_uri;
}

inline bsl::string& DumpQueueCommand::key()
{
    return d_key;
}

// ACCESSORS
template <typename t_ACCESSOR>
int DumpQueueCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int DumpQueueCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_KEY: {
        return accessor(d_key, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_KEY]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int DumpQueueCommand::accessAttribute(t_ACCESSOR& accessor,
                                      const char* name,
                                      int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& DumpQueueCommand::uri() const
{
    return d_uri;
}

inline const bsl::string& DumpQueueCommand::key() const
{
    return d_key;
}

// ------------------------------
// class JournalCommandChoiceType
// ------------------------------

// CLASS METHODS
inline int JournalCommandChoiceType::fromString(Value*             result,
                                                const bsl::string& string)
{
    return fromString(result,
                      string.c_str(),
                      static_cast<int>(string.length()));
}

inline bsl::ostream&
JournalCommandChoiceType::print(bsl::ostream&                   stream,
                                JournalCommandChoiceType::Value value)
{
    return stream << toString(value);
}

// -----------------
// class ListCommand
// -----------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ListCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ListCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ListCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                     const char*    name,
                                     int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bdlb::NullableValue<bsl::string>& ListCommand::uri()
{
    return d_uri;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ListCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ListCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ListCommand::accessAttribute(t_ACCESSOR& accessor,
                                 const char* name,
                                 int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bdlb::NullableValue<bsl::string>& ListCommand::uri() const
{
    return d_uri;
}

// -----------------------
// class ListQueuesCommand
// -----------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ListQueuesCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int ListQueuesCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ListQueuesCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                           const char*    name,
                                           int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

// ACCESSORS
template <typename t_ACCESSOR>
int ListQueuesCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int ListQueuesCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ListQueuesCommand::accessAttribute(t_ACCESSOR& accessor,
                                       const char* name,
                                       int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

// ---------------------
// class LoadPostCommand
// ---------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int LoadPostCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_file, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int LoadPostCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_FILE: {
        return manipulator(&d_file,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int LoadPostCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                         const char*    name,
                                         int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& LoadPostCommand::uri()
{
    return d_uri;
}

inline bsl::string& LoadPostCommand::file()
{
    return d_file;
}

// ACCESSORS
template <typename t_ACCESSOR>
int LoadPostCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_file, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int LoadPostCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_FILE: {
        return accessor(d_file, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FILE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int LoadPostCommand::accessAttribute(t_ACCESSOR& accessor,
                                     const char* name,
                                     int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& LoadPostCommand::uri() const
{
    return d_uri;
}

inline const bsl::string& LoadPostCommand::file() const
{
    return d_file;
}

// -------------------------
// class MessagePropertyType
// -------------------------

// CLASS METHODS
inline int MessagePropertyType::fromString(Value*             result,
                                           const bsl::string& string)
{
    return fromString(result,
                      string.c_str(),
                      static_cast<int>(string.length()));
}

inline bsl::ostream&
MessagePropertyType::print(bsl::ostream&              stream,
                           MessagePropertyType::Value value)
{
    return stream << toString(value);
}

// ---------------------
// class MetadataCommand
// ---------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int MetadataCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    (void)manipulator;
    return 0;
}

template <typename t_MANIPULATOR>
int MetadataCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    (void)manipulator;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int MetadataCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                         const char*    name,
                                         int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

// ACCESSORS
template <typename t_ACCESSOR>
int MetadataCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    (void)accessor;
    return 0;
}

template <typename t_ACCESSOR>
int MetadataCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    (void)accessor;
    enum { NOT_FOUND = -1 };

    switch (id) {
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int MetadataCommand::accessAttribute(t_ACCESSOR& accessor,
                                     const char* name,
                                     int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

// ------------------------
// class OpenStorageCommand
// ------------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int OpenStorageCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_path, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PATH]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int OpenStorageCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_PATH: {
        return manipulator(&d_path,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PATH]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int OpenStorageCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                            const char*    name,
                                            int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& OpenStorageCommand::path()
{
    return d_path;
}

// ACCESSORS
template <typename t_ACCESSOR>
int OpenStorageCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_path, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PATH]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int OpenStorageCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_PATH: {
        return accessor(d_path, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PATH]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int OpenStorageCommand::accessAttribute(t_ACCESSOR& accessor,
                                        const char* name,
                                        int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& OpenStorageCommand::path() const
{
    return d_path;
}

// ------------------------
// class QlistCommandChoice
// ------------------------

// CLASS METHODS
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void QlistCommandChoice::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef QlistCommandChoice Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_N: hashAppend(hashAlgorithm, this->n()); break;
    case Class::SELECTION_ID_NEXT:
        hashAppend(hashAlgorithm, this->next());
        break;
    case Class::SELECTION_ID_P: hashAppend(hashAlgorithm, this->p()); break;
    case Class::SELECTION_ID_PREV:
        hashAppend(hashAlgorithm, this->prev());
        break;
    case Class::SELECTION_ID_R: hashAppend(hashAlgorithm, this->r()); break;
    case Class::SELECTION_ID_RECORD:
        hashAppend(hashAlgorithm, this->record());
        break;
    case Class::SELECTION_ID_LIST:
        hashAppend(hashAlgorithm, this->list());
        break;
    case Class::SELECTION_ID_L: hashAppend(hashAlgorithm, this->l()); break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool QlistCommandChoice::isEqualTo(const QlistCommandChoice& rhs) const
{
    typedef QlistCommandChoice Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_N: return this->n() == rhs.n();
        case Class::SELECTION_ID_NEXT: return this->next() == rhs.next();
        case Class::SELECTION_ID_P: return this->p() == rhs.p();
        case Class::SELECTION_ID_PREV: return this->prev() == rhs.prev();
        case Class::SELECTION_ID_R: return this->r() == rhs.r();
        case Class::SELECTION_ID_RECORD: return this->record() == rhs.record();
        case Class::SELECTION_ID_LIST: return this->list() == rhs.list();
        case Class::SELECTION_ID_L: return this->l() == rhs.l();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

// CREATORS
inline QlistCommandChoice::QlistCommandChoice()
: d_selectionId(SELECTION_ID_UNDEFINED)
{
}

inline QlistCommandChoice::~QlistCommandChoice()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int QlistCommandChoice::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case QlistCommandChoice::SELECTION_ID_N:
        return manipulator(&d_n.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_N]);
    case QlistCommandChoice::SELECTION_ID_NEXT:
        return manipulator(&d_next.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT]);
    case QlistCommandChoice::SELECTION_ID_P:
        return manipulator(&d_p.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_P]);
    case QlistCommandChoice::SELECTION_ID_PREV:
        return manipulator(&d_prev.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV]);
    case QlistCommandChoice::SELECTION_ID_R:
        return manipulator(&d_r.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_R]);
    case QlistCommandChoice::SELECTION_ID_RECORD:
        return manipulator(&d_record.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD]);
    case QlistCommandChoice::SELECTION_ID_LIST:
        return manipulator(&d_list.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST]);
    case QlistCommandChoice::SELECTION_ID_L:
        return manipulator(&d_l.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_L]);
    default:
        BSLS_ASSERT(QlistCommandChoice::SELECTION_ID_UNDEFINED ==
                    d_selectionId);
        return -1;
    }
}

inline bsls::Types::Uint64& QlistCommandChoice::n()
{
    BSLS_ASSERT(SELECTION_ID_N == d_selectionId);
    return d_n.object();
}

inline bsls::Types::Uint64& QlistCommandChoice::next()
{
    BSLS_ASSERT(SELECTION_ID_NEXT == d_selectionId);
    return d_next.object();
}

inline bsls::Types::Uint64& QlistCommandChoice::p()
{
    BSLS_ASSERT(SELECTION_ID_P == d_selectionId);
    return d_p.object();
}

inline bsls::Types::Uint64& QlistCommandChoice::prev()
{
    BSLS_ASSERT(SELECTION_ID_PREV == d_selectionId);
    return d_prev.object();
}

inline bsls::Types::Uint64& QlistCommandChoice::r()
{
    BSLS_ASSERT(SELECTION_ID_R == d_selectionId);
    return d_r.object();
}

inline bsls::Types::Uint64& QlistCommandChoice::record()
{
    BSLS_ASSERT(SELECTION_ID_RECORD == d_selectionId);
    return d_record.object();
}

inline int& QlistCommandChoice::list()
{
    BSLS_ASSERT(SELECTION_ID_LIST == d_selectionId);
    return d_list.object();
}

inline int& QlistCommandChoice::l()
{
    BSLS_ASSERT(SELECTION_ID_L == d_selectionId);
    return d_l.object();
}

// ACCESSORS
inline int QlistCommandChoice::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int QlistCommandChoice::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_N:
        return accessor(d_n.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_N]);
    case SELECTION_ID_NEXT:
        return accessor(d_next.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT]);
    case SELECTION_ID_P:
        return accessor(d_p.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_P]);
    case SELECTION_ID_PREV:
        return accessor(d_prev.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV]);
    case SELECTION_ID_R:
        return accessor(d_r.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_R]);
    case SELECTION_ID_RECORD:
        return accessor(d_record.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD]);
    case SELECTION_ID_LIST:
        return accessor(d_list.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST]);
    case SELECTION_ID_L:
        return accessor(d_l.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_L]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const bsls::Types::Uint64& QlistCommandChoice::n() const
{
    BSLS_ASSERT(SELECTION_ID_N == d_selectionId);
    return d_n.object();
}

inline const bsls::Types::Uint64& QlistCommandChoice::next() const
{
    BSLS_ASSERT(SELECTION_ID_NEXT == d_selectionId);
    return d_next.object();
}

inline const bsls::Types::Uint64& QlistCommandChoice::p() const
{
    BSLS_ASSERT(SELECTION_ID_P == d_selectionId);
    return d_p.object();
}

inline const bsls::Types::Uint64& QlistCommandChoice::prev() const
{
    BSLS_ASSERT(SELECTION_ID_PREV == d_selectionId);
    return d_prev.object();
}

inline const bsls::Types::Uint64& QlistCommandChoice::r() const
{
    BSLS_ASSERT(SELECTION_ID_R == d_selectionId);
    return d_r.object();
}

inline const bsls::Types::Uint64& QlistCommandChoice::record() const
{
    BSLS_ASSERT(SELECTION_ID_RECORD == d_selectionId);
    return d_record.object();
}

inline const int& QlistCommandChoice::list() const
{
    BSLS_ASSERT(SELECTION_ID_LIST == d_selectionId);
    return d_list.object();
}

inline const int& QlistCommandChoice::l() const
{
    BSLS_ASSERT(SELECTION_ID_L == d_selectionId);
    return d_l.object();
}

inline bool QlistCommandChoice::isNValue() const
{
    return SELECTION_ID_N == d_selectionId;
}

inline bool QlistCommandChoice::isNextValue() const
{
    return SELECTION_ID_NEXT == d_selectionId;
}

inline bool QlistCommandChoice::isPValue() const
{
    return SELECTION_ID_P == d_selectionId;
}

inline bool QlistCommandChoice::isPrevValue() const
{
    return SELECTION_ID_PREV == d_selectionId;
}

inline bool QlistCommandChoice::isRValue() const
{
    return SELECTION_ID_R == d_selectionId;
}

inline bool QlistCommandChoice::isRecordValue() const
{
    return SELECTION_ID_RECORD == d_selectionId;
}

inline bool QlistCommandChoice::isListValue() const
{
    return SELECTION_ID_LIST == d_selectionId;
}

inline bool QlistCommandChoice::isLValue() const
{
    return SELECTION_ID_L == d_selectionId;
}

inline bool QlistCommandChoice::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}

// ------------------
// class StartCommand
// ------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StartCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StartCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ASYNC: {
        return manipulator(&d_async,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StartCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                      const char*    name,
                                      int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bool& StartCommand::async()
{
    return d_async;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StartCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StartCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ASYNC: {
        return accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StartCommand::accessAttribute(t_ACCESSOR& accessor,
                                  const char* name,
                                  int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline bool StartCommand::async() const
{
    return d_async;
}

// -----------------
// class StopCommand
// -----------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int StopCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int StopCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ASYNC: {
        return manipulator(&d_async,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int StopCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                     const char*    name,
                                     int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bool& StopCommand::async()
{
    return d_async;
}

// ACCESSORS
template <typename t_ACCESSOR>
int StopCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int StopCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_ASYNC: {
        return accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int StopCommand::accessAttribute(t_ACCESSOR& accessor,
                                 const char* name,
                                 int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline bool StopCommand::async() const
{
    return d_async;
}

// ------------------
// class Subscription
// ------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void Subscription::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->correlationId());
    hashAppend(hashAlgorithm, this->expression());
    hashAppend(hashAlgorithm, this->maxUnconfirmedMessages());
    hashAppend(hashAlgorithm, this->maxUnconfirmedBytes());
    hashAppend(hashAlgorithm, this->consumerPriority());
}

inline bool Subscription::isEqualTo(const Subscription& rhs) const
{
    return this->correlationId() == rhs.correlationId() &&
           this->expression() == rhs.expression() &&
           this->maxUnconfirmedMessages() == rhs.maxUnconfirmedMessages() &&
           this->maxUnconfirmedBytes() == rhs.maxUnconfirmedBytes() &&
           this->consumerPriority() == rhs.consumerPriority();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int Subscription::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_correlationId,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CORRELATION_ID]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_expression,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxUnconfirmedMessages,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxUnconfirmedBytes,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_consumerPriority,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int Subscription::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CORRELATION_ID: {
        return manipulator(
            &d_correlationId,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CORRELATION_ID]);
    }
    case ATTRIBUTE_ID_EXPRESSION: {
        return manipulator(&d_expression,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES: {
        return manipulator(
            &d_maxUnconfirmedMessages,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES: {
        return manipulator(
            &d_maxUnconfirmedBytes,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    }
    case ATTRIBUTE_ID_CONSUMER_PRIORITY: {
        return manipulator(
            &d_consumerPriority,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int Subscription::manipulateAttribute(t_MANIPULATOR& manipulator,
                                      const char*    name,
                                      int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bdlb::NullableValue<unsigned int>& Subscription::correlationId()
{
    return d_correlationId;
}

inline bdlb::NullableValue<bsl::string>& Subscription::expression()
{
    return d_expression;
}

inline bdlb::NullableValue<int>& Subscription::maxUnconfirmedMessages()
{
    return d_maxUnconfirmedMessages;
}

inline bdlb::NullableValue<int>& Subscription::maxUnconfirmedBytes()
{
    return d_maxUnconfirmedBytes;
}

inline bdlb::NullableValue<int>& Subscription::consumerPriority()
{
    return d_consumerPriority;
}

// ACCESSORS
template <typename t_ACCESSOR>
int Subscription::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_correlationId,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CORRELATION_ID]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_expression,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_maxUnconfirmedMessages,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_maxUnconfirmedBytes,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_consumerPriority,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int Subscription::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CORRELATION_ID: {
        return accessor(d_correlationId,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CORRELATION_ID]);
    }
    case ATTRIBUTE_ID_EXPRESSION: {
        return accessor(d_expression,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EXPRESSION]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES: {
        return accessor(
            d_maxUnconfirmedMessages,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES: {
        return accessor(
            d_maxUnconfirmedBytes,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    }
    case ATTRIBUTE_ID_CONSUMER_PRIORITY: {
        return accessor(
            d_consumerPriority,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int Subscription::accessAttribute(t_ACCESSOR& accessor,
                                  const char* name,
                                  int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bdlb::NullableValue<unsigned int>&
Subscription::correlationId() const
{
    return d_correlationId;
}

inline const bdlb::NullableValue<bsl::string>& Subscription::expression() const
{
    return d_expression;
}

inline const bdlb::NullableValue<int>&
Subscription::maxUnconfirmedMessages() const
{
    return d_maxUnconfirmedMessages;
}

inline const bdlb::NullableValue<int>&
Subscription::maxUnconfirmedBytes() const
{
    return d_maxUnconfirmedBytes;
}

inline const bdlb::NullableValue<int>& Subscription::consumerPriority() const
{
    return d_consumerPriority;
}

// ---------------------------
// class ConfigureQueueCommand
// ---------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void ConfigureQueueCommand::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->uri());
    hashAppend(hashAlgorithm, this->async());
    hashAppend(hashAlgorithm, this->maxUnconfirmedMessages());
    hashAppend(hashAlgorithm, this->maxUnconfirmedBytes());
    hashAppend(hashAlgorithm, this->consumerPriority());
    hashAppend(hashAlgorithm, this->subscriptions());
}

inline bool
ConfigureQueueCommand::isEqualTo(const ConfigureQueueCommand& rhs) const
{
    return this->uri() == rhs.uri() && this->async() == rhs.async() &&
           this->maxUnconfirmedMessages() == rhs.maxUnconfirmedMessages() &&
           this->maxUnconfirmedBytes() == rhs.maxUnconfirmedBytes() &&
           this->consumerPriority() == rhs.consumerPriority() &&
           this->subscriptions() == rhs.subscriptions();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int ConfigureQueueCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxUnconfirmedMessages,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxUnconfirmedBytes,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_consumerPriority,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_subscriptions,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int ConfigureQueueCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                               int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_ASYNC: {
        return manipulator(&d_async,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES: {
        return manipulator(
            &d_maxUnconfirmedMessages,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES: {
        return manipulator(
            &d_maxUnconfirmedBytes,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    }
    case ATTRIBUTE_ID_CONSUMER_PRIORITY: {
        return manipulator(
            &d_consumerPriority,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    }
    case ATTRIBUTE_ID_SUBSCRIPTIONS: {
        return manipulator(
            &d_subscriptions,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int ConfigureQueueCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                               const char*    name,
                                               int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& ConfigureQueueCommand::uri()
{
    return d_uri;
}

inline bool& ConfigureQueueCommand::async()
{
    return d_async;
}

inline int& ConfigureQueueCommand::maxUnconfirmedMessages()
{
    return d_maxUnconfirmedMessages;
}

inline int& ConfigureQueueCommand::maxUnconfirmedBytes()
{
    return d_maxUnconfirmedBytes;
}

inline int& ConfigureQueueCommand::consumerPriority()
{
    return d_consumerPriority;
}

inline bsl::vector<Subscription>& ConfigureQueueCommand::subscriptions()
{
    return d_subscriptions;
}

// ACCESSORS
template <typename t_ACCESSOR>
int ConfigureQueueCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_maxUnconfirmedMessages,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_maxUnconfirmedBytes,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_consumerPriority,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_subscriptions,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int ConfigureQueueCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_ASYNC: {
        return accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES: {
        return accessor(
            d_maxUnconfirmedMessages,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES: {
        return accessor(
            d_maxUnconfirmedBytes,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    }
    case ATTRIBUTE_ID_CONSUMER_PRIORITY: {
        return accessor(
            d_consumerPriority,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    }
    case ATTRIBUTE_ID_SUBSCRIPTIONS: {
        return accessor(d_subscriptions,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int ConfigureQueueCommand::accessAttribute(t_ACCESSOR& accessor,
                                           const char* name,
                                           int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& ConfigureQueueCommand::uri() const
{
    return d_uri;
}

inline bool ConfigureQueueCommand::async() const
{
    return d_async;
}

inline int ConfigureQueueCommand::maxUnconfirmedMessages() const
{
    return d_maxUnconfirmedMessages;
}

inline int ConfigureQueueCommand::maxUnconfirmedBytes() const
{
    return d_maxUnconfirmedBytes;
}

inline int ConfigureQueueCommand::consumerPriority() const
{
    return d_consumerPriority;
}

inline const bsl::vector<Subscription>&
ConfigureQueueCommand::subscriptions() const
{
    return d_subscriptions;
}

// -----------------
// class DataCommand
// -----------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int DataCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_choice, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int DataCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CHOICE: {
        return manipulator(&d_choice,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int DataCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                     const char*    name,
                                     int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline DataCommandChoice& DataCommand::choice()
{
    return d_choice;
}

// ACCESSORS
template <typename t_ACCESSOR>
int DataCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_choice, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int DataCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CHOICE: {
        return accessor(d_choice,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int DataCommand::accessAttribute(t_ACCESSOR& accessor,
                                 const char* name,
                                 int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const DataCommandChoice& DataCommand::choice() const
{
    return d_choice;
}

// --------------------------
// class JournalCommandChoice
// --------------------------

// CLASS METHODS
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void JournalCommandChoice::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef JournalCommandChoice Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_N: hashAppend(hashAlgorithm, this->n()); break;
    case Class::SELECTION_ID_NEXT:
        hashAppend(hashAlgorithm, this->next());
        break;
    case Class::SELECTION_ID_P: hashAppend(hashAlgorithm, this->p()); break;
    case Class::SELECTION_ID_PREV:
        hashAppend(hashAlgorithm, this->prev());
        break;
    case Class::SELECTION_ID_R: hashAppend(hashAlgorithm, this->r()); break;
    case Class::SELECTION_ID_RECORD:
        hashAppend(hashAlgorithm, this->record());
        break;
    case Class::SELECTION_ID_LIST:
        hashAppend(hashAlgorithm, this->list());
        break;
    case Class::SELECTION_ID_L: hashAppend(hashAlgorithm, this->l()); break;
    case Class::SELECTION_ID_DUMP:
        hashAppend(hashAlgorithm, this->dump());
        break;
    case Class::SELECTION_ID_TYPE:
        hashAppend(hashAlgorithm, this->type());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool
JournalCommandChoice::isEqualTo(const JournalCommandChoice& rhs) const
{
    typedef JournalCommandChoice Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_N: return this->n() == rhs.n();
        case Class::SELECTION_ID_NEXT: return this->next() == rhs.next();
        case Class::SELECTION_ID_P: return this->p() == rhs.p();
        case Class::SELECTION_ID_PREV: return this->prev() == rhs.prev();
        case Class::SELECTION_ID_R: return this->r() == rhs.r();
        case Class::SELECTION_ID_RECORD: return this->record() == rhs.record();
        case Class::SELECTION_ID_LIST: return this->list() == rhs.list();
        case Class::SELECTION_ID_L: return this->l() == rhs.l();
        case Class::SELECTION_ID_DUMP: return this->dump() == rhs.dump();
        case Class::SELECTION_ID_TYPE: return this->type() == rhs.type();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

// CREATORS
inline JournalCommandChoice::JournalCommandChoice(
    bslma::Allocator* basicAllocator)
: d_selectionId(SELECTION_ID_UNDEFINED)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

inline JournalCommandChoice::~JournalCommandChoice()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int JournalCommandChoice::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case JournalCommandChoice::SELECTION_ID_N:
        return manipulator(&d_n.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_N]);
    case JournalCommandChoice::SELECTION_ID_NEXT:
        return manipulator(&d_next.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT]);
    case JournalCommandChoice::SELECTION_ID_P:
        return manipulator(&d_p.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_P]);
    case JournalCommandChoice::SELECTION_ID_PREV:
        return manipulator(&d_prev.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV]);
    case JournalCommandChoice::SELECTION_ID_R:
        return manipulator(&d_r.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_R]);
    case JournalCommandChoice::SELECTION_ID_RECORD:
        return manipulator(&d_record.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD]);
    case JournalCommandChoice::SELECTION_ID_LIST:
        return manipulator(&d_list.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST]);
    case JournalCommandChoice::SELECTION_ID_L:
        return manipulator(&d_l.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_L]);
    case JournalCommandChoice::SELECTION_ID_DUMP:
        return manipulator(&d_dump.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_DUMP]);
    case JournalCommandChoice::SELECTION_ID_TYPE:
        return manipulator(&d_type.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_TYPE]);
    default:
        BSLS_ASSERT(JournalCommandChoice::SELECTION_ID_UNDEFINED ==
                    d_selectionId);
        return -1;
    }
}

inline bsls::Types::Uint64& JournalCommandChoice::n()
{
    BSLS_ASSERT(SELECTION_ID_N == d_selectionId);
    return d_n.object();
}

inline bsls::Types::Uint64& JournalCommandChoice::next()
{
    BSLS_ASSERT(SELECTION_ID_NEXT == d_selectionId);
    return d_next.object();
}

inline bsls::Types::Uint64& JournalCommandChoice::p()
{
    BSLS_ASSERT(SELECTION_ID_P == d_selectionId);
    return d_p.object();
}

inline bsls::Types::Uint64& JournalCommandChoice::prev()
{
    BSLS_ASSERT(SELECTION_ID_PREV == d_selectionId);
    return d_prev.object();
}

inline bsls::Types::Uint64& JournalCommandChoice::r()
{
    BSLS_ASSERT(SELECTION_ID_R == d_selectionId);
    return d_r.object();
}

inline bsls::Types::Uint64& JournalCommandChoice::record()
{
    BSLS_ASSERT(SELECTION_ID_RECORD == d_selectionId);
    return d_record.object();
}

inline int& JournalCommandChoice::list()
{
    BSLS_ASSERT(SELECTION_ID_LIST == d_selectionId);
    return d_list.object();
}

inline int& JournalCommandChoice::l()
{
    BSLS_ASSERT(SELECTION_ID_L == d_selectionId);
    return d_l.object();
}

inline bsl::string& JournalCommandChoice::dump()
{
    BSLS_ASSERT(SELECTION_ID_DUMP == d_selectionId);
    return d_dump.object();
}

inline JournalCommandChoiceType::Value& JournalCommandChoice::type()
{
    BSLS_ASSERT(SELECTION_ID_TYPE == d_selectionId);
    return d_type.object();
}

// ACCESSORS
inline int JournalCommandChoice::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int JournalCommandChoice::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_N:
        return accessor(d_n.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_N]);
    case SELECTION_ID_NEXT:
        return accessor(d_next.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_NEXT]);
    case SELECTION_ID_P:
        return accessor(d_p.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_P]);
    case SELECTION_ID_PREV:
        return accessor(d_prev.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_PREV]);
    case SELECTION_ID_R:
        return accessor(d_r.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_R]);
    case SELECTION_ID_RECORD:
        return accessor(d_record.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_RECORD]);
    case SELECTION_ID_LIST:
        return accessor(d_list.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST]);
    case SELECTION_ID_L:
        return accessor(d_l.object(), SELECTION_INFO_ARRAY[SELECTION_INDEX_L]);
    case SELECTION_ID_DUMP:
        return accessor(d_dump.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_DUMP]);
    case SELECTION_ID_TYPE:
        return accessor(d_type.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_TYPE]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const bsls::Types::Uint64& JournalCommandChoice::n() const
{
    BSLS_ASSERT(SELECTION_ID_N == d_selectionId);
    return d_n.object();
}

inline const bsls::Types::Uint64& JournalCommandChoice::next() const
{
    BSLS_ASSERT(SELECTION_ID_NEXT == d_selectionId);
    return d_next.object();
}

inline const bsls::Types::Uint64& JournalCommandChoice::p() const
{
    BSLS_ASSERT(SELECTION_ID_P == d_selectionId);
    return d_p.object();
}

inline const bsls::Types::Uint64& JournalCommandChoice::prev() const
{
    BSLS_ASSERT(SELECTION_ID_PREV == d_selectionId);
    return d_prev.object();
}

inline const bsls::Types::Uint64& JournalCommandChoice::r() const
{
    BSLS_ASSERT(SELECTION_ID_R == d_selectionId);
    return d_r.object();
}

inline const bsls::Types::Uint64& JournalCommandChoice::record() const
{
    BSLS_ASSERT(SELECTION_ID_RECORD == d_selectionId);
    return d_record.object();
}

inline const int& JournalCommandChoice::list() const
{
    BSLS_ASSERT(SELECTION_ID_LIST == d_selectionId);
    return d_list.object();
}

inline const int& JournalCommandChoice::l() const
{
    BSLS_ASSERT(SELECTION_ID_L == d_selectionId);
    return d_l.object();
}

inline const bsl::string& JournalCommandChoice::dump() const
{
    BSLS_ASSERT(SELECTION_ID_DUMP == d_selectionId);
    return d_dump.object();
}

inline const JournalCommandChoiceType::Value&
JournalCommandChoice::type() const
{
    BSLS_ASSERT(SELECTION_ID_TYPE == d_selectionId);
    return d_type.object();
}

inline bool JournalCommandChoice::isNValue() const
{
    return SELECTION_ID_N == d_selectionId;
}

inline bool JournalCommandChoice::isNextValue() const
{
    return SELECTION_ID_NEXT == d_selectionId;
}

inline bool JournalCommandChoice::isPValue() const
{
    return SELECTION_ID_P == d_selectionId;
}

inline bool JournalCommandChoice::isPrevValue() const
{
    return SELECTION_ID_PREV == d_selectionId;
}

inline bool JournalCommandChoice::isRValue() const
{
    return SELECTION_ID_R == d_selectionId;
}

inline bool JournalCommandChoice::isRecordValue() const
{
    return SELECTION_ID_RECORD == d_selectionId;
}

inline bool JournalCommandChoice::isListValue() const
{
    return SELECTION_ID_LIST == d_selectionId;
}

inline bool JournalCommandChoice::isLValue() const
{
    return SELECTION_ID_L == d_selectionId;
}

inline bool JournalCommandChoice::isDumpValue() const
{
    return SELECTION_ID_DUMP == d_selectionId;
}

inline bool JournalCommandChoice::isTypeValue() const
{
    return SELECTION_ID_TYPE == d_selectionId;
}

inline bool JournalCommandChoice::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}

// ---------------------
// class MessageProperty
// ---------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void MessageProperty::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->name());
    hashAppend(hashAlgorithm, this->value());
    hashAppend(hashAlgorithm, this->type());
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int MessageProperty::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_value, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_type, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TYPE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int MessageProperty::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return manipulator(&d_name,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_VALUE: {
        return manipulator(&d_value,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUE]);
    }
    case ATTRIBUTE_ID_TYPE: {
        return manipulator(&d_type,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TYPE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int MessageProperty::manipulateAttribute(t_MANIPULATOR& manipulator,
                                         const char*    name,
                                         int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& MessageProperty::name()
{
    return d_name;
}

inline bsl::string& MessageProperty::value()
{
    return d_value;
}

inline MessagePropertyType::Value& MessageProperty::type()
{
    return d_type;
}

// ACCESSORS
template <typename t_ACCESSOR>
int MessageProperty::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_value, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_type, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TYPE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int MessageProperty::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_NAME: {
        return accessor(d_name, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NAME]);
    }
    case ATTRIBUTE_ID_VALUE: {
        return accessor(d_value, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VALUE]);
    }
    case ATTRIBUTE_ID_TYPE: {
        return accessor(d_type, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_TYPE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int MessageProperty::accessAttribute(t_ACCESSOR& accessor,
                                     const char* name,
                                     int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& MessageProperty::name() const
{
    return d_name;
}

inline const bsl::string& MessageProperty::value() const
{
    return d_value;
}

inline MessagePropertyType::Value MessageProperty::type() const
{
    return d_type;
}

// ----------------------
// class OpenQueueCommand
// ----------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void OpenQueueCommand::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->uri());
    hashAppend(hashAlgorithm, this->flags());
    hashAppend(hashAlgorithm, this->async());
    hashAppend(hashAlgorithm, this->maxUnconfirmedMessages());
    hashAppend(hashAlgorithm, this->maxUnconfirmedBytes());
    hashAppend(hashAlgorithm, this->consumerPriority());
    hashAppend(hashAlgorithm, this->subscriptions());
}

inline bool OpenQueueCommand::isEqualTo(const OpenQueueCommand& rhs) const
{
    return this->uri() == rhs.uri() && this->flags() == rhs.flags() &&
           this->async() == rhs.async() &&
           this->maxUnconfirmedMessages() == rhs.maxUnconfirmedMessages() &&
           this->maxUnconfirmedBytes() == rhs.maxUnconfirmedBytes() &&
           this->consumerPriority() == rhs.consumerPriority() &&
           this->subscriptions() == rhs.subscriptions();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int OpenQueueCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_flags, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxUnconfirmedMessages,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_maxUnconfirmedBytes,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_consumerPriority,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_subscriptions,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int OpenQueueCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_FLAGS: {
        return manipulator(&d_flags,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    }
    case ATTRIBUTE_ID_ASYNC: {
        return manipulator(&d_async,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES: {
        return manipulator(
            &d_maxUnconfirmedMessages,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES: {
        return manipulator(
            &d_maxUnconfirmedBytes,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    }
    case ATTRIBUTE_ID_CONSUMER_PRIORITY: {
        return manipulator(
            &d_consumerPriority,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    }
    case ATTRIBUTE_ID_SUBSCRIPTIONS: {
        return manipulator(
            &d_subscriptions,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int OpenQueueCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                          const char*    name,
                                          int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& OpenQueueCommand::uri()
{
    return d_uri;
}

inline bsl::string& OpenQueueCommand::flags()
{
    return d_flags;
}

inline bool& OpenQueueCommand::async()
{
    return d_async;
}

inline int& OpenQueueCommand::maxUnconfirmedMessages()
{
    return d_maxUnconfirmedMessages;
}

inline int& OpenQueueCommand::maxUnconfirmedBytes()
{
    return d_maxUnconfirmedBytes;
}

inline int& OpenQueueCommand::consumerPriority()
{
    return d_consumerPriority;
}

inline bsl::vector<Subscription>& OpenQueueCommand::subscriptions()
{
    return d_subscriptions;
}

// ACCESSORS
template <typename t_ACCESSOR>
int OpenQueueCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_flags, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_maxUnconfirmedMessages,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_maxUnconfirmedBytes,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_consumerPriority,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_subscriptions,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int OpenQueueCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_FLAGS: {
        return accessor(d_flags, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_FLAGS]);
    }
    case ATTRIBUTE_ID_ASYNC: {
        return accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_MESSAGES: {
        return accessor(
            d_maxUnconfirmedMessages,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_MESSAGES]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED_BYTES: {
        return accessor(
            d_maxUnconfirmedBytes,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED_BYTES]);
    }
    case ATTRIBUTE_ID_CONSUMER_PRIORITY: {
        return accessor(
            d_consumerPriority,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONSUMER_PRIORITY]);
    }
    case ATTRIBUTE_ID_SUBSCRIPTIONS: {
        return accessor(d_subscriptions,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int OpenQueueCommand::accessAttribute(t_ACCESSOR& accessor,
                                      const char* name,
                                      int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& OpenQueueCommand::uri() const
{
    return d_uri;
}

inline const bsl::string& OpenQueueCommand::flags() const
{
    return d_flags;
}

inline bool OpenQueueCommand::async() const
{
    return d_async;
}

inline int OpenQueueCommand::maxUnconfirmedMessages() const
{
    return d_maxUnconfirmedMessages;
}

inline int OpenQueueCommand::maxUnconfirmedBytes() const
{
    return d_maxUnconfirmedBytes;
}

inline int OpenQueueCommand::consumerPriority() const
{
    return d_consumerPriority;
}

inline const bsl::vector<Subscription>& OpenQueueCommand::subscriptions() const
{
    return d_subscriptions;
}

// ------------------
// class QlistCommand
// ------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int QlistCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_choice, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int QlistCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CHOICE: {
        return manipulator(&d_choice,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int QlistCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                      const char*    name,
                                      int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline QlistCommandChoice& QlistCommand::choice()
{
    return d_choice;
}

// ACCESSORS
template <typename t_ACCESSOR>
int QlistCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_choice, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int QlistCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CHOICE: {
        return accessor(d_choice,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int QlistCommand::accessAttribute(t_ACCESSOR& accessor,
                                  const char* name,
                                  int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const QlistCommandChoice& QlistCommand::choice() const
{
    return d_choice;
}

// ---------------------------
// class CommandLineParameters
// ---------------------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void CommandLineParameters::hashAppendImpl(
    t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->mode());
    hashAppend(hashAlgorithm, this->broker());
    hashAppend(hashAlgorithm, this->queueUri());
    hashAppend(hashAlgorithm, this->queueFlags());
    hashAppend(hashAlgorithm, this->latency());
    hashAppend(hashAlgorithm, this->latencyReport());
    hashAppend(hashAlgorithm, this->dumpMsg());
    hashAppend(hashAlgorithm, this->confirmMsg());
    hashAppend(hashAlgorithm, this->eventSize());
    hashAppend(hashAlgorithm, this->msgSize());
    hashAppend(hashAlgorithm, this->postRate());
    hashAppend(hashAlgorithm, this->eventsCount());
    hashAppend(hashAlgorithm, this->maxUnconfirmed());
    hashAppend(hashAlgorithm, this->postInterval());
    hashAppend(hashAlgorithm, this->verbosity());
    hashAppend(hashAlgorithm, this->logFormat());
    hashAppend(hashAlgorithm, this->memoryDebug());
    hashAppend(hashAlgorithm, this->threads());
    hashAppend(hashAlgorithm, this->shutdownGrace());
    hashAppend(hashAlgorithm, this->noSessionEventHandler());
    hashAppend(hashAlgorithm, this->storage());
    hashAppend(hashAlgorithm, this->log());
    hashAppend(hashAlgorithm, this->sequentialMessagePattern());
    hashAppend(hashAlgorithm, this->messageProperties());
    hashAppend(hashAlgorithm, this->subscriptions());
}

inline bool
CommandLineParameters::isEqualTo(const CommandLineParameters& rhs) const
{
    return this->mode() == rhs.mode() && this->broker() == rhs.broker() &&
           this->queueUri() == rhs.queueUri() &&
           this->queueFlags() == rhs.queueFlags() &&
           this->latency() == rhs.latency() &&
           this->latencyReport() == rhs.latencyReport() &&
           this->dumpMsg() == rhs.dumpMsg() &&
           this->confirmMsg() == rhs.confirmMsg() &&
           this->eventSize() == rhs.eventSize() &&
           this->msgSize() == rhs.msgSize() &&
           this->postRate() == rhs.postRate() &&
           this->eventsCount() == rhs.eventsCount() &&
           this->maxUnconfirmed() == rhs.maxUnconfirmed() &&
           this->postInterval() == rhs.postInterval() &&
           this->verbosity() == rhs.verbosity() &&
           this->logFormat() == rhs.logFormat() &&
           this->memoryDebug() == rhs.memoryDebug() &&
           this->threads() == rhs.threads() &&
           this->shutdownGrace() == rhs.shutdownGrace() &&
           this->noSessionEventHandler() == rhs.noSessionEventHandler() &&
           this->storage() == rhs.storage() && this->log() == rhs.log() &&
           this->sequentialMessagePattern() ==
               rhs.sequentialMessagePattern() &&
           this->messageProperties() == rhs.messageProperties() &&
           this->subscriptions() == rhs.subscriptions();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int CommandLineParameters::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_mode, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_broker, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_queueUri,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_URI]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_queueFlags,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_FLAGS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_latency,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_latencyReport,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY_REPORT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_dumpMsg,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_MSG]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_confirmMsg,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIRM_MSG]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_eventSize,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENT_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_msgSize,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_SIZE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_postRate,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_RATE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_eventsCount,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENTS_COUNT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_maxUnconfirmed,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_postInterval,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_verbosity,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERBOSITY]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_logFormat,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_FORMAT]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_memoryDebug,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MEMORY_DEBUG]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_threads,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THREADS]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_shutdownGrace,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SHUTDOWN_GRACE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_noSessionEventHandler,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NO_SESSION_EVENT_HANDLER]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_storage,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_log, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_sequentialMessagePattern,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENTIAL_MESSAGE_PATTERN]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_messageProperties,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_subscriptions,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int CommandLineParameters::manipulateAttribute(t_MANIPULATOR& manipulator,
                                               int            id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_MODE: {
        return manipulator(&d_mode,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    }
    case ATTRIBUTE_ID_BROKER: {
        return manipulator(&d_broker,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER]);
    }
    case ATTRIBUTE_ID_QUEUE_URI: {
        return manipulator(&d_queueUri,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_URI]);
    }
    case ATTRIBUTE_ID_QUEUE_FLAGS: {
        return manipulator(&d_queueFlags,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_FLAGS]);
    }
    case ATTRIBUTE_ID_LATENCY: {
        return manipulator(&d_latency,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY]);
    }
    case ATTRIBUTE_ID_LATENCY_REPORT: {
        return manipulator(
            &d_latencyReport,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY_REPORT]);
    }
    case ATTRIBUTE_ID_DUMP_MSG: {
        return manipulator(&d_dumpMsg,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_MSG]);
    }
    case ATTRIBUTE_ID_CONFIRM_MSG: {
        return manipulator(&d_confirmMsg,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIRM_MSG]);
    }
    case ATTRIBUTE_ID_EVENT_SIZE: {
        return manipulator(&d_eventSize,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENT_SIZE]);
    }
    case ATTRIBUTE_ID_MSG_SIZE: {
        return manipulator(&d_msgSize,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_SIZE]);
    }
    case ATTRIBUTE_ID_POST_RATE: {
        return manipulator(&d_postRate,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_RATE]);
    }
    case ATTRIBUTE_ID_EVENTS_COUNT: {
        return manipulator(&d_eventsCount,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENTS_COUNT]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED: {
        return manipulator(
            &d_maxUnconfirmed,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED]);
    }
    case ATTRIBUTE_ID_POST_INTERVAL: {
        return manipulator(
            &d_postInterval,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_INTERVAL]);
    }
    case ATTRIBUTE_ID_VERBOSITY: {
        return manipulator(&d_verbosity,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERBOSITY]);
    }
    case ATTRIBUTE_ID_LOG_FORMAT: {
        return manipulator(&d_logFormat,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_FORMAT]);
    }
    case ATTRIBUTE_ID_MEMORY_DEBUG: {
        return manipulator(&d_memoryDebug,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MEMORY_DEBUG]);
    }
    case ATTRIBUTE_ID_THREADS: {
        return manipulator(&d_threads,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THREADS]);
    }
    case ATTRIBUTE_ID_SHUTDOWN_GRACE: {
        return manipulator(
            &d_shutdownGrace,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SHUTDOWN_GRACE]);
    }
    case ATTRIBUTE_ID_NO_SESSION_EVENT_HANDLER: {
        return manipulator(
            &d_noSessionEventHandler,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NO_SESSION_EVENT_HANDLER]);
    }
    case ATTRIBUTE_ID_STORAGE: {
        return manipulator(&d_storage,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE]);
    }
    case ATTRIBUTE_ID_LOG: {
        return manipulator(&d_log, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG]);
    }
    case ATTRIBUTE_ID_SEQUENTIAL_MESSAGE_PATTERN: {
        return manipulator(
            &d_sequentialMessagePattern,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENTIAL_MESSAGE_PATTERN]);
    }
    case ATTRIBUTE_ID_MESSAGE_PROPERTIES: {
        return manipulator(
            &d_messageProperties,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES]);
    }
    case ATTRIBUTE_ID_SUBSCRIPTIONS: {
        return manipulator(
            &d_subscriptions,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int CommandLineParameters::manipulateAttribute(t_MANIPULATOR& manipulator,
                                               const char*    name,
                                               int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& CommandLineParameters::mode()
{
    return d_mode;
}

inline bsl::string& CommandLineParameters::broker()
{
    return d_broker;
}

inline bsl::string& CommandLineParameters::queueUri()
{
    return d_queueUri;
}

inline bsl::string& CommandLineParameters::queueFlags()
{
    return d_queueFlags;
}

inline bsl::string& CommandLineParameters::latency()
{
    return d_latency;
}

inline bsl::string& CommandLineParameters::latencyReport()
{
    return d_latencyReport;
}

inline bool& CommandLineParameters::dumpMsg()
{
    return d_dumpMsg;
}

inline bool& CommandLineParameters::confirmMsg()
{
    return d_confirmMsg;
}

inline bsls::Types::Int64& CommandLineParameters::eventSize()
{
    return d_eventSize;
}

inline int& CommandLineParameters::msgSize()
{
    return d_msgSize;
}

inline int& CommandLineParameters::postRate()
{
    return d_postRate;
}

inline bsl::string& CommandLineParameters::eventsCount()
{
    return d_eventsCount;
}

inline bsl::string& CommandLineParameters::maxUnconfirmed()
{
    return d_maxUnconfirmed;
}

inline int& CommandLineParameters::postInterval()
{
    return d_postInterval;
}

inline bsl::string& CommandLineParameters::verbosity()
{
    return d_verbosity;
}

inline bsl::string& CommandLineParameters::logFormat()
{
    return d_logFormat;
}

inline bool& CommandLineParameters::memoryDebug()
{
    return d_memoryDebug;
}

inline int& CommandLineParameters::threads()
{
    return d_threads;
}

inline int& CommandLineParameters::shutdownGrace()
{
    return d_shutdownGrace;
}

inline bool& CommandLineParameters::noSessionEventHandler()
{
    return d_noSessionEventHandler;
}

inline bsl::string& CommandLineParameters::storage()
{
    return d_storage;
}

inline bsl::string& CommandLineParameters::log()
{
    return d_log;
}

inline bsl::string& CommandLineParameters::sequentialMessagePattern()
{
    return d_sequentialMessagePattern;
}

inline bsl::vector<MessageProperty>& CommandLineParameters::messageProperties()
{
    return d_messageProperties;
}

inline bsl::vector<Subscription>& CommandLineParameters::subscriptions()
{
    return d_subscriptions;
}

// ACCESSORS
template <typename t_ACCESSOR>
int CommandLineParameters::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_mode, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_broker, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_queueUri,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_URI]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_queueFlags,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_FLAGS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_latency, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_latencyReport,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY_REPORT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_dumpMsg, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_MSG]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_confirmMsg,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIRM_MSG]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_eventSize,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENT_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_msgSize, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_SIZE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_postRate,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_RATE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_eventsCount,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENTS_COUNT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_maxUnconfirmed,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_postInterval,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_INTERVAL]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_verbosity,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERBOSITY]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_logFormat,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_FORMAT]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_memoryDebug,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MEMORY_DEBUG]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_threads, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THREADS]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_shutdownGrace,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SHUTDOWN_GRACE]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_noSessionEventHandler,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NO_SESSION_EVENT_HANDLER]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_storage, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_log, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_sequentialMessagePattern,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENTIAL_MESSAGE_PATTERN]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_messageProperties,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_subscriptions,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
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
    case ATTRIBUTE_ID_MODE: {
        return accessor(d_mode, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MODE]);
    }
    case ATTRIBUTE_ID_BROKER: {
        return accessor(d_broker,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_BROKER]);
    }
    case ATTRIBUTE_ID_QUEUE_URI: {
        return accessor(d_queueUri,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_URI]);
    }
    case ATTRIBUTE_ID_QUEUE_FLAGS: {
        return accessor(d_queueFlags,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_QUEUE_FLAGS]);
    }
    case ATTRIBUTE_ID_LATENCY: {
        return accessor(d_latency,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY]);
    }
    case ATTRIBUTE_ID_LATENCY_REPORT: {
        return accessor(d_latencyReport,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LATENCY_REPORT]);
    }
    case ATTRIBUTE_ID_DUMP_MSG: {
        return accessor(d_dumpMsg,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_DUMP_MSG]);
    }
    case ATTRIBUTE_ID_CONFIRM_MSG: {
        return accessor(d_confirmMsg,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CONFIRM_MSG]);
    }
    case ATTRIBUTE_ID_EVENT_SIZE: {
        return accessor(d_eventSize,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENT_SIZE]);
    }
    case ATTRIBUTE_ID_MSG_SIZE: {
        return accessor(d_msgSize,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MSG_SIZE]);
    }
    case ATTRIBUTE_ID_POST_RATE: {
        return accessor(d_postRate,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_RATE]);
    }
    case ATTRIBUTE_ID_EVENTS_COUNT: {
        return accessor(d_eventsCount,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_EVENTS_COUNT]);
    }
    case ATTRIBUTE_ID_MAX_UNCONFIRMED: {
        return accessor(d_maxUnconfirmed,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_UNCONFIRMED]);
    }
    case ATTRIBUTE_ID_POST_INTERVAL: {
        return accessor(d_postInterval,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_POST_INTERVAL]);
    }
    case ATTRIBUTE_ID_VERBOSITY: {
        return accessor(d_verbosity,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_VERBOSITY]);
    }
    case ATTRIBUTE_ID_LOG_FORMAT: {
        return accessor(d_logFormat,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG_FORMAT]);
    }
    case ATTRIBUTE_ID_MEMORY_DEBUG: {
        return accessor(d_memoryDebug,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MEMORY_DEBUG]);
    }
    case ATTRIBUTE_ID_THREADS: {
        return accessor(d_threads,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_THREADS]);
    }
    case ATTRIBUTE_ID_SHUTDOWN_GRACE: {
        return accessor(d_shutdownGrace,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SHUTDOWN_GRACE]);
    }
    case ATTRIBUTE_ID_NO_SESSION_EVENT_HANDLER: {
        return accessor(
            d_noSessionEventHandler,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_NO_SESSION_EVENT_HANDLER]);
    }
    case ATTRIBUTE_ID_STORAGE: {
        return accessor(d_storage,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_STORAGE]);
    }
    case ATTRIBUTE_ID_LOG: {
        return accessor(d_log, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_LOG]);
    }
    case ATTRIBUTE_ID_SEQUENTIAL_MESSAGE_PATTERN: {
        return accessor(
            d_sequentialMessagePattern,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SEQUENTIAL_MESSAGE_PATTERN]);
    }
    case ATTRIBUTE_ID_MESSAGE_PROPERTIES: {
        return accessor(
            d_messageProperties,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES]);
    }
    case ATTRIBUTE_ID_SUBSCRIPTIONS: {
        return accessor(d_subscriptions,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SUBSCRIPTIONS]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int CommandLineParameters::accessAttribute(t_ACCESSOR& accessor,
                                           const char* name,
                                           int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& CommandLineParameters::mode() const
{
    return d_mode;
}

inline const bsl::string& CommandLineParameters::broker() const
{
    return d_broker;
}

inline const bsl::string& CommandLineParameters::queueUri() const
{
    return d_queueUri;
}

inline const bsl::string& CommandLineParameters::queueFlags() const
{
    return d_queueFlags;
}

inline const bsl::string& CommandLineParameters::latency() const
{
    return d_latency;
}

inline const bsl::string& CommandLineParameters::latencyReport() const
{
    return d_latencyReport;
}

inline bool CommandLineParameters::dumpMsg() const
{
    return d_dumpMsg;
}

inline bool CommandLineParameters::confirmMsg() const
{
    return d_confirmMsg;
}

inline bsls::Types::Int64 CommandLineParameters::eventSize() const
{
    return d_eventSize;
}

inline int CommandLineParameters::msgSize() const
{
    return d_msgSize;
}

inline int CommandLineParameters::postRate() const
{
    return d_postRate;
}

inline const bsl::string& CommandLineParameters::eventsCount() const
{
    return d_eventsCount;
}

inline const bsl::string& CommandLineParameters::maxUnconfirmed() const
{
    return d_maxUnconfirmed;
}

inline int CommandLineParameters::postInterval() const
{
    return d_postInterval;
}

inline const bsl::string& CommandLineParameters::verbosity() const
{
    return d_verbosity;
}

inline const bsl::string& CommandLineParameters::logFormat() const
{
    return d_logFormat;
}

inline bool CommandLineParameters::memoryDebug() const
{
    return d_memoryDebug;
}

inline int CommandLineParameters::threads() const
{
    return d_threads;
}

inline int CommandLineParameters::shutdownGrace() const
{
    return d_shutdownGrace;
}

inline bool CommandLineParameters::noSessionEventHandler() const
{
    return d_noSessionEventHandler;
}

inline const bsl::string& CommandLineParameters::storage() const
{
    return d_storage;
}

inline const bsl::string& CommandLineParameters::log() const
{
    return d_log;
}

inline const bsl::string&
CommandLineParameters::sequentialMessagePattern() const
{
    return d_sequentialMessagePattern;
}

inline const bsl::vector<MessageProperty>&
CommandLineParameters::messageProperties() const
{
    return d_messageProperties;
}

inline const bsl::vector<Subscription>&
CommandLineParameters::subscriptions() const
{
    return d_subscriptions;
}

// --------------------
// class JournalCommand
// --------------------

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int JournalCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_choice, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int JournalCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CHOICE: {
        return manipulator(&d_choice,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int JournalCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                        const char*    name,
                                        int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline JournalCommandChoice& JournalCommand::choice()
{
    return d_choice;
}

// ACCESSORS
template <typename t_ACCESSOR>
int JournalCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_choice, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int JournalCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_CHOICE: {
        return accessor(d_choice,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_CHOICE]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int JournalCommand::accessAttribute(t_ACCESSOR& accessor,
                                    const char* name,
                                    int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const JournalCommandChoice& JournalCommand::choice() const
{
    return d_choice;
}

// -----------------
// class PostCommand
// -----------------

// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void PostCommand::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->uri());
    hashAppend(hashAlgorithm, this->payload());
    hashAppend(hashAlgorithm, this->async());
    hashAppend(hashAlgorithm, this->groupid());
    hashAppend(hashAlgorithm, this->compressionAlgorithmType());
    hashAppend(hashAlgorithm, this->messageProperties());
}

inline bool PostCommand::isEqualTo(const PostCommand& rhs) const
{
    return this->uri() == rhs.uri() && this->payload() == rhs.payload() &&
           this->async() == rhs.async() && this->groupid() == rhs.groupid() &&
           this->compressionAlgorithmType() ==
               rhs.compressionAlgorithmType() &&
           this->messageProperties() == rhs.messageProperties();
}

// CLASS METHODS
// MANIPULATORS
template <typename t_MANIPULATOR>
int PostCommand::manipulateAttributes(t_MANIPULATOR& manipulator)
{
    int ret;

    ret = manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_payload,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PAYLOAD]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    ret = manipulator(&d_groupid,
                      ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GROUPID]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_compressionAlgorithmType,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMPRESSION_ALGORITHM_TYPE]);
    if (ret) {
        return ret;
    }

    ret = manipulator(
        &d_messageProperties,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_MANIPULATOR>
int PostCommand::manipulateAttribute(t_MANIPULATOR& manipulator, int id)
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return manipulator(&d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_PAYLOAD: {
        return manipulator(&d_payload,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PAYLOAD]);
    }
    case ATTRIBUTE_ID_ASYNC: {
        return manipulator(&d_async,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    case ATTRIBUTE_ID_GROUPID: {
        return manipulator(&d_groupid,
                           ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GROUPID]);
    }
    case ATTRIBUTE_ID_COMPRESSION_ALGORITHM_TYPE: {
        return manipulator(
            &d_compressionAlgorithmType,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMPRESSION_ALGORITHM_TYPE]);
    }
    case ATTRIBUTE_ID_MESSAGE_PROPERTIES: {
        return manipulator(
            &d_messageProperties,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_MANIPULATOR>
int PostCommand::manipulateAttribute(t_MANIPULATOR& manipulator,
                                     const char*    name,
                                     int            nameLength)
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return manipulateAttribute(manipulator, attributeInfo->d_id);
}

inline bsl::string& PostCommand::uri()
{
    return d_uri;
}

inline bsl::vector<bsl::string>& PostCommand::payload()
{
    return d_payload;
}

inline bool& PostCommand::async()
{
    return d_async;
}

inline bsl::string& PostCommand::groupid()
{
    return d_groupid;
}

inline bsl::string& PostCommand::compressionAlgorithmType()
{
    return d_compressionAlgorithmType;
}

inline bsl::vector<MessageProperty>& PostCommand::messageProperties()
{
    return d_messageProperties;
}

// ACCESSORS
template <typename t_ACCESSOR>
int PostCommand::accessAttributes(t_ACCESSOR& accessor) const
{
    int ret;

    ret = accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_payload, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PAYLOAD]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_groupid, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GROUPID]);
    if (ret) {
        return ret;
    }

    ret = accessor(
        d_compressionAlgorithmType,
        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMPRESSION_ALGORITHM_TYPE]);
    if (ret) {
        return ret;
    }

    ret = accessor(d_messageProperties,
                   ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES]);
    if (ret) {
        return ret;
    }

    return 0;
}

template <typename t_ACCESSOR>
int PostCommand::accessAttribute(t_ACCESSOR& accessor, int id) const
{
    enum { NOT_FOUND = -1 };

    switch (id) {
    case ATTRIBUTE_ID_URI: {
        return accessor(d_uri, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_URI]);
    }
    case ATTRIBUTE_ID_PAYLOAD: {
        return accessor(d_payload,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_PAYLOAD]);
    }
    case ATTRIBUTE_ID_ASYNC: {
        return accessor(d_async, ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_ASYNC]);
    }
    case ATTRIBUTE_ID_GROUPID: {
        return accessor(d_groupid,
                        ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_GROUPID]);
    }
    case ATTRIBUTE_ID_COMPRESSION_ALGORITHM_TYPE: {
        return accessor(
            d_compressionAlgorithmType,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_COMPRESSION_ALGORITHM_TYPE]);
    }
    case ATTRIBUTE_ID_MESSAGE_PROPERTIES: {
        return accessor(
            d_messageProperties,
            ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MESSAGE_PROPERTIES]);
    }
    default: return NOT_FOUND;
    }
}

template <typename t_ACCESSOR>
int PostCommand::accessAttribute(t_ACCESSOR& accessor,
                                 const char* name,
                                 int         nameLength) const
{
    enum { NOT_FOUND = -1 };

    const bdlat_AttributeInfo* attributeInfo = lookupAttributeInfo(name,
                                                                   nameLength);
    if (0 == attributeInfo) {
        return NOT_FOUND;
    }

    return accessAttribute(accessor, attributeInfo->d_id);
}

inline const bsl::string& PostCommand::uri() const
{
    return d_uri;
}

inline const bsl::vector<bsl::string>& PostCommand::payload() const
{
    return d_payload;
}

inline bool PostCommand::async() const
{
    return d_async;
}

inline const bsl::string& PostCommand::groupid() const
{
    return d_groupid;
}

inline const bsl::string& PostCommand::compressionAlgorithmType() const
{
    return d_compressionAlgorithmType;
}

inline const bsl::vector<MessageProperty>&
PostCommand::messageProperties() const
{
    return d_messageProperties;
}

// -------------
// class Command
// -------------

// CLASS METHODS
// PRIVATE ACCESSORS
template <typename t_HASH_ALGORITHM>
void Command::hashAppendImpl(t_HASH_ALGORITHM& hashAlgorithm) const
{
    typedef Command Class;
    using bslh::hashAppend;
    hashAppend(hashAlgorithm, this->selectionId());
    switch (this->selectionId()) {
    case Class::SELECTION_ID_START:
        hashAppend(hashAlgorithm, this->start());
        break;
    case Class::SELECTION_ID_STOP:
        hashAppend(hashAlgorithm, this->stop());
        break;
    case Class::SELECTION_ID_OPEN_QUEUE:
        hashAppend(hashAlgorithm, this->openQueue());
        break;
    case Class::SELECTION_ID_CONFIGURE_QUEUE:
        hashAppend(hashAlgorithm, this->configureQueue());
        break;
    case Class::SELECTION_ID_CLOSE_QUEUE:
        hashAppend(hashAlgorithm, this->closeQueue());
        break;
    case Class::SELECTION_ID_POST:
        hashAppend(hashAlgorithm, this->post());
        break;
    case Class::SELECTION_ID_LIST:
        hashAppend(hashAlgorithm, this->list());
        break;
    case Class::SELECTION_ID_CONFIRM:
        hashAppend(hashAlgorithm, this->confirm());
        break;
    case Class::SELECTION_ID_BATCH_POST:
        hashAppend(hashAlgorithm, this->batchPost());
        break;
    case Class::SELECTION_ID_LOAD_POST:
        hashAppend(hashAlgorithm, this->loadPost());
        break;
    case Class::SELECTION_ID_OPEN_STORAGE:
        hashAppend(hashAlgorithm, this->openStorage());
        break;
    case Class::SELECTION_ID_CLOSE_STORAGE:
        hashAppend(hashAlgorithm, this->closeStorage());
        break;
    case Class::SELECTION_ID_METADATA:
        hashAppend(hashAlgorithm, this->metadata());
        break;
    case Class::SELECTION_ID_LIST_QUEUES:
        hashAppend(hashAlgorithm, this->listQueues());
        break;
    case Class::SELECTION_ID_DUMP_QUEUE:
        hashAppend(hashAlgorithm, this->dumpQueue());
        break;
    case Class::SELECTION_ID_DATA:
        hashAppend(hashAlgorithm, this->data());
        break;
    case Class::SELECTION_ID_QLIST:
        hashAppend(hashAlgorithm, this->qlist());
        break;
    case Class::SELECTION_ID_JOURNAL:
        hashAppend(hashAlgorithm, this->journal());
        break;
    default: BSLS_ASSERT(this->selectionId() == Class::SELECTION_ID_UNDEFINED);
    }
}

inline bool Command::isEqualTo(const Command& rhs) const
{
    typedef Command Class;
    if (this->selectionId() == rhs.selectionId()) {
        switch (rhs.selectionId()) {
        case Class::SELECTION_ID_START: return this->start() == rhs.start();
        case Class::SELECTION_ID_STOP: return this->stop() == rhs.stop();
        case Class::SELECTION_ID_OPEN_QUEUE:
            return this->openQueue() == rhs.openQueue();
        case Class::SELECTION_ID_CONFIGURE_QUEUE:
            return this->configureQueue() == rhs.configureQueue();
        case Class::SELECTION_ID_CLOSE_QUEUE:
            return this->closeQueue() == rhs.closeQueue();
        case Class::SELECTION_ID_POST: return this->post() == rhs.post();
        case Class::SELECTION_ID_LIST: return this->list() == rhs.list();
        case Class::SELECTION_ID_CONFIRM:
            return this->confirm() == rhs.confirm();
        case Class::SELECTION_ID_BATCH_POST:
            return this->batchPost() == rhs.batchPost();
        case Class::SELECTION_ID_LOAD_POST:
            return this->loadPost() == rhs.loadPost();
        case Class::SELECTION_ID_OPEN_STORAGE:
            return this->openStorage() == rhs.openStorage();
        case Class::SELECTION_ID_CLOSE_STORAGE:
            return this->closeStorage() == rhs.closeStorage();
        case Class::SELECTION_ID_METADATA:
            return this->metadata() == rhs.metadata();
        case Class::SELECTION_ID_LIST_QUEUES:
            return this->listQueues() == rhs.listQueues();
        case Class::SELECTION_ID_DUMP_QUEUE:
            return this->dumpQueue() == rhs.dumpQueue();
        case Class::SELECTION_ID_DATA: return this->data() == rhs.data();
        case Class::SELECTION_ID_QLIST: return this->qlist() == rhs.qlist();
        case Class::SELECTION_ID_JOURNAL:
            return this->journal() == rhs.journal();
        default:
            BSLS_ASSERT(Class::SELECTION_ID_UNDEFINED == rhs.selectionId());
            return true;
        }
    }
    else {
        return false;
    }
}

// CREATORS
inline Command::Command(bslma::Allocator* basicAllocator)
: d_selectionId(SELECTION_ID_UNDEFINED)
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
}

inline Command::~Command()
{
    reset();
}

// MANIPULATORS
template <typename t_MANIPULATOR>
int Command::manipulateSelection(t_MANIPULATOR& manipulator)
{
    switch (d_selectionId) {
    case Command::SELECTION_ID_START:
        return manipulator(&d_start.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_START]);
    case Command::SELECTION_ID_STOP:
        return manipulator(&d_stop.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_STOP]);
    case Command::SELECTION_ID_OPEN_QUEUE:
        return manipulator(&d_openQueue.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_QUEUE]);
    case Command::SELECTION_ID_CONFIGURE_QUEUE:
        return manipulator(
            &d_configureQueue.object(),
            SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIGURE_QUEUE]);
    case Command::SELECTION_ID_CLOSE_QUEUE:
        return manipulator(&d_closeQueue.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_QUEUE]);
    case Command::SELECTION_ID_POST:
        return manipulator(&d_post.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_POST]);
    case Command::SELECTION_ID_LIST:
        return manipulator(&d_list.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST]);
    case Command::SELECTION_ID_CONFIRM:
        return manipulator(&d_confirm.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIRM]);
    case Command::SELECTION_ID_BATCH_POST:
        return manipulator(&d_batchPost.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_BATCH_POST]);
    case Command::SELECTION_ID_LOAD_POST:
        return manipulator(&d_loadPost.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_LOAD_POST]);
    case Command::SELECTION_ID_OPEN_STORAGE:
        return manipulator(&d_openStorage.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_STORAGE]);
    case Command::SELECTION_ID_CLOSE_STORAGE:
        return manipulator(
            &d_closeStorage.object(),
            SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_STORAGE]);
    case Command::SELECTION_ID_METADATA:
        return manipulator(&d_metadata.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_METADATA]);
    case Command::SELECTION_ID_LIST_QUEUES:
        return manipulator(&d_listQueues.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST_QUEUES]);
    case Command::SELECTION_ID_DUMP_QUEUE:
        return manipulator(&d_dumpQueue.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_DUMP_QUEUE]);
    case Command::SELECTION_ID_DATA:
        return manipulator(&d_data.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_DATA]);
    case Command::SELECTION_ID_QLIST:
        return manipulator(&d_qlist.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_QLIST]);
    case Command::SELECTION_ID_JOURNAL:
        return manipulator(&d_journal.object(),
                           SELECTION_INFO_ARRAY[SELECTION_INDEX_JOURNAL]);
    default:
        BSLS_ASSERT(Command::SELECTION_ID_UNDEFINED == d_selectionId);
        return -1;
    }
}

inline StartCommand& Command::start()
{
    BSLS_ASSERT(SELECTION_ID_START == d_selectionId);
    return d_start.object();
}

inline StopCommand& Command::stop()
{
    BSLS_ASSERT(SELECTION_ID_STOP == d_selectionId);
    return d_stop.object();
}

inline OpenQueueCommand& Command::openQueue()
{
    BSLS_ASSERT(SELECTION_ID_OPEN_QUEUE == d_selectionId);
    return d_openQueue.object();
}

inline ConfigureQueueCommand& Command::configureQueue()
{
    BSLS_ASSERT(SELECTION_ID_CONFIGURE_QUEUE == d_selectionId);
    return d_configureQueue.object();
}

inline CloseQueueCommand& Command::closeQueue()
{
    BSLS_ASSERT(SELECTION_ID_CLOSE_QUEUE == d_selectionId);
    return d_closeQueue.object();
}

inline PostCommand& Command::post()
{
    BSLS_ASSERT(SELECTION_ID_POST == d_selectionId);
    return d_post.object();
}

inline ListCommand& Command::list()
{
    BSLS_ASSERT(SELECTION_ID_LIST == d_selectionId);
    return d_list.object();
}

inline ConfirmCommand& Command::confirm()
{
    BSLS_ASSERT(SELECTION_ID_CONFIRM == d_selectionId);
    return d_confirm.object();
}

inline BatchPostCommand& Command::batchPost()
{
    BSLS_ASSERT(SELECTION_ID_BATCH_POST == d_selectionId);
    return d_batchPost.object();
}

inline LoadPostCommand& Command::loadPost()
{
    BSLS_ASSERT(SELECTION_ID_LOAD_POST == d_selectionId);
    return d_loadPost.object();
}

inline OpenStorageCommand& Command::openStorage()
{
    BSLS_ASSERT(SELECTION_ID_OPEN_STORAGE == d_selectionId);
    return d_openStorage.object();
}

inline CloseStorageCommand& Command::closeStorage()
{
    BSLS_ASSERT(SELECTION_ID_CLOSE_STORAGE == d_selectionId);
    return d_closeStorage.object();
}

inline MetadataCommand& Command::metadata()
{
    BSLS_ASSERT(SELECTION_ID_METADATA == d_selectionId);
    return d_metadata.object();
}

inline ListQueuesCommand& Command::listQueues()
{
    BSLS_ASSERT(SELECTION_ID_LIST_QUEUES == d_selectionId);
    return d_listQueues.object();
}

inline DumpQueueCommand& Command::dumpQueue()
{
    BSLS_ASSERT(SELECTION_ID_DUMP_QUEUE == d_selectionId);
    return d_dumpQueue.object();
}

inline DataCommand& Command::data()
{
    BSLS_ASSERT(SELECTION_ID_DATA == d_selectionId);
    return d_data.object();
}

inline QlistCommand& Command::qlist()
{
    BSLS_ASSERT(SELECTION_ID_QLIST == d_selectionId);
    return d_qlist.object();
}

inline JournalCommand& Command::journal()
{
    BSLS_ASSERT(SELECTION_ID_JOURNAL == d_selectionId);
    return d_journal.object();
}

// ACCESSORS
inline int Command::selectionId() const
{
    return d_selectionId;
}

template <typename t_ACCESSOR>
int Command::accessSelection(t_ACCESSOR& accessor) const
{
    switch (d_selectionId) {
    case SELECTION_ID_START:
        return accessor(d_start.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_START]);
    case SELECTION_ID_STOP:
        return accessor(d_stop.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_STOP]);
    case SELECTION_ID_OPEN_QUEUE:
        return accessor(d_openQueue.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_QUEUE]);
    case SELECTION_ID_CONFIGURE_QUEUE:
        return accessor(d_configureQueue.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIGURE_QUEUE]);
    case SELECTION_ID_CLOSE_QUEUE:
        return accessor(d_closeQueue.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_QUEUE]);
    case SELECTION_ID_POST:
        return accessor(d_post.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_POST]);
    case SELECTION_ID_LIST:
        return accessor(d_list.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST]);
    case SELECTION_ID_CONFIRM:
        return accessor(d_confirm.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_CONFIRM]);
    case SELECTION_ID_BATCH_POST:
        return accessor(d_batchPost.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_BATCH_POST]);
    case SELECTION_ID_LOAD_POST:
        return accessor(d_loadPost.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_LOAD_POST]);
    case SELECTION_ID_OPEN_STORAGE:
        return accessor(d_openStorage.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_OPEN_STORAGE]);
    case SELECTION_ID_CLOSE_STORAGE:
        return accessor(d_closeStorage.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_CLOSE_STORAGE]);
    case SELECTION_ID_METADATA:
        return accessor(d_metadata.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_METADATA]);
    case SELECTION_ID_LIST_QUEUES:
        return accessor(d_listQueues.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_LIST_QUEUES]);
    case SELECTION_ID_DUMP_QUEUE:
        return accessor(d_dumpQueue.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_DUMP_QUEUE]);
    case SELECTION_ID_DATA:
        return accessor(d_data.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_DATA]);
    case SELECTION_ID_QLIST:
        return accessor(d_qlist.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_QLIST]);
    case SELECTION_ID_JOURNAL:
        return accessor(d_journal.object(),
                        SELECTION_INFO_ARRAY[SELECTION_INDEX_JOURNAL]);
    default: BSLS_ASSERT(SELECTION_ID_UNDEFINED == d_selectionId); return -1;
    }
}

inline const StartCommand& Command::start() const
{
    BSLS_ASSERT(SELECTION_ID_START == d_selectionId);
    return d_start.object();
}

inline const StopCommand& Command::stop() const
{
    BSLS_ASSERT(SELECTION_ID_STOP == d_selectionId);
    return d_stop.object();
}

inline const OpenQueueCommand& Command::openQueue() const
{
    BSLS_ASSERT(SELECTION_ID_OPEN_QUEUE == d_selectionId);
    return d_openQueue.object();
}

inline const ConfigureQueueCommand& Command::configureQueue() const
{
    BSLS_ASSERT(SELECTION_ID_CONFIGURE_QUEUE == d_selectionId);
    return d_configureQueue.object();
}

inline const CloseQueueCommand& Command::closeQueue() const
{
    BSLS_ASSERT(SELECTION_ID_CLOSE_QUEUE == d_selectionId);
    return d_closeQueue.object();
}

inline const PostCommand& Command::post() const
{
    BSLS_ASSERT(SELECTION_ID_POST == d_selectionId);
    return d_post.object();
}

inline const ListCommand& Command::list() const
{
    BSLS_ASSERT(SELECTION_ID_LIST == d_selectionId);
    return d_list.object();
}

inline const ConfirmCommand& Command::confirm() const
{
    BSLS_ASSERT(SELECTION_ID_CONFIRM == d_selectionId);
    return d_confirm.object();
}

inline const BatchPostCommand& Command::batchPost() const
{
    BSLS_ASSERT(SELECTION_ID_BATCH_POST == d_selectionId);
    return d_batchPost.object();
}

inline const LoadPostCommand& Command::loadPost() const
{
    BSLS_ASSERT(SELECTION_ID_LOAD_POST == d_selectionId);
    return d_loadPost.object();
}

inline const OpenStorageCommand& Command::openStorage() const
{
    BSLS_ASSERT(SELECTION_ID_OPEN_STORAGE == d_selectionId);
    return d_openStorage.object();
}

inline const CloseStorageCommand& Command::closeStorage() const
{
    BSLS_ASSERT(SELECTION_ID_CLOSE_STORAGE == d_selectionId);
    return d_closeStorage.object();
}

inline const MetadataCommand& Command::metadata() const
{
    BSLS_ASSERT(SELECTION_ID_METADATA == d_selectionId);
    return d_metadata.object();
}

inline const ListQueuesCommand& Command::listQueues() const
{
    BSLS_ASSERT(SELECTION_ID_LIST_QUEUES == d_selectionId);
    return d_listQueues.object();
}

inline const DumpQueueCommand& Command::dumpQueue() const
{
    BSLS_ASSERT(SELECTION_ID_DUMP_QUEUE == d_selectionId);
    return d_dumpQueue.object();
}

inline const DataCommand& Command::data() const
{
    BSLS_ASSERT(SELECTION_ID_DATA == d_selectionId);
    return d_data.object();
}

inline const QlistCommand& Command::qlist() const
{
    BSLS_ASSERT(SELECTION_ID_QLIST == d_selectionId);
    return d_qlist.object();
}

inline const JournalCommand& Command::journal() const
{
    BSLS_ASSERT(SELECTION_ID_JOURNAL == d_selectionId);
    return d_journal.object();
}

inline bool Command::isStartValue() const
{
    return SELECTION_ID_START == d_selectionId;
}

inline bool Command::isStopValue() const
{
    return SELECTION_ID_STOP == d_selectionId;
}

inline bool Command::isOpenQueueValue() const
{
    return SELECTION_ID_OPEN_QUEUE == d_selectionId;
}

inline bool Command::isConfigureQueueValue() const
{
    return SELECTION_ID_CONFIGURE_QUEUE == d_selectionId;
}

inline bool Command::isCloseQueueValue() const
{
    return SELECTION_ID_CLOSE_QUEUE == d_selectionId;
}

inline bool Command::isPostValue() const
{
    return SELECTION_ID_POST == d_selectionId;
}

inline bool Command::isListValue() const
{
    return SELECTION_ID_LIST == d_selectionId;
}

inline bool Command::isConfirmValue() const
{
    return SELECTION_ID_CONFIRM == d_selectionId;
}

inline bool Command::isBatchPostValue() const
{
    return SELECTION_ID_BATCH_POST == d_selectionId;
}

inline bool Command::isLoadPostValue() const
{
    return SELECTION_ID_LOAD_POST == d_selectionId;
}

inline bool Command::isOpenStorageValue() const
{
    return SELECTION_ID_OPEN_STORAGE == d_selectionId;
}

inline bool Command::isCloseStorageValue() const
{
    return SELECTION_ID_CLOSE_STORAGE == d_selectionId;
}

inline bool Command::isMetadataValue() const
{
    return SELECTION_ID_METADATA == d_selectionId;
}

inline bool Command::isListQueuesValue() const
{
    return SELECTION_ID_LIST_QUEUES == d_selectionId;
}

inline bool Command::isDumpQueueValue() const
{
    return SELECTION_ID_DUMP_QUEUE == d_selectionId;
}

inline bool Command::isDataValue() const
{
    return SELECTION_ID_DATA == d_selectionId;
}

inline bool Command::isQlistValue() const
{
    return SELECTION_ID_QLIST == d_selectionId;
}

inline bool Command::isJournalValue() const
{
    return SELECTION_ID_JOURNAL == d_selectionId;
}

inline bool Command::isUndefinedValue() const
{
    return SELECTION_ID_UNDEFINED == d_selectionId;
}
}  // close package namespace

// FREE FUNCTIONS

}  // close enterprise namespace
#endif

// GENERATED BY BLP_BAS_CODEGEN_2024.05.16
// USING bas_codegen.pl -m msg --noAggregateConversion --noExternalization
// --noIdent --package m_bmqtool --msgComponent messages bmqtoolcmd.xsd
