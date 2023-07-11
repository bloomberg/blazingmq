// Copyright 2016-2023 Bloomberg Finance L.P.
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

// bmqa_messageproperties.cpp                                         -*-C++-*-
#include <bmqa_messageproperties.h>

#include <bmqscm_version.h>
// BMQ
#include <bmqp_messageproperties.h>
#include <bmqp_protocol.h>

// BSL
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bslma_default.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqa {

namespace {
// Compile-time assertions to ensure that some constants declared in this
// component always remain in sync with their counterparts in the bmqp package.
// Note that this component needs to re-declare these constants because bmqp
// headers cannot be included in application-facing headers.

BSLMF_ASSERT(MessageProperties::k_MAX_NUM_PROPERTIES ==
             bmqp::MessageProperties::k_MAX_NUM_PROPERTIES);

BSLMF_ASSERT(MessageProperties::k_MAX_PROPERTIES_AREA_LENGTH ==
             bmqp::MessageProperties::k_MAX_PROPERTIES_AREA_LENGTH);

BSLMF_ASSERT(MessageProperties::k_MAX_PROPERTY_NAME_LENGTH ==
             bmqp::MessageProperties::k_MAX_PROPERTY_NAME_LENGTH);

BSLMF_ASSERT(MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH ==
             bmqp::MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH);

BSLMF_ASSERT(MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH ==
             bmqp::MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH);
}  // close unnamed namespace

// -----------------------
// class MessageProperties
// -----------------------

// CREATORS
MessageProperties::MessageProperties(bslma::Allocator* basicAllocator)
: d_impl_p(0)
, d_buffer()
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    BSLMF_ASSERT(k_MAX_SIZEOF_BMQP_MESSAGEPROPERTIES >=
                 sizeof(bmqp::MessageProperties));
    // Compile-time assert to keep the hardcoded value of size of an object
    // of type 'bmqp::MessageProperties' in sync with its actual size.  We
    // need to hard code the size in 'bmqa_messageproperties.h' because
    // none of the 'bmqp' headers can be included in 'bmqa' headers.  Note
    // that we don't check exact size, but 'enough' size, because exact
    // size differs on 32bit vs 64bit platforms.

    new (d_buffer.buffer()) bmqp::MessageProperties(d_allocator_p);
    d_impl_p = reinterpret_cast<bmqp::MessageProperties*>(d_buffer.buffer());
}

MessageProperties::MessageProperties(const MessageProperties& other,
                                     bslma::Allocator*        basicAllocator)
: d_impl_p(0)
, d_buffer()
, d_allocator_p(bslma::Default::allocator(basicAllocator))
{
    // PRECONDITIONS
    BSLS_ASSERT(other.d_impl_p);

    new (d_buffer.buffer())
        bmqp::MessageProperties(*other.d_impl_p, d_allocator_p);
    d_impl_p = reinterpret_cast<bmqp::MessageProperties*>(d_buffer.buffer());
}

MessageProperties::~MessageProperties()
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    d_impl_p->bmqp::MessageProperties::~MessageProperties();
    d_impl_p = 0;
}

// MANIPULATORS
MessageProperties& MessageProperties::operator=(const MessageProperties& rhs)
{
    if (this == &rhs) {
        return *this;  // RETURN
    }

    BSLS_ASSERT(d_impl_p);
    d_impl_p->bmqp::MessageProperties::~MessageProperties();

    BSLS_ASSERT(rhs.d_impl_p);
    new (d_buffer.buffer())
        bmqp::MessageProperties(*rhs.d_impl_p, d_allocator_p);
    d_impl_p = reinterpret_cast<bmqp::MessageProperties*>(d_buffer.buffer());

    return *this;
}

void MessageProperties::clear()
{
    d_impl_p->clear();
}

bool MessageProperties::remove(const bsl::string&        name,
                               bmqt::PropertyType::Enum* buffer)
{
    return d_impl_p->remove(name, buffer);
}

int MessageProperties::setPropertyAsBool(const bsl::string& name, bool value)
{
    return d_impl_p->setPropertyAsBool(name, value);
}

int MessageProperties::setPropertyAsChar(const bsl::string& name, char value)
{
    return d_impl_p->setPropertyAsChar(name, value);
}

int MessageProperties::setPropertyAsShort(const bsl::string& name, short value)
{
    return d_impl_p->setPropertyAsShort(name, value);
}

int MessageProperties::setPropertyAsInt32(const bsl::string& name,
                                          bsl::int32_t       value)
{
    return d_impl_p->setPropertyAsInt32(name, value);
}

int MessageProperties::setPropertyAsInt64(const bsl::string& name,
                                          bsls::Types::Int64 value)
{
    return d_impl_p->setPropertyAsInt64(name, value);
}

int MessageProperties::setPropertyAsString(const bsl::string& name,
                                           const bsl::string& value)
{
    return d_impl_p->setPropertyAsString(name, value);
}

int MessageProperties::setPropertyAsBinary(const bsl::string&       name,
                                           const bsl::vector<char>& value)
{
    return d_impl_p->setPropertyAsBinary(name, value);
}

int MessageProperties::streamIn(const bdlbb::Blob& blob)
{
    return d_impl_p->streamIn(blob, false);
    // For backward-compatibility, use the old format.
}

// ACCESSORS
bool MessageProperties::getPropertyAsBool(const bsl::string& name) const
{
    return d_impl_p->getPropertyAsBool(name);
}

char MessageProperties::getPropertyAsChar(const bsl::string& name) const
{
    return d_impl_p->getPropertyAsChar(name);
}

short MessageProperties::getPropertyAsShort(const bsl::string& name) const
{
    return d_impl_p->getPropertyAsShort(name);
}

bsl::int32_t
MessageProperties::getPropertyAsInt32(const bsl::string& name) const
{
    return d_impl_p->getPropertyAsInt32(name);
}

bsls::Types::Int64
MessageProperties::getPropertyAsInt64(const bsl::string& name) const
{
    return d_impl_p->getPropertyAsInt64(name);
}

const bsl::string&
MessageProperties::getPropertyAsString(const bsl::string& name) const
{
    return d_impl_p->getPropertyAsString(name);
}

const bsl::vector<char>&
MessageProperties::getPropertyAsBinary(const bsl::string& name) const
{
    return d_impl_p->getPropertyAsBinary(name);
}

bool MessageProperties::getPropertyAsBoolOr(const bsl::string& name,
                                            bool               value) const
{
    return d_impl_p->getPropertyAsBoolOr(name, value);
}

char MessageProperties::getPropertyAsCharOr(const bsl::string& name,
                                            char               value) const
{
    return d_impl_p->getPropertyAsCharOr(name, value);
}

short MessageProperties::getPropertyAsShortOr(const bsl::string& name,
                                              short              value) const
{
    return d_impl_p->getPropertyAsShortOr(name, value);
}

bsl::int32_t MessageProperties::getPropertyAsInt32Or(const bsl::string& name,
                                                     bsl::int32_t value) const
{
    return d_impl_p->getPropertyAsInt32Or(name, value);
}

bsls::Types::Int64
MessageProperties::getPropertyAsInt64Or(const bsl::string& name,
                                        bsls::Types::Int64 value) const
{
    return d_impl_p->getPropertyAsInt64Or(name, value);
}

const bsl::string&
MessageProperties::getPropertyAsStringOr(const bsl::string& name,
                                         const bsl::string& value) const
{
    return d_impl_p->getPropertyAsStringOr(name, value);
}

const bsl::vector<char>&
MessageProperties::getPropertyAsBinaryOr(const bsl::string&       name,
                                         const bsl::vector<char>& value) const
{
    return d_impl_p->getPropertyAsBinaryOr(name, value);
}

int MessageProperties::totalSize() const
{
    return d_impl_p->totalSize();
}

int MessageProperties::numProperties() const
{
    return d_impl_p->numProperties();
}

bool MessageProperties::hasProperty(const bsl::string&        name,
                                    bmqt::PropertyType::Enum* type) const
{
    return d_impl_p->hasProperty(name, type);
}

bmqt::PropertyType::Enum
MessageProperties::propertyType(const bsl::string& name) const
{
    return d_impl_p->propertyType(name);
}

const bdlbb::Blob&
MessageProperties::streamOut(bdlbb::BlobBufferFactory* bufferFactory) const
{
    // PRECONDITIONS
    BSLS_ASSERT(bufferFactory && "Buffer factory must be specified.");

    return d_impl_p->streamOut(bufferFactory,
                               bmqp::MessagePropertiesInfo::makeNoSchema());
    // For backward-compatibility, use the old format.
}

bsl::ostream& MessageProperties::print(bsl::ostream& stream,
                                       int           level,
                                       int           spacesPerLevel) const
{
    return d_impl_p->print(stream, level, spacesPerLevel);
}

// -------------------------------
// class MessagePropertiesIterator
// -------------------------------

// CREATORS
MessagePropertiesIterator::MessagePropertiesIterator()
: d_impl_p(0)
, d_buffer()
{
    new (d_buffer.buffer()) bmqp::MessagePropertiesIterator();
    d_impl_p = reinterpret_cast<bmqp::MessagePropertiesIterator*>(
        d_buffer.buffer());
}

MessagePropertiesIterator::MessagePropertiesIterator(
    const MessageProperties* properties)
: d_impl_p(0)
, d_buffer()
{
    // PRECONDITIONS
    BSLS_ASSERT(properties);

    const bmqp::MessageProperties* propertiesImpl = properties->d_impl_p;
    new (d_buffer.buffer()) bmqp::MessagePropertiesIterator(propertiesImpl);

    d_impl_p = reinterpret_cast<bmqp::MessagePropertiesIterator*>(
        d_buffer.buffer());
}

MessagePropertiesIterator::MessagePropertiesIterator(
    const MessagePropertiesIterator& other)
{
    // PRECONDITIONS
    BSLS_ASSERT(other.d_impl_p);

    new (d_buffer.buffer()) bmqp::MessagePropertiesIterator(*other.d_impl_p);
    d_impl_p = reinterpret_cast<bmqp::MessagePropertiesIterator*>(
        d_buffer.buffer());
}

MessagePropertiesIterator::~MessagePropertiesIterator()
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    d_impl_p->bmqp::MessagePropertiesIterator::~MessagePropertiesIterator();
    d_impl_p = 0;
}

bool MessagePropertiesIterator::hasNext()
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    return d_impl_p->hasNext();
}

// ACCESSORS
const bsl::string& MessagePropertiesIterator::name() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    return d_impl_p->name();
}

bmqt::PropertyType::Enum MessagePropertiesIterator::type() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    return d_impl_p->type();
}

bool MessagePropertiesIterator::getAsBool() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    return d_impl_p->getAsBool();
}

char MessagePropertiesIterator::getAsChar() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    return d_impl_p->getAsChar();
}

short MessagePropertiesIterator::getAsShort() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    return d_impl_p->getAsShort();
}

bsl::int32_t MessagePropertiesIterator::getAsInt32() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    return d_impl_p->getAsInt32();
}

bsls::Types::Int64 MessagePropertiesIterator::getAsInt64() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    return d_impl_p->getAsInt64();
}

const bsl::string& MessagePropertiesIterator::getAsString() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    return d_impl_p->getAsString();
}

const bsl::vector<char>& MessagePropertiesIterator::getAsBinary() const
{
    // PRECONDITIONS
    BSLS_ASSERT(d_impl_p);

    return d_impl_p->getAsBinary();
}

}  // close package namespace
}  // close enterprise namespace
