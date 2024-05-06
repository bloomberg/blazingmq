// Copyright 2023 Bloomberg Finance L.P.
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

// bmqp_messageproperties.t.cpp                                       -*-C++-*-
#include <bmqp_messageproperties.h>

// BMQ
#include <bmqp_protocol.h>
#include <bmqp_protocolutil.h>
#include <bmqt_resultcode.h>
#include <mwcu_memoutstream.h>

// MWC
#include <mwcu_blob.h>
#include <mwcu_memoutstream.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlb_variant.h>
#include <bdlbb_blobutil.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bsl_ios.h>
#include <bsl_limits.h>
#include <bsl_map.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslmf_assert.h>
#include <bsls_types.h>

// TEST DRIVER
#include <mwctst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

typedef bdlb::Variant7<bool,
                       char,
                       short,
                       int,
                       bsls::Types::Int64,
                       bsl::string,
                       bsl::vector<char> >
    PropertyVariant;

typedef bsl::pair<bmqt::PropertyType::Enum, size_t> PropertyTypeSizePair;

typedef bsl::pair<PropertyTypeSizePair, PropertyVariant>
    PropertyTypeSizeVariantPair;

typedef bsl::map<bsl::string, PropertyTypeSizeVariantPair> PropertyMap;

typedef PropertyMap::iterator PropertyMapIter;

typedef PropertyMap::const_iterator PropertyMapConstIter;

typedef bsl::pair<PropertyMapIter, bool> PropertyMapInsertRc;

// ===================================
// class PropertyValueStreamOutVisitor
// ===================================

/// This class provides operator() which enables it to be specified as a
/// visitor for the variant holding property value, and is used to stream
/// out the property value to the associated blob.
class PropertyValueStreamOutVisitor {
  private:
    // DATA
    bdlbb::Blob* d_blob_p;  // Blob to stream out the property value held in
                            // the variant.

  public:
    // CREATORS
    PropertyValueStreamOutVisitor(bdlbb::Blob* blob)
    : d_blob_p(blob)
    {
        BSLS_ASSERT_SAFE(d_blob_p);
    }

    // MANIPULATORS
    template <class TYPE>
    void operator()(BSLS_ANNOTATION_UNUSED const TYPE& value)
    {
        // This method is partially specialized for each possible type, so its
        // safe to assert here.

        BSLS_ASSERT_OPT(0 && "Unexpected type in the property variant.");
    }

    void operator()(bool value)
    {
        char val = value ? 1 : 0;
        bdlbb::BlobUtil::append(d_blob_p, &val, 1);
    }

    void operator()(char value)
    {
        bdlbb::BlobUtil::append(d_blob_p, &value, 1);
    }

    void operator()(short value)
    {
        bdlb::BigEndianInt16 nboValue = bdlb::BigEndianInt16::make(value);
        bdlbb::BlobUtil::append(d_blob_p,
                                reinterpret_cast<char*>(&nboValue),
                                sizeof(nboValue));
    }

    void operator()(int value)
    {
        bdlb::BigEndianInt32 nboValue = bdlb::BigEndianInt32::make(value);
        bdlbb::BlobUtil::append(d_blob_p,
                                reinterpret_cast<char*>(&nboValue),
                                sizeof(nboValue));
    }

    void operator()(bsls::Types::Int64 value)
    {
        bdlb::BigEndianInt64 nboValue = bdlb::BigEndianInt64::make(value);
        bdlbb::BlobUtil::append(d_blob_p,
                                reinterpret_cast<char*>(&nboValue),
                                sizeof(nboValue));
    }

    void operator()(const bsl::string& value)
    {
        bdlbb::BlobUtil::append(d_blob_p,
                                value.c_str(),
                                static_cast<int>(value.length()));
    }

    void operator()(const bsl::vector<char>& value)
    {
        bdlbb::BlobUtil::append(d_blob_p,
                                value.data(),
                                static_cast<int>(value.size()));
    }
};

void populateProperties(bmqp::MessageProperties* properties,
                        PropertyMap*             propertyMap,
                        size_t                   numProps)
{
    bmqp::MessageProperties& p            = *properties;   // convenience
    PropertyMap&             pmap         = *propertyMap;  // convenience
    size_t                   numPropTypes = 7;

    for (size_t i = 1; i < (numProps + 1); ++i) {
        size_t             remainder = i % numPropTypes;
        mwcu::MemOutStream osstr(s_allocator_p);

        switch (remainder) {
        case 0: {
            osstr << "boolPropName" << i << bsl::ends;
            bool value = false;
            if (0 == i % 2) {
                value = true;
            }

            ASSERT_EQ_D(i, 0, p.setPropertyAsBool(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_BOOL, 1),  // size
                    PropertyVariant(value))));

            ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 1: {
            osstr << "charPropName" << i << bsl::ends;
            char value = static_cast<char>(bsl::numeric_limits<char>::max() /
                                           i);

            ASSERT_EQ_D(i, 0, p.setPropertyAsChar(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_CHAR, sizeof(value)),
                    PropertyVariant(value))));

            ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 2: {
            osstr << "shortPropName" << i << bsl::ends;
            short value = static_cast<short>(
                bsl::numeric_limits<short>::max() / i);

            ASSERT_EQ_D(i, 0, p.setPropertyAsShort(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_SHORT, sizeof(value)),
                    PropertyVariant(value))));

            ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 3: {
            osstr << "intPropName" << i << bsl::ends;
            int value = static_cast<int>(bsl::numeric_limits<int>::max() / i);

            ASSERT_EQ_D(i, 0, p.setPropertyAsInt32(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_INT32, sizeof(value)),
                    PropertyVariant(value))));

            ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 4: {
            osstr << "int64PropName" << i << bsl::ends;
            bsls::Types::Int64 value =
                bsl::numeric_limits<bsls::Types::Int64>::max() / i;

            ASSERT_EQ_D(i, 0, p.setPropertyAsInt64(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_INT64, sizeof(value)),
                    PropertyVariant(value))));

            ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 5: {
            osstr << "stringPropName" << i << bsl::ends;
            const bsl::string value(i, 'x', s_allocator_p);

            ASSERT_EQ_D(i, 0, p.setPropertyAsString(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_STRING, value.size()),
                    PropertyVariant(value))));

            ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 6: {
            osstr << "binaryPropName" << i << bsl::ends;
            const bsl::vector<char> value(i, 'x', s_allocator_p);

            ASSERT_EQ_D(i, 0, p.setPropertyAsBinary(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_BINARY, value.size()),
                    PropertyVariant(value))));

            ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        default: BSLS_ASSERT_OPT(false && "Unreachable by design.");
        }
    }
}

void verify(PropertyMap*                   propertyMap,
            const bmqp::MessageProperties& properties)
{
    PropertyMap&                    pmap = *propertyMap;  // convenience
    bmqp::MessagePropertiesIterator iter(&properties);
    size_t                          iteration = 0;

    while (iter.hasNext()) {
        const bsl::string& propName = iter.name();
        PropertyMapIter    pmapIt   = pmap.find(iter.name());

        ASSERT_EQ_D(iteration, false, pmap.end() == pmapIt);

        PropertyTypeSizeVariantPair& tsvPair = pmapIt->second;

        ASSERT_EQ_D(iteration, pmapIt->first, propName);
        ASSERT_EQ_D(iteration, iter.type(), tsvPair.first.first);
        ASSERT_EQ_D(iteration, false, 0 == tsvPair.first.second);

        const size_t propSize = tsvPair.first.second;

        // Set the size to be '0' to record that property has been seen.
        tsvPair.first.second = 0;

        switch (tsvPair.first.first) {
        case bmqt::PropertyType::e_BOOL: {
            ASSERT_EQ_D(iteration,
                        1u,  // bool uses hardcoded size of 1
                        propSize);

            ASSERT_EQ_D(iteration,
                        iter.getAsBool(),
                        tsvPair.second.the<bool>());
        } break;  // BREAK

        case bmqt::PropertyType::e_CHAR: {
            ASSERT_EQ_D(iteration, sizeof(char), propSize);

            ASSERT_EQ_D(iteration,
                        iter.getAsChar(),
                        tsvPair.second.the<char>());
        } break;  // BREAK

        case bmqt::PropertyType::e_SHORT: {
            ASSERT_EQ_D(iteration, sizeof(short), propSize);

            ASSERT_EQ_D(iteration,
                        iter.getAsShort(),
                        tsvPair.second.the<short>());
        } break;  // BREAK

        case bmqt::PropertyType::e_INT32: {
            ASSERT_EQ_D(iteration, sizeof(int), propSize);

            ASSERT_EQ_D(iteration,
                        iter.getAsInt32(),
                        tsvPair.second.the<int>());
        } break;  // BREAK

        case bmqt::PropertyType::e_INT64: {
            ASSERT_EQ_D(iteration, sizeof(bsls::Types::Int64), propSize);

            ASSERT_EQ_D(iteration,
                        iter.getAsInt64(),
                        tsvPair.second.the<bsls::Types::Int64>());
        } break;  // BREAK

        case bmqt::PropertyType::e_STRING: {
            const bsl::string& value = iter.getAsString();

            ASSERT_EQ_D(iteration, value.size(), propSize);

            ASSERT_EQ_D(iteration, value, tsvPair.second.the<bsl::string>());
        } break;  // BREAK

        case bmqt::PropertyType::e_BINARY: {
            const bsl::vector<char>& value = iter.getAsBinary();

            ASSERT_EQ_D(iteration, value.size(), propSize);

            ASSERT_EQ_D(iteration,
                        value,
                        tsvPair.second.the<bsl::vector<char> >());
        } break;  // BREAK

        case bmqt::PropertyType::e_UNDEFINED:
        default: BSLS_ASSERT_OPT(false && "Unreachable by design");
        }

        ++iteration;
    }

    ASSERT_EQ(static_cast<size_t>(iteration), pmap.size());
    ASSERT_EQ(static_cast<size_t>(properties.numProperties()), pmap.size());
}

void encode(bdlbb::Blob* blob, const PropertyMap& pmap)
{
    // Iterate over 'pmap' twice.  In the first pass, append
    // 'bmqp::MessagePropertyHeader' for each property.  In the second pass,
    // append property (name, value) pairs.  At the end, append padding.
    // This encodes w/o using Builder and in the old style.

    bdlbb::Blob& b = *blob;  // convenience
    b.removeAll();

    if (0 == pmap.size()) {
        return;  // RETURN
    }

    int totalSize = 0;

    // Add 'bmqp::MessagePropertiesHeader'.
    mwcu::BlobUtil::reserve(blob, sizeof(bmqp::MessagePropertiesHeader));

    new (b.buffer(0).data()) bmqp::MessagePropertiesHeader;
    // Above is ok because in this test driver, we control the buffer size
    // of blob buffer factory, which, we know, is greater than the size of
    // `bmqp::MessagePropertiesHeader`.
    bmqp::MessagePropertiesHeader* mpsh =
        reinterpret_cast<bmqp::MessagePropertiesHeader*>(b.buffer(0).data());

    mpsh->setHeaderSize(sizeof(bmqp::MessagePropertiesHeader));
    mpsh->setMessagePropertyHeaderSize(sizeof(bmqp::MessagePropertyHeader));
    mpsh->setNumProperties(static_cast<int>(pmap.size()));

    totalSize += sizeof(bmqp::MessagePropertiesHeader);

    // First pass.
    for (PropertyMapConstIter cit = pmap.begin(); cit != pmap.end(); ++cit) {
        const PropertyTypeSizeVariantPair& tsvPair = cit->second;
        bmqp::MessagePropertyHeader        mph;
        mph.setPropertyType(tsvPair.first.first);
        mph.setPropertyNameLength(static_cast<int>(cit->first.length()));
        mph.setPropertyValueLength(tsvPair.first.second);

        bdlbb::BlobUtil::append(blob,
                                reinterpret_cast<const char*>(&mph),
                                sizeof(mph));
        totalSize += sizeof(bmqp::MessagePropertyHeader);
    }

    // Second pass.
    for (PropertyMapConstIter cit = pmap.begin(); cit != pmap.end(); ++cit) {
        const PropertyTypeSizeVariantPair& tsvPair = cit->second;
        bdlbb::BlobUtil::append(blob, cit->first.c_str(), cit->first.length());
        totalSize += static_cast<int>(cit->first.length());

        PropertyValueStreamOutVisitor visitor(blob);
        tsvPair.second.apply(visitor);
        totalSize += static_cast<int>(tsvPair.first.second);
    }

    ASSERT_EQ(totalSize, b.length());
    ASSERT_EQ(true,
              totalSize <=
                  bmqp::MessageProperties::k_MAX_PROPERTIES_AREA_LENGTH);

    // Append padding.

    int padding  = 0;
    int numWords = bmqp::ProtocolUtil::calcNumWordsAndPadding(&padding,
                                                              totalSize);
    bmqp::ProtocolUtil::appendPaddingRaw(blob, padding);
    mpsh->setMessagePropertiesAreaWords(numWords);
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    mwctst::TestHelper::printTestName("BREATHING TEST");
    PV("Testing MessageProperties");

    bdlbb::PooledBlobBufferFactory bufferFactory(128, s_allocator_p);
    bmqp::MessageProperties        p(s_allocator_p);
    int                            totalLen = 0;
    bmqp::MessagePropertiesInfo    logic =
        bmqp::MessagePropertiesInfo::makeInvalidSchema();
    // Empty instance.
    ASSERT_EQ(0, p.numProperties());
    ASSERT_EQ(0, p.totalSize());
    ASSERT_EQ(false, p.remove("foobar"));

    bmqt::PropertyType::Enum ptype = bmqt::PropertyType::e_UNDEFINED;
    ASSERT_EQ(false, p.remove("none", &ptype));
    ASSERT_EQ(ptype, bmqt::PropertyType::e_UNDEFINED);

    ASSERT_EQ(false, p.hasProperty("foobar"));

    ASSERT_EQ(0, p.streamOut(&bufferFactory, logic).length());

    // Clear out the instance and repeat above.
    p.clear();

    ASSERT_EQ(0, p.numProperties());
    ASSERT_EQ(false, p.remove("foobar"));
    ASSERT_EQ(false, p.remove("none", &ptype));
    ASSERT_EQ(ptype, bmqt::PropertyType::e_UNDEFINED);
    ASSERT_EQ(false, p.hasProperty("foobar"));
    ASSERT_EQ(0, p.streamOut(&bufferFactory, logic).length());

    // Add a property.
    ASSERT_EQ(0, p.setPropertyAsChar("category", 'Q'));

    // Update 'totalLen'.  Since this is first property, add size of overall
    // properties header as well.
    totalLen += sizeof(bmqp::MessagePropertiesHeader) +
                sizeof(bmqp::MessagePropertyHeader) + bsl::strlen("category") +
                +1;

    ASSERT_EQ(p.numProperties(), 1);
    ASSERT_EQ(p.hasProperty("category"), true);
    ASSERT_EQ(p.propertyType("category"), bmqt::PropertyType::e_CHAR);
    ASSERT_EQ(p.getPropertyAsChar("category"), 'Q');
    ASSERT_EQ(p.totalSize(), totalLen);
    ASSERT_GT(p.streamOut(&bufferFactory, logic).length(), totalLen);

    ASSERT_EQ(p.remove("foobar"), false);
    ASSERT_EQ(p.hasProperty("foobar"), false);

    // Add another property.

    ASSERT_EQ(p.setPropertyAsInt64("timestamp", 123456789LL), 0);
    totalLen += sizeof(bmqp::MessagePropertyHeader) +
                bsl::strlen("timestamp") + sizeof(bsls::Types::Int64);

    ASSERT_EQ(p.numProperties(), 2);
    ASSERT_EQ(p.hasProperty("timestamp"), true);
    ASSERT_EQ(p.propertyType("timestamp"), bmqt::PropertyType::e_INT64);
    ASSERT_EQ(p.getPropertyAsInt64("timestamp"), 123456789LL);
    ASSERT_EQ(p.totalSize(), totalLen);
    ASSERT_GT(p.streamOut(&bufferFactory, logic).length(), totalLen);

    // Delete a property.

    ASSERT_EQ(true, p.remove("timestamp", &ptype));
    ASSERT_EQ(bmqt::PropertyType::e_INT64, ptype);

    totalLen -= static_cast<int>(sizeof(bmqp::MessagePropertyHeader) +
                                 bsl::strlen("timestamp") +
                                 sizeof(bsls::Types::Int64));

    ASSERT_EQ(p.numProperties(), 1);
    ASSERT_EQ(p.hasProperty("category"), true);
    ASSERT_EQ(p.propertyType("category"), bmqt::PropertyType::e_CHAR);
    ASSERT_EQ(p.getPropertyAsChar("category"), 'Q');
    ASSERT_EQ(p.totalSize(), totalLen);
    ASSERT_GT(p.streamOut(&bufferFactory, logic).length(), totalLen);

    // Test 'Or' flavor.
    const bsl::string       dummyName("blahblah", s_allocator_p);
    const bsl::string       defaultVal("defval", s_allocator_p);
    const bsl::vector<char> defaultBin(1024, 'F', s_allocator_p);

    ASSERT_EQ(p.getPropertyAsStringOr(dummyName, defaultVal), defaultVal);
    ASSERT_EQ(p.getPropertyAsInt32Or(dummyName, 42), 42);
    ASSERT_EQ(p.getPropertyAsBoolOr(dummyName, true), true);
    ASSERT_EQ(p.getPropertyAsCharOr(dummyName, 'Z'), 'Z');
    ASSERT_EQ(p.getPropertyAsShortOr(dummyName, 11), 11);
    ASSERT_EQ(p.getPropertyAsInt64Or(dummyName, 987LL), 987LL);
    ASSERT_EQ(p.getPropertyAsBinaryOr(dummyName, defaultBin), defaultBin);

    PV("Testing empty MessagePropertiesIterator");

    bmqp::MessagePropertiesIterator objIt;

    ASSERT_SAFE_FAIL(objIt.hasNext());
    ASSERT_SAFE_FAIL(objIt.name());
    ASSERT_SAFE_FAIL(objIt.type());
    ASSERT_SAFE_FAIL(objIt.getAsBool());
    ASSERT_SAFE_FAIL(objIt.getAsChar());
    ASSERT_SAFE_FAIL(objIt.getAsShort());
    ASSERT_SAFE_FAIL(objIt.getAsInt32());
    ASSERT_SAFE_FAIL(objIt.getAsInt64());
    ASSERT_SAFE_FAIL(objIt.getAsString());
    ASSERT_SAFE_FAIL(objIt.getAsBinary());
}

static void test2_setPropertyTest()
{
    mwctst::TestHelper::printTestName("'setPropertyAs*' TEST");

    {
        // Test all flavors of 'setPropertyAs*'.
        bmqp::MessageProperties obj(s_allocator_p);

        const bsl::string boolN("boolPropName", s_allocator_p);
        const bsl::string charN("charPropName", s_allocator_p);
        const bsl::string shortN("shortPropName", s_allocator_p);
        const bsl::string intN("intPropName", s_allocator_p);
        const bsl::string int64N("int64PropName", s_allocator_p);
        const bsl::string stringN("stringPropName", s_allocator_p);
        const bsl::string binaryN("binaryPropName", s_allocator_p);

        bool               boolV  = true;
        char               charV  = bsl::numeric_limits<char>::max();
        short              shortV = bsl::numeric_limits<short>::max();
        int                intV   = bsl::numeric_limits<int>::max();
        bsls::Types::Int64 int64V =
            bsl::numeric_limits<bsls::Types::Int64>::max();

        bsl::string       stringV(42, 'x', s_allocator_p);
        bsl::vector<char> binaryV(84, 250, s_allocator_p);

        ASSERT_NE(0, obj.setPropertyAsBool("", boolV));
        ASSERT_NE(0, obj.setPropertyAsChar("", charV));
        ASSERT_NE(0, obj.setPropertyAsShort("", shortV));
        ASSERT_NE(0, obj.setPropertyAsInt32("", intV));
        ASSERT_NE(0, obj.setPropertyAsInt64("", int64V));
        ASSERT_NE(0, obj.setPropertyAsString("", stringV));
        ASSERT_NE(0, obj.setPropertyAsBinary("", binaryV));

        ASSERT_EQ(0, obj.setPropertyAsBool(boolN, boolV));
        ASSERT_EQ(0, obj.setPropertyAsChar(charN, charV));
        ASSERT_EQ(0, obj.setPropertyAsShort(shortN, shortV));
        ASSERT_EQ(0, obj.setPropertyAsInt32(intN, intV));
        ASSERT_EQ(0, obj.setPropertyAsInt64(int64N, int64V));
        ASSERT_EQ(0, obj.setPropertyAsString(stringN, stringV));
        ASSERT_EQ(0, obj.setPropertyAsBinary(binaryN, binaryV));

        ASSERT_EQ(7, obj.numProperties());

        ASSERT_EQ(true, obj.hasProperty(boolN));
        ASSERT_EQ(true, obj.hasProperty(charN));
        ASSERT_EQ(true, obj.hasProperty(shortN));
        ASSERT_EQ(true, obj.hasProperty(intN));
        ASSERT_EQ(true, obj.hasProperty(int64N));
        ASSERT_EQ(true, obj.hasProperty(stringN));
        ASSERT_EQ(true, obj.hasProperty(binaryN));

        ASSERT_EQ(bmqt::PropertyType::e_BOOL, obj.propertyType(boolN));
        ASSERT_EQ(bmqt::PropertyType::e_CHAR, obj.propertyType(charN));
        ASSERT_EQ(bmqt::PropertyType::e_SHORT, obj.propertyType(shortN));
        ASSERT_EQ(bmqt::PropertyType::e_INT32, obj.propertyType(intN));
        ASSERT_EQ(bmqt::PropertyType::e_INT64, obj.propertyType(int64N));
        ASSERT_EQ(bmqt::PropertyType::e_STRING, obj.propertyType(stringN));
        ASSERT_EQ(bmqt::PropertyType::e_BINARY, obj.propertyType(binaryN));

        bmqt::PropertyType::Enum ptype = bmqt::PropertyType::e_UNDEFINED;

        ASSERT_EQ(obj.hasProperty(boolN, &ptype), true);
        ASSERT_EQ(bmqt::PropertyType::e_BOOL, ptype);

        ASSERT_EQ(obj.hasProperty(charN, &ptype), true);
        ASSERT_EQ(bmqt::PropertyType::e_CHAR, ptype);

        ASSERT_EQ(obj.hasProperty(shortN, &ptype), true);
        ASSERT_EQ(bmqt::PropertyType::e_SHORT, ptype);

        ASSERT_EQ(obj.hasProperty(intN, &ptype), true);
        ASSERT_EQ(bmqt::PropertyType::e_INT32, ptype);

        ASSERT_EQ(obj.hasProperty(int64N, &ptype), true);
        ASSERT_EQ(bmqt::PropertyType::e_INT64, ptype);

        ASSERT_EQ(obj.hasProperty(stringN, &ptype), true);
        ASSERT_EQ(bmqt::PropertyType::e_STRING, ptype);

        ASSERT_EQ(obj.hasProperty(binaryN, &ptype), true);
        ASSERT_EQ(bmqt::PropertyType::e_BINARY, ptype);

        ASSERT_EQ(boolV, obj.getPropertyAsBool(boolN));
        ASSERT_EQ(charV, obj.getPropertyAsChar(charN));
        ASSERT_EQ(shortV, obj.getPropertyAsShort(shortN));
        ASSERT_EQ(intV, obj.getPropertyAsInt32(intN));
        ASSERT_EQ(int64V, obj.getPropertyAsInt64(int64N));
        ASSERT_EQ(stringV, obj.getPropertyAsString(stringN));
        ASSERT_EQ(binaryV, obj.getPropertyAsBinary(binaryN));

        // Update the values of the existed properties.
        // --------------------------------------------
        boolV   = false;
        charV   = 'q';
        shortV  = 17;
        intV    = 987;
        int64V  = 123456LL;
        stringV = bsl::string(42, 'z', s_allocator_p);
        binaryV = bsl::vector<char>(29, 170, s_allocator_p);

        ASSERT_EQ(0, obj.setPropertyAsBool(boolN, boolV));
        ASSERT_EQ(0, obj.setPropertyAsChar(charN, charV));
        ASSERT_EQ(0, obj.setPropertyAsShort(shortN, shortV));
        ASSERT_EQ(0, obj.setPropertyAsInt32(intN, intV));
        ASSERT_EQ(0, obj.setPropertyAsInt64(int64N, int64V));
        ASSERT_EQ(0, obj.setPropertyAsString(stringN, stringV));
        ASSERT_EQ(0, obj.setPropertyAsBinary(binaryN, binaryV));

        ASSERT_EQ(boolV, obj.getPropertyAsBool(boolN));
        ASSERT_EQ(charV, obj.getPropertyAsChar(charN));
        ASSERT_EQ(shortV, obj.getPropertyAsShort(shortN));
        ASSERT_EQ(intV, obj.getPropertyAsInt32(intN));
        ASSERT_EQ(int64V, obj.getPropertyAsInt64(int64N));
        ASSERT_EQ(stringV, obj.getPropertyAsString(stringN));
        ASSERT_EQ(binaryV, obj.getPropertyAsBinary(binaryN));
    }

    {
        // Test 'setPropertyAs*' to return error.
        bmqp::MessageProperties p(s_allocator_p);

        // Invalid property name.
        // ---------------------
        const bsl::string invalidPropName1("#MyPropName", s_allocator_p);
        const bsl::string invalidPropName2(
            bmqp::MessageProperties::k_MAX_PROPERTY_NAME_LENGTH + 1,
            'x',
            s_allocator_p);

        ASSERT_EQ(bmqt::GenericResult::e_INVALID_ARGUMENT,
                  p.setPropertyAsChar(invalidPropName1, 'A'));
        ASSERT_EQ(bmqt::GenericResult::e_INVALID_ARGUMENT,
                  p.setPropertyAsChar(invalidPropName2, 'A'));

        // Invalid property value length.
        // -----------------------------
        const bsl::string invalidValue(
            bmqp::MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH + 1,
            'x',
            s_allocator_p);

        ASSERT_EQ(bmqt::GenericResult::e_INVALID_ARGUMENT,
                  p.setPropertyAsString("dummy", invalidValue));

        // Property with same name but with different type exists.
        // ------------------------------------------------------
        ASSERT_EQ(0, p.setPropertyAsString("name", "value"));
        ASSERT_EQ(bmqt::GenericResult::e_INVALID_ARGUMENT,
                  p.setPropertyAsInt64("name", 123456789LL));

        // Property with the same name and type but no more space
        // left for bigger values.
        // ------------------------------------------------------
        const bsl::string bigValue(
            bmqp::MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH,
            'x',
            s_allocator_p);

        const int numProp =
            bmqp::MessageProperties::k_MAX_PROPERTIES_AREA_LENGTH /
            bmqp::MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH;

        for (int i = 0; i < numProp; ++i) {
            mwcu::MemOutStream osstr(s_allocator_p);
            osstr << "propName" << i << bsl::ends;
            ASSERT_EQ(0, p.setPropertyAsString(osstr.str(), bigValue));
        }

        ASSERT_EQ(bmqt::GenericResult::e_REFUSED,
                  p.setPropertyAsString("name", bigValue));

        // Exceed total number of properties.
        // ---------------------------------
        p.clear();  // Clear out any previous properties

        for (int i = 0; i < bmqp::MessageProperties::k_MAX_NUM_PROPERTIES;
             ++i) {
            mwcu::MemOutStream osstr(s_allocator_p);
            osstr << "propName" << i << bsl::ends;
            ASSERT_EQ(0, p.setPropertyAsInt32(osstr.str(), i));
        }
        // Add one more property, which should fail.
        ASSERT_EQ(bmqt::GenericResult::e_REFUSED,
                  p.setPropertyAsShort("dummy", 42));
    }
}

static void test3_binaryPropertyTest()
{
    // Ensure that a binary property is set and retrieved correctly.
    mwctst::TestHelper::printTestName("'setPropertyAsBinary' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(128, s_allocator_p);

    const size_t      length   = 2080;
    const char        binValue = bsl::numeric_limits<char>::max();
    bsl::vector<char> binaryV(length, binValue, s_allocator_p);
    const bsl::string binaryN("binPropName", s_allocator_p);

    bmqp::MessageProperties p(s_allocator_p);
    ASSERT_EQ(0, p.setPropertyAsBinary(binaryN, binaryV));

    bmqt::PropertyType::Enum ptype = bmqt::PropertyType::e_UNDEFINED;
    ASSERT_EQ(true, p.hasProperty(binaryN, &ptype));
    ASSERT_EQ(bmqt::PropertyType::e_BINARY, ptype);
    const bsl::vector<char>& outV = p.getPropertyAsBinary(binaryN);
    ASSERT_EQ(true, binaryV == outV);
}

static void test4_iteratorTest()
{
    // Ensure iterator's functionality is correct.
    mwctst::TestHelper::printTestName("'setPropertyAsBinary' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(128, s_allocator_p);
    bmqp::MessageProperties        p(s_allocator_p);
    const size_t                   numProps = 157;
    PropertyMap                    pmap(s_allocator_p);

    // Populate 'p' instance with various properties.
    populateProperties(&p, &pmap, numProps);

    ASSERT_EQ(static_cast<int>(numProps), p.numProperties());
    ASSERT_EQ(numProps, pmap.size());

    // Iterate over 'p' and verify that each entry in 'pmap' is encountered,
    // and encountered only once.  When an entry is encountered, we will simply
    // set the size of the property in 'pmap' to 0 (since we don't carry a
    // 'hasBeenEncountered' flag in value type of 'pmap').
    verify(&pmap, p);
}

static void test5_streamInTest()
{
    // Ensure 'streamIn' functionality is correct.
    mwctst::TestHelper::printTestName("'streamIn' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(128, s_allocator_p);
    bmqp::MessageProperties        p(s_allocator_p);
    bdlbb::Blob                    wireRep(&bufferFactory, s_allocator_p);
    bmqp::MessagePropertiesInfo    logic =
        bmqp::MessagePropertiesInfo::makeNoSchema();
    // Empty rep.
    p.streamIn(wireRep, logic.isExtended());
    ASSERT_EQ(0, p.numProperties());
    ASSERT_EQ(0, p.totalSize());

    // Real stuff.
    const size_t numProps = 207;
    PropertyMap  pmap(s_allocator_p);

    // Populate with various properties.

    // Note that `dummyP` is used only because `populateProperties`
    // requires one.
    bmqp::MessageProperties dummyP(s_allocator_p);

    populateProperties(&dummyP, &pmap, numProps);

    ASSERT_EQ(static_cast<int>(numProps), dummyP.numProperties());
    ASSERT_EQ(numProps, pmap.size());

    encode(&wireRep, pmap);

    ASSERT_EQ(0, p.streamIn(wireRep, logic.isExtended()));
    // Note that 'streamIn' will invoke 'clear' on 'p' unconditionally.

    // Recall that 'MessageProperties::totalSize()' returns length of its wire
    // representation *excluding* the word-aligned padding at the end.
    int padding = 0;
    if (wireRep.length()) {
        const bdlbb::BlobBuffer& lastBuf = wireRep.buffer(
            wireRep.numDataBuffers() - 1);
        padding = lastBuf.data()[wireRep.lastDataBufferLength() - 1];
    }

    ASSERT_EQ((p.totalSize() + padding), wireRep.length());

    verify(&pmap, p);
}

static void test6_streamOutTest()
{
    // Ensure 'streamOut' functionality is correct.
    mwctst::TestHelper::printTestName("'streamOut' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(128, s_allocator_p);
    bdlbb::Blob                    wireRep(&bufferFactory, s_allocator_p);
    bmqp::MessageProperties        p(s_allocator_p);
    bmqp::MessagePropertiesInfo    logic =
        bmqp::MessagePropertiesInfo::makeInvalidSchema();

    // Empty rep.
    const bdlbb::Blob& out = p.streamOut(&bufferFactory, logic);
    ASSERT_EQ(0, bdlbb::BlobUtil::compare(wireRep, out));

    // Non empty.
    const size_t numProps = 187;
    PropertyMap  pmap(s_allocator_p);

    populateProperties(&p, &pmap, numProps);

    ASSERT_EQ(static_cast<int>(numProps), p.numProperties());
    ASSERT_EQ(numProps, pmap.size());

    encode(&wireRep, pmap);
    verify(&pmap, p);

    const bdlbb::Blob& out2 = p.streamOut(&bufferFactory, logic);

    bdlbb::PooledBlobBufferFactory bigBufferFactory(1024, s_allocator_p);
    bdlbb::Blob                    buffer(&bigBufferFactory, s_allocator_p);

    ASSERT_EQ(0,
              bmqp::ProtocolUtil::convertToOld(
                  &buffer,
                  &out2,
                  bmqt::CompressionAlgorithmType::e_NONE,
                  &bufferFactory,
                  s_allocator_p));
    // 'streamOut' encodes in the new style, 'encode' - in the old
    ASSERT_EQ(0, bdlbb::BlobUtil::compare(wireRep, buffer));

    ASSERT_EQ(0,
              bmqp::ProtocolUtil::convertToOld(
                  const_cast<bdlbb::Blob*>(&out2),
                  &out2,
                  bmqt::CompressionAlgorithmType::e_NONE,
                  &bufferFactory,
                  s_allocator_p));
    // 'streamOut' encodes in the new style, 'encode' - in the old
    ASSERT_EQ(0, bdlbb::BlobUtil::compare(wireRep, out2));
}

static void test7_streamInOutMixTest()
{
    // Ensure that 'streamOut' works correctly if 'streamIn' was invoked before
    // on the instance.

    mwctst::TestHelper::printTestName("'streamIn/Out Mix' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(128, s_allocator_p);
    bdlbb::Blob                    wireRep(&bufferFactory, s_allocator_p);
    bmqp::MessageProperties        p(s_allocator_p);
    bmqp::MessagePropertiesInfo    logic =
        bmqp::MessagePropertiesInfo::makeNoSchema();

    // First stream in a valid wire-representation in an instance.
    const size_t numProps = 37;
    PropertyMap  pmap(s_allocator_p);

    // Populate with various properties.

    // Note that `dummyP` is used only because `populateProperties`
    // requires one.
    bmqp::MessageProperties dummyP(s_allocator_p);

    populateProperties(&dummyP, &pmap, numProps);

    ASSERT_EQ(static_cast<int>(numProps), dummyP.numProperties());
    ASSERT_EQ(numProps, pmap.size());

    encode(&wireRep, pmap);

    ASSERT_EQ(0, p.streamIn(wireRep, logic.isExtended()));

    // Recall that 'MessageProperties::totalSize()' returns length of its wire
    // representation *excluding* the word-aligned padding at the end.

    int padding = 0;
    if (wireRep.length()) {
        const bdlbb::BlobBuffer& lastBuf = wireRep.buffer(
            wireRep.numDataBuffers() - 1);
        padding = lastBuf.data()[wireRep.lastDataBufferLength() - 1];
    }

    ASSERT_EQ((p.totalSize() + padding), wireRep.length());

    verify(&pmap, p);

    // 'wireRep' has now been correctly streamed into 'p'.

    // Add another property in 'p' so that internal wire rep becomes dirty.
    bsl::string newPropName("1111111111", s_allocator_p);

    ASSERT_EQ(0, p.setPropertyAsBool(newPropName, false));
    ASSERT_EQ(static_cast<int>(numProps + 1), p.numProperties());

    // Now stream out 'p'.
    const bdlbb::Blob& newWireRep = p.streamOut(&bufferFactory, logic);
    if (newWireRep.length()) {
        const bdlbb::BlobBuffer& lastBuf = newWireRep.buffer(
            newWireRep.numDataBuffers() - 1);
        padding = lastBuf.data()[newWireRep.lastDataBufferLength() - 1];
    }
    ASSERT_EQ(newWireRep.length(), (p.totalSize() + padding));
}

static void test8_printTest()
// --------------------------------------------------------------------
// PRINT
//
// Concerns:
//   bmqp::MessageProperties print methods.
//
// Plan:
//   For every valid bmqt::PropertyType do the following:
//     1. Create a bmqp::MessageProperties object.
//     2. Add related property with a valid name and value.
//     3. Check than the output of the 'print' method and '<<' operator
//        matches the expected pattern.
//
// Testing:
//   bmqp::MessageProperties::print()
//
//   bsl::ostream&
//   bmqp::operator<<(bsl::ostream& stream,
//                    const bmqp::MessageProperties& rhs)
// --------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("PRINT");

    BSLMF_ASSERT(bmqt::PropertyType::e_BOOL ==
                 bmqt::PropertyType::k_LOWEST_SUPPORTED_PROPERTY_TYPE);

    BSLMF_ASSERT(bmqt::PropertyType::e_BINARY ==
                 bmqt::PropertyType::k_HIGHEST_SUPPORTED_PROPERTY_TYPE);

    const bsl::string boolN("boolPropName", s_allocator_p);
    const bsl::string charN("charPropName", s_allocator_p);
    const bsl::string shortN("shortPropName", s_allocator_p);
    const bsl::string intN("intPropName", s_allocator_p);
    const bsl::string int64N("int64PropName", s_allocator_p);
    const bsl::string stringN("stringPropName", s_allocator_p);
    const bsl::string binaryN("binaryPropName", s_allocator_p);

    bool               boolV  = true;
    char               charV  = 63;
    short              shortV = 500;
    int                intV   = 333;
    bsls::Types::Int64 int64V = 123LL;
    bsl::string        stringV(3, 'A', s_allocator_p);
    bsl::vector<char>  binaryV(15, 255, s_allocator_p);

    struct Test {
        bmqt::PropertyType::Enum d_type;
        const char*              d_expected;
    } k_DATA[] = {
        {bmqt::PropertyType::e_BOOL, "[ boolPropName (BOOL) = true ]"},
        {bmqt::PropertyType::e_CHAR, "[ charPropName (CHAR) = 3f ]"},
        {bmqt::PropertyType::e_SHORT, "[ shortPropName (SHORT) = 500 ]"},
        {bmqt::PropertyType::e_INT32, "[ intPropName (INT32) = 333 ]"},
        {bmqt::PropertyType::e_INT64, "[ int64PropName (INT64) = 123 ]"},
        {bmqt::PropertyType::e_STRING,
         "[ stringPropName (STRING) = \"AAA\" ]"},
        {bmqt::PropertyType::e_BINARY,
         "[ binaryPropName (BINARY) = \"     0:"
         "   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFF "
         "      |............... |\n\" ]"}};

    const size_t k_NUM_DATA = sizeof(k_DATA) / sizeof(*k_DATA);

    for (size_t idx = 0; idx < k_NUM_DATA; ++idx) {
        const Test&             test = k_DATA[idx];
        bmqp::MessageProperties obj(s_allocator_p);
        mwcu::MemOutStream      out(s_allocator_p);
        mwcu::MemOutStream      expected(s_allocator_p);

        switch (test.d_type) {
        case bmqt::PropertyType::e_BOOL: {
            obj.setPropertyAsBool(boolN, boolV);
        } break;
        case bmqt::PropertyType::e_CHAR: {
            obj.setPropertyAsChar(charN, charV);
        } break;
        case bmqt::PropertyType::e_SHORT: {
            obj.setPropertyAsShort(shortN, shortV);
        } break;
        case bmqt::PropertyType::e_INT32: {
            obj.setPropertyAsInt32(intN, intV);
        } break;
        case bmqt::PropertyType::e_INT64: {
            obj.setPropertyAsInt64(int64N, int64V);
        } break;
        case bmqt::PropertyType::e_STRING: {
            obj.setPropertyAsString(stringN, stringV);
        } break;
        case bmqt::PropertyType::e_BINARY: {
            obj.setPropertyAsBinary(binaryN, binaryV);
        } break;
        case bmqt::PropertyType::e_UNDEFINED:
        default: BSLS_ASSERT_OPT(false && "Unreachable by design.");
        }

        expected << test.d_expected;

        out.setstate(bsl::ios_base::badbit);
        obj.print(out, 0, -1);

        ASSERT_EQ(out.str(), "");

        out.clear();
        obj.print(out, 0, -1);

        ASSERT_EQ(out.str(), expected.str());

        out.reset();
        out << obj;

        ASSERT_EQ(out.str(), expected.str());
    }
}

static void test9_copyAssignTest()
// --------------------------------------------------------------------
// COPY AND ASSIGN
//
// Concerns:
//   bmqp::MessageProperties creators.
//
// Plan:
//     1. Create a bmqp::MessageProperties object.
//     2. Populate it with valid properties.
//     3. Check that new bmqp::MessageProperties objects
//        created from the initial one using a copy constructor
//        and an assignment operator have the same properties.
//
// Testing:
//   bmqp::MessageProperties(const MessageProperties&  other,
//                           bslma::Allocator         *basicAllocator = 0);
//   bmqp::MessageProperties& operator=(const MessageProperties& rhs);
// --------------------------------------------------------------------
{
    mwctst::TestHelper::printTestName("COPY AND ASSIGN");

    bdlbb::PooledBlobBufferFactory bufferFactory(128, s_allocator_p);
    bmqp::MessageProperties        obj(s_allocator_p);
    bdlbb::Blob                    wireRep(&bufferFactory, s_allocator_p);
    PropertyMap                    pmap(s_allocator_p);
    bmqp::MessagePropertiesInfo    logic =
        bmqp::MessagePropertiesInfo::makeNoSchema();

    const size_t numProps =
        bmqp::MessagePropertiesHeader::k_MAX_NUM_PROPERTIES;

    // Populate with various properties.

    // Note that `dummyP` is used only because `populateProperties`
    // requires one.
    bmqp::MessageProperties dummyP(s_allocator_p);

    populateProperties(&dummyP, &pmap, numProps);

    ASSERT_EQ(numProps, pmap.size());

    encode(&wireRep, pmap);

    ASSERT_EQ(0, obj.streamIn(wireRep, logic.isExtended()));

    // Check copy ctr
    bmqp::MessageProperties obj1 = obj;

    ASSERT_EQ(obj.totalSize(), obj1.totalSize());
    ASSERT_EQ(obj.numProperties(), obj1.numProperties());

    PropertyMap pmap1 = pmap;
    verify(&pmap1, obj1);

    // Check assignment
    bmqp::MessageProperties obj2;
    obj2 = obj;

    ASSERT_EQ(obj.totalSize(), obj2.totalSize());
    ASSERT_EQ(obj.numProperties(), obj2.numProperties());

    PropertyMap pmap2 = pmap;
    verify(&pmap2, obj2);

    // Check self assignment
    PropertyMap pmap3 = pmap;
    obj2              = obj2;
    verify(&pmap3, obj2);
}

static void test10_empty()
{
    // Even if SDK does not send out empty MPS, streaming out/in should work.

    mwctst::TestHelper::printTestName("'empty MPs' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(128, s_allocator_p);
    bmqp::MessageProperties        p(s_allocator_p);
    bdlbb::Blob                    wireRep(&bufferFactory, s_allocator_p);
    bmqp::MessagePropertiesInfo    logic(true, 1, true);

    // Empty rep.
    p.streamIn(wireRep, logic.isExtended());
    ASSERT_EQ(0, p.numProperties());
    ASSERT_EQ(0, p.totalSize());

    const bdlbb::Blob& out = p.streamOut(&bufferFactory, logic);
    ASSERT_EQ(0, out.length());

    ASSERT(!p.hasProperty("z"));
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(s_allocator_p);

    switch (_testCase) {
    case 0:
    case 10: test10_empty(); break;
    case 9: test9_copyAssignTest(); break;
    case 8: test8_printTest(); break;
    case 7: test7_streamInOutMixTest(); break;
    case 6: test6_streamOutTest(); break;
    case 5: test5_streamInTest(); break;
    case 4: test4_iteratorTest(); break;
    case 3: test3_binaryPropertyTest(); break;
    case 2: test2_setPropertyTest(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        s_testStatus = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_GBL_ALLOC);

    // Check for default allocator is explicitly disabled as
    // 'bmqp::MessageProperties' or one of its data members may allocate
    // temporaries with default allocator.
}
