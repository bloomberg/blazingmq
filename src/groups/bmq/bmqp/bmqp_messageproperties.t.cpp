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
#include <bmqu_memoutstream.h>

#include <bmqu_blob.h>
#include <bmqu_memoutstream.h>

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
#include <bmqtst_testhelper.h>
#include <bsl_cstring.h>
#include <bsl_ostream.h>

#ifdef BMQTST_BENCHMARK_ENABLED
#include <bslmt_barrier.h>
#include <bslmt_latch.h>
#include <bslmt_threadgroup.h>

// BENCHMARKING LIBRARY
#include <benchmark/benchmark.h>
#endif

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
    void operator()(BSLA_UNUSED const TYPE& value)
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
        bmqu::MemOutStream osstr(bmqtst::TestHelperUtil::allocator());

        switch (remainder) {
        case 0: {
            osstr << "boolPropName" << i << bsl::ends;
            bool value = false;
            if (0 == i % 2) {
                value = true;
            }

            BMQTST_ASSERT_EQ_D(i, 0, p.setPropertyAsBool(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_BOOL, 1),  // size
                    PropertyVariant(value))));

            BMQTST_ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 1: {
            osstr << "charPropName" << i << bsl::ends;
            char value = static_cast<char>(bsl::numeric_limits<char>::max() /
                                           i);

            BMQTST_ASSERT_EQ_D(i, 0, p.setPropertyAsChar(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_CHAR, sizeof(value)),
                    PropertyVariant(value))));

            BMQTST_ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 2: {
            osstr << "shortPropName" << i << bsl::ends;
            short value = static_cast<short>(
                bsl::numeric_limits<short>::max() / i);

            BMQTST_ASSERT_EQ_D(i, 0, p.setPropertyAsShort(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_SHORT, sizeof(value)),
                    PropertyVariant(value))));

            BMQTST_ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 3: {
            osstr << "intPropName" << i << bsl::ends;
            int value = static_cast<int>(bsl::numeric_limits<int>::max() / i);

            BMQTST_ASSERT_EQ_D(i, 0, p.setPropertyAsInt32(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_INT32, sizeof(value)),
                    PropertyVariant(value))));

            BMQTST_ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 4: {
            osstr << "int64PropName" << i << bsl::ends;
            bsls::Types::Int64 value =
                bsl::numeric_limits<bsls::Types::Int64>::max() / i;

            BMQTST_ASSERT_EQ_D(i, 0, p.setPropertyAsInt64(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_INT64, sizeof(value)),
                    PropertyVariant(value))));

            BMQTST_ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 5: {
            osstr << "stringPropName" << i << bsl::ends;
            const bsl::string value(i,
                                    'x',
                                    bmqtst::TestHelperUtil::allocator());

            BMQTST_ASSERT_EQ_D(i,
                               0,
                               p.setPropertyAsString(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_STRING, value.size()),
                    PropertyVariant(value))));

            BMQTST_ASSERT_EQ_D(i, true, insertRc.second);
        } break;  // BREAK

        case 6: {
            osstr << "binaryPropName" << i << bsl::ends;
            const bsl::vector<char> value(i,
                                          'x',
                                          bmqtst::TestHelperUtil::allocator());

            BMQTST_ASSERT_EQ_D(i,
                               0,
                               p.setPropertyAsBinary(osstr.str(), value));

            PropertyMapInsertRc insertRc = pmap.insert(bsl::make_pair(
                osstr.str(),
                PropertyTypeSizeVariantPair(
                    bsl::make_pair(bmqt::PropertyType::e_BINARY, value.size()),
                    PropertyVariant(value))));

            BMQTST_ASSERT_EQ_D(i, true, insertRc.second);
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

        BMQTST_ASSERT_EQ_D(iteration, false, pmap.end() == pmapIt);

        PropertyTypeSizeVariantPair& tsvPair = pmapIt->second;

        BMQTST_ASSERT_EQ_D(iteration, pmapIt->first, propName);
        BMQTST_ASSERT_EQ_D(iteration, iter.type(), tsvPair.first.first);
        BMQTST_ASSERT_EQ_D(iteration, false, 0 == tsvPair.first.second);

        const size_t propSize = tsvPair.first.second;

        // Set the size to be '0' to record that property has been seen.
        tsvPair.first.second = 0;

        switch (tsvPair.first.first) {
        case bmqt::PropertyType::e_BOOL: {
            BMQTST_ASSERT_EQ_D(iteration,
                               1u,  // bool uses hardcoded size of 1
                               propSize);

            BMQTST_ASSERT_EQ_D(iteration,
                               iter.getAsBool(),
                               tsvPair.second.the<bool>());
        } break;  // BREAK

        case bmqt::PropertyType::e_CHAR: {
            BMQTST_ASSERT_EQ_D(iteration, sizeof(char), propSize);

            BMQTST_ASSERT_EQ_D(iteration,
                               iter.getAsChar(),
                               tsvPair.second.the<char>());
        } break;  // BREAK

        case bmqt::PropertyType::e_SHORT: {
            BMQTST_ASSERT_EQ_D(iteration, sizeof(short), propSize);

            BMQTST_ASSERT_EQ_D(iteration,
                               iter.getAsShort(),
                               tsvPair.second.the<short>());
        } break;  // BREAK

        case bmqt::PropertyType::e_INT32: {
            BMQTST_ASSERT_EQ_D(iteration, sizeof(int), propSize);

            BMQTST_ASSERT_EQ_D(iteration,
                               iter.getAsInt32(),
                               tsvPair.second.the<int>());
        } break;  // BREAK

        case bmqt::PropertyType::e_INT64: {
            BMQTST_ASSERT_EQ_D(iteration,
                               sizeof(bsls::Types::Int64),
                               propSize);

            BMQTST_ASSERT_EQ_D(iteration,
                               iter.getAsInt64(),
                               tsvPair.second.the<bsls::Types::Int64>());
        } break;  // BREAK

        case bmqt::PropertyType::e_STRING: {
            const bsl::string& value = iter.getAsString();

            BMQTST_ASSERT_EQ_D(iteration, value.size(), propSize);

            BMQTST_ASSERT_EQ_D(iteration,
                               value,
                               tsvPair.second.the<bsl::string>());
        } break;  // BREAK

        case bmqt::PropertyType::e_BINARY: {
            const bsl::vector<char>& value = iter.getAsBinary();

            BMQTST_ASSERT_EQ_D(iteration, value.size(), propSize);

            BMQTST_ASSERT_EQ_D(iteration,
                               value,
                               tsvPair.second.the<bsl::vector<char> >());
        } break;  // BREAK

        case bmqt::PropertyType::e_UNDEFINED:
        default: BSLS_ASSERT_OPT(false && "Unreachable by design");
        }

        ++iteration;
    }

    BMQTST_ASSERT_EQ(static_cast<size_t>(iteration), pmap.size());
    BMQTST_ASSERT_EQ(static_cast<size_t>(properties.numProperties()),
                     pmap.size());
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
    bmqu::BlobUtil::reserve(blob, sizeof(bmqp::MessagePropertiesHeader));

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
        mph.setPropertyValueLength(static_cast<int>(tsvPair.first.second));

        bdlbb::BlobUtil::append(blob,
                                reinterpret_cast<const char*>(&mph),
                                sizeof(mph));
        totalSize += sizeof(bmqp::MessagePropertyHeader);
    }

    // Second pass.
    for (PropertyMapConstIter cit = pmap.begin(); cit != pmap.end(); ++cit) {
        const PropertyTypeSizeVariantPair& tsvPair = cit->second;
        bdlbb::BlobUtil::append(blob,
                                cit->first.c_str(),
                                static_cast<int>(cit->first.length()));
        totalSize += static_cast<int>(cit->first.length());

        PropertyValueStreamOutVisitor visitor(blob);
        tsvPair.second.apply(visitor);
        totalSize += static_cast<int>(tsvPair.first.second);
    }

    BMQTST_ASSERT_EQ(totalSize, b.length());
    BMQTST_ASSERT_EQ(
        true,
        totalSize <= bmqp::MessageProperties::k_MAX_PROPERTIES_AREA_LENGTH);

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
    bmqtst::TestHelper::printTestName("BREATHING TEST");
    PV("Testing MessageProperties");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bmqp::MessageProperties     p(bmqtst::TestHelperUtil::allocator());
    int                         totalLen = 0;
    bmqp::MessagePropertiesInfo logic =
        bmqp::MessagePropertiesInfo::makeInvalidSchema();
    // Empty instance.
    BMQTST_ASSERT_EQ(0, p.numProperties());
    BMQTST_ASSERT_EQ(0, p.totalSize());
    BMQTST_ASSERT_EQ(false, p.remove("foobar"));

    bmqt::PropertyType::Enum ptype = bmqt::PropertyType::e_UNDEFINED;
    BMQTST_ASSERT_EQ(false, p.remove("none", &ptype));
    BMQTST_ASSERT_EQ(ptype, bmqt::PropertyType::e_UNDEFINED);

    BMQTST_ASSERT_EQ(false, p.hasProperty("foobar"));

    BMQTST_ASSERT_EQ(0, p.streamOut(&bufferFactory, logic).length());

    // Clear out the instance and repeat above.
    p.clear();

    BMQTST_ASSERT_EQ(0, p.numProperties());
    BMQTST_ASSERT_EQ(false, p.remove("foobar"));
    BMQTST_ASSERT_EQ(false, p.remove("none", &ptype));
    BMQTST_ASSERT_EQ(ptype, bmqt::PropertyType::e_UNDEFINED);
    BMQTST_ASSERT_EQ(false, p.hasProperty("foobar"));
    BMQTST_ASSERT_EQ(0, p.streamOut(&bufferFactory, logic).length());

    // Add a property.
    BMQTST_ASSERT_EQ(0, p.setPropertyAsChar("category", 'Q'));

    // Update 'totalLen'.  Since this is first property, add size of overall
    // properties header as well.
    totalLen += sizeof(bmqp::MessagePropertiesHeader) +
                sizeof(bmqp::MessagePropertyHeader) + bsl::strlen("category") +
                +1;

    BMQTST_ASSERT_EQ(p.numProperties(), 1);
    BMQTST_ASSERT_EQ(p.hasProperty("category"), true);
    BMQTST_ASSERT_EQ(p.propertyType("category"), bmqt::PropertyType::e_CHAR);
    BMQTST_ASSERT_EQ(p.getPropertyAsChar("category"), 'Q');
    BMQTST_ASSERT_EQ(p.totalSize(), totalLen);
    BMQTST_ASSERT_GT(p.streamOut(&bufferFactory, logic).length(), totalLen);

    BMQTST_ASSERT_EQ(p.remove("foobar"), false);
    BMQTST_ASSERT_EQ(p.hasProperty("foobar"), false);

    // Add another property.

    BMQTST_ASSERT_EQ(p.setPropertyAsInt64("timestamp", 123456789LL), 0);
    totalLen += sizeof(bmqp::MessagePropertyHeader) +
                bsl::strlen("timestamp") + sizeof(bsls::Types::Int64);

    BMQTST_ASSERT_EQ(p.numProperties(), 2);
    BMQTST_ASSERT_EQ(p.hasProperty("timestamp"), true);
    BMQTST_ASSERT_EQ(p.propertyType("timestamp"), bmqt::PropertyType::e_INT64);
    BMQTST_ASSERT_EQ(p.getPropertyAsInt64("timestamp"), 123456789LL);
    BMQTST_ASSERT_EQ(p.totalSize(), totalLen);
    BMQTST_ASSERT_GT(p.streamOut(&bufferFactory, logic).length(), totalLen);

    // Delete a property.

    BMQTST_ASSERT_EQ(true, p.remove("timestamp", &ptype));
    BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT64, ptype);

    totalLen -= static_cast<int>(sizeof(bmqp::MessagePropertyHeader) +
                                 bsl::strlen("timestamp") +
                                 sizeof(bsls::Types::Int64));

    BMQTST_ASSERT_EQ(p.numProperties(), 1);
    BMQTST_ASSERT_EQ(p.hasProperty("category"), true);
    BMQTST_ASSERT_EQ(p.propertyType("category"), bmqt::PropertyType::e_CHAR);
    BMQTST_ASSERT_EQ(p.getPropertyAsChar("category"), 'Q');
    BMQTST_ASSERT_EQ(p.totalSize(), totalLen);
    BMQTST_ASSERT_GT(p.streamOut(&bufferFactory, logic).length(), totalLen);

    // Test 'Or' flavor.
    const bsl::string       dummyName("blahblah",
                                bmqtst::TestHelperUtil::allocator());
    const bsl::string       defaultVal("defval",
                                 bmqtst::TestHelperUtil::allocator());
    const bsl::vector<char> defaultBin(1024,
                                       'F',
                                       bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(p.getPropertyAsStringOr(dummyName, defaultVal),
                     defaultVal);
    BMQTST_ASSERT_EQ(p.getPropertyAsInt32Or(dummyName, 42), 42);
    BMQTST_ASSERT_EQ(p.getPropertyAsBoolOr(dummyName, true), true);
    BMQTST_ASSERT_EQ(p.getPropertyAsCharOr(dummyName, 'Z'), 'Z');
    BMQTST_ASSERT_EQ(p.getPropertyAsShortOr(dummyName, 11), 11);
    BMQTST_ASSERT_EQ(p.getPropertyAsInt64Or(dummyName, 987LL), 987LL);
    BMQTST_ASSERT_EQ(p.getPropertyAsBinaryOr(dummyName, defaultBin),
                     defaultBin);

    PV("Testing empty MessagePropertiesIterator");

    bmqp::MessagePropertiesIterator objIt;

    BMQTST_ASSERT_SAFE_FAIL(objIt.hasNext());
    BMQTST_ASSERT_SAFE_FAIL(objIt.name());
    BMQTST_ASSERT_SAFE_FAIL(objIt.type());
    BMQTST_ASSERT_SAFE_FAIL(objIt.getAsBool());
    BMQTST_ASSERT_SAFE_FAIL(objIt.getAsChar());
    BMQTST_ASSERT_SAFE_FAIL(objIt.getAsShort());
    BMQTST_ASSERT_SAFE_FAIL(objIt.getAsInt32());
    BMQTST_ASSERT_SAFE_FAIL(objIt.getAsInt64());
    BMQTST_ASSERT_SAFE_FAIL(objIt.getAsString());
    BMQTST_ASSERT_SAFE_FAIL(objIt.getAsBinary());
}

static void test2_setPropertyTest()
{
    bmqtst::TestHelper::printTestName("'setPropertyAs*' TEST");

    {
        // Test all flavors of 'setPropertyAs*'.
        bmqp::MessageProperties obj(bmqtst::TestHelperUtil::allocator());

        const bsl::string boolN("boolPropName",
                                bmqtst::TestHelperUtil::allocator());
        const bsl::string charN("charPropName",
                                bmqtst::TestHelperUtil::allocator());
        const bsl::string shortN("shortPropName",
                                 bmqtst::TestHelperUtil::allocator());
        const bsl::string intN("intPropName",
                               bmqtst::TestHelperUtil::allocator());
        const bsl::string int64N("int64PropName",
                                 bmqtst::TestHelperUtil::allocator());
        const bsl::string stringN("stringPropName",
                                  bmqtst::TestHelperUtil::allocator());
        const bsl::string binaryN("binaryPropName",
                                  bmqtst::TestHelperUtil::allocator());

        bool               boolV  = true;
        char               charV  = bsl::numeric_limits<char>::max();
        short              shortV = bsl::numeric_limits<short>::max();
        int                intV   = bsl::numeric_limits<int>::max();
        bsls::Types::Int64 int64V =
            bsl::numeric_limits<bsls::Types::Int64>::max();

        bsl::string stringV(42, 'x', bmqtst::TestHelperUtil::allocator());
        bsl::vector<char> binaryV(84,
                                  250,
                                  bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_NE(0, obj.setPropertyAsBool("", boolV));
        BMQTST_ASSERT_NE(0, obj.setPropertyAsChar("", charV));
        BMQTST_ASSERT_NE(0, obj.setPropertyAsShort("", shortV));
        BMQTST_ASSERT_NE(0, obj.setPropertyAsInt32("", intV));
        BMQTST_ASSERT_NE(0, obj.setPropertyAsInt64("", int64V));
        BMQTST_ASSERT_NE(0, obj.setPropertyAsString("", stringV));
        BMQTST_ASSERT_NE(0, obj.setPropertyAsBinary("", binaryV));

        BMQTST_ASSERT_EQ(0, obj.setPropertyAsBool(boolN, boolV));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsChar(charN, charV));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsShort(shortN, shortV));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsInt32(intN, intV));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsInt64(int64N, int64V));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsString(stringN, stringV));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsBinary(binaryN, binaryV));

        BMQTST_ASSERT_EQ(7, obj.numProperties());

        BMQTST_ASSERT_EQ(true, obj.hasProperty(boolN));
        BMQTST_ASSERT_EQ(true, obj.hasProperty(charN));
        BMQTST_ASSERT_EQ(true, obj.hasProperty(shortN));
        BMQTST_ASSERT_EQ(true, obj.hasProperty(intN));
        BMQTST_ASSERT_EQ(true, obj.hasProperty(int64N));
        BMQTST_ASSERT_EQ(true, obj.hasProperty(stringN));
        BMQTST_ASSERT_EQ(true, obj.hasProperty(binaryN));

        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_BOOL, obj.propertyType(boolN));
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_CHAR, obj.propertyType(charN));
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_SHORT,
                         obj.propertyType(shortN));
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT32, obj.propertyType(intN));
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT64,
                         obj.propertyType(int64N));
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_STRING,
                         obj.propertyType(stringN));
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_BINARY,
                         obj.propertyType(binaryN));

        bmqt::PropertyType::Enum ptype = bmqt::PropertyType::e_UNDEFINED;

        BMQTST_ASSERT_EQ(obj.hasProperty(boolN, &ptype), true);
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_BOOL, ptype);

        BMQTST_ASSERT_EQ(obj.hasProperty(charN, &ptype), true);
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_CHAR, ptype);

        BMQTST_ASSERT_EQ(obj.hasProperty(shortN, &ptype), true);
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_SHORT, ptype);

        BMQTST_ASSERT_EQ(obj.hasProperty(intN, &ptype), true);
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT32, ptype);

        BMQTST_ASSERT_EQ(obj.hasProperty(int64N, &ptype), true);
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_INT64, ptype);

        BMQTST_ASSERT_EQ(obj.hasProperty(stringN, &ptype), true);
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_STRING, ptype);

        BMQTST_ASSERT_EQ(obj.hasProperty(binaryN, &ptype), true);
        BMQTST_ASSERT_EQ(bmqt::PropertyType::e_BINARY, ptype);

        BMQTST_ASSERT_EQ(boolV, obj.getPropertyAsBool(boolN));
        BMQTST_ASSERT_EQ(charV, obj.getPropertyAsChar(charN));
        BMQTST_ASSERT_EQ(shortV, obj.getPropertyAsShort(shortN));
        BMQTST_ASSERT_EQ(intV, obj.getPropertyAsInt32(intN));
        BMQTST_ASSERT_EQ(int64V, obj.getPropertyAsInt64(int64N));
        BMQTST_ASSERT_EQ(stringV, obj.getPropertyAsString(stringN));
        BMQTST_ASSERT_EQ(binaryV, obj.getPropertyAsBinary(binaryN));

        // Update the values of the existed properties.
        // --------------------------------------------
        boolV   = false;
        charV   = 'q';
        shortV  = 17;
        intV    = 987;
        int64V  = 123456LL;
        stringV = bsl::string(42, 'z', bmqtst::TestHelperUtil::allocator());
        binaryV = bsl::vector<char>(29,
                                    170,
                                    bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(0, obj.setPropertyAsBool(boolN, boolV));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsChar(charN, charV));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsShort(shortN, shortV));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsInt32(intN, intV));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsInt64(int64N, int64V));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsString(stringN, stringV));
        BMQTST_ASSERT_EQ(0, obj.setPropertyAsBinary(binaryN, binaryV));

        BMQTST_ASSERT_EQ(boolV, obj.getPropertyAsBool(boolN));
        BMQTST_ASSERT_EQ(charV, obj.getPropertyAsChar(charN));
        BMQTST_ASSERT_EQ(shortV, obj.getPropertyAsShort(shortN));
        BMQTST_ASSERT_EQ(intV, obj.getPropertyAsInt32(intN));
        BMQTST_ASSERT_EQ(int64V, obj.getPropertyAsInt64(int64N));
        BMQTST_ASSERT_EQ(stringV, obj.getPropertyAsString(stringN));
        BMQTST_ASSERT_EQ(binaryV, obj.getPropertyAsBinary(binaryN));
    }

    {
        // Test 'setPropertyAs*' to return error.
        bmqp::MessageProperties p(bmqtst::TestHelperUtil::allocator());

        // Invalid property name.
        // ---------------------
        const bsl::string invalidPropName1(
            "#MyPropName",
            bmqtst::TestHelperUtil::allocator());
        const bsl::string invalidPropName2(
            bmqp::MessageProperties::k_MAX_PROPERTY_NAME_LENGTH + 1,
            'x',
            bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(bmqt::GenericResult::e_INVALID_ARGUMENT,
                         p.setPropertyAsChar(invalidPropName1, 'A'));
        BMQTST_ASSERT_EQ(bmqt::GenericResult::e_INVALID_ARGUMENT,
                         p.setPropertyAsChar(invalidPropName2, 'A'));

        // Invalid property value length.
        // -----------------------------
        const bsl::string invalidValue(
            bmqp::MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH + 1,
            'x',
            bmqtst::TestHelperUtil::allocator());

        BMQTST_ASSERT_EQ(bmqt::GenericResult::e_INVALID_ARGUMENT,
                         p.setPropertyAsString("dummy", invalidValue));

        // Property with same name but with different type exists.
        // ------------------------------------------------------
        BMQTST_ASSERT_EQ(0, p.setPropertyAsString("name", "value"));
        BMQTST_ASSERT_EQ(bmqt::GenericResult::e_INVALID_ARGUMENT,
                         p.setPropertyAsInt64("name", 123456789LL));

        // Property with the same name and type but no more space
        // left for bigger values.
        // ------------------------------------------------------
        const bsl::string bigValue(
            bmqp::MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH,
            'x',
            bmqtst::TestHelperUtil::allocator());

        const int numProp =
            bmqp::MessageProperties::k_MAX_PROPERTIES_AREA_LENGTH /
            bmqp::MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH;

        for (int i = 0; i < numProp; ++i) {
            bmqu::MemOutStream osstr(bmqtst::TestHelperUtil::allocator());
            osstr << "propName" << i << bsl::ends;
            BMQTST_ASSERT_EQ(0, p.setPropertyAsString(osstr.str(), bigValue));
        }

        BMQTST_ASSERT_EQ(bmqt::GenericResult::e_REFUSED,
                         p.setPropertyAsString("name", bigValue));

        // Exceed total number of properties.
        // ---------------------------------
        p.clear();  // Clear out any previous properties

        for (int i = 0; i < bmqp::MessageProperties::k_MAX_NUM_PROPERTIES;
             ++i) {
            bmqu::MemOutStream osstr(bmqtst::TestHelperUtil::allocator());
            osstr << "propName" << i << bsl::ends;
            BMQTST_ASSERT_EQ(0, p.setPropertyAsInt32(osstr.str(), i));
        }
        // Add one more property, which should fail.
        BMQTST_ASSERT_EQ(bmqt::GenericResult::e_REFUSED,
                         p.setPropertyAsShort("dummy", 42));
    }
}

static void test3_binaryPropertyTest()
{
    // Ensure that a binary property is set and retrieved correctly.
    bmqtst::TestHelper::printTestName("'setPropertyAsBinary' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());

    const size_t      length   = 2080;
    const char        binValue = bsl::numeric_limits<char>::max();
    bsl::vector<char> binaryV(length,
                              binValue,
                              bmqtst::TestHelperUtil::allocator());
    const bsl::string binaryN("binPropName",
                              bmqtst::TestHelperUtil::allocator());

    bmqp::MessageProperties p(bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_EQ(0, p.setPropertyAsBinary(binaryN, binaryV));

    bmqt::PropertyType::Enum ptype = bmqt::PropertyType::e_UNDEFINED;
    BMQTST_ASSERT_EQ(true, p.hasProperty(binaryN, &ptype));
    BMQTST_ASSERT_EQ(bmqt::PropertyType::e_BINARY, ptype);
    const bsl::vector<char>& outV = p.getPropertyAsBinary(binaryN);
    BMQTST_ASSERT_EQ(true, binaryV == outV);
}

static void test4_iteratorTest()
{
    // Ensure iterator's functionality is correct.
    bmqtst::TestHelper::printTestName("'setPropertyAsBinary' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bmqp::MessageProperties p(bmqtst::TestHelperUtil::allocator());
    const size_t            numProps = 157;
    PropertyMap             pmap(bmqtst::TestHelperUtil::allocator());

    // Populate 'p' instance with various properties.
    populateProperties(&p, &pmap, numProps);

    BMQTST_ASSERT_EQ(static_cast<int>(numProps), p.numProperties());
    BMQTST_ASSERT_EQ(numProps, pmap.size());

    // Iterate over 'p' and verify that each entry in 'pmap' is encountered,
    // and encountered only once.  When an entry is encountered, we will simply
    // set the size of the property in 'pmap' to 0 (since we don't carry a
    // 'hasBeenEncountered' flag in value type of 'pmap').
    verify(&pmap, p);
}

static void test5_streamInTest()
{
    // Ensure 'streamIn' functionality is correct.
    bmqtst::TestHelper::printTestName("'streamIn' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bmqp::MessageProperties p(bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob wireRep(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::MessagePropertiesInfo logic =
        bmqp::MessagePropertiesInfo::makeNoSchema();
    // Empty rep.
    p.streamIn(wireRep, logic.isExtended());
    BMQTST_ASSERT_EQ(0, p.numProperties());
    BMQTST_ASSERT_EQ(0, p.totalSize());

    // Real stuff.
    const size_t numProps = 207;
    PropertyMap  pmap(bmqtst::TestHelperUtil::allocator());

    // Populate with various properties.

    // Note that `dummyP` is used only because `populateProperties`
    // requires one.
    bmqp::MessageProperties dummyP(bmqtst::TestHelperUtil::allocator());

    populateProperties(&dummyP, &pmap, numProps);

    BMQTST_ASSERT_EQ(static_cast<int>(numProps), dummyP.numProperties());
    BMQTST_ASSERT_EQ(numProps, pmap.size());

    encode(&wireRep, pmap);

    BMQTST_ASSERT_EQ(0, p.streamIn(wireRep, logic.isExtended()));
    // Note that 'streamIn' will invoke 'clear' on 'p' unconditionally.

    // Recall that 'MessageProperties::totalSize()' returns length of its wire
    // representation *excluding* the word-aligned padding at the end.
    int padding = 0;
    if (wireRep.length()) {
        const bdlbb::BlobBuffer& lastBuf = wireRep.buffer(
            wireRep.numDataBuffers() - 1);
        padding = lastBuf.data()[wireRep.lastDataBufferLength() - 1];
    }

    BMQTST_ASSERT_EQ((p.totalSize() + padding), wireRep.length());

    verify(&pmap, p);
}

static void test6_streamOutTest()
{
    // Ensure 'streamOut' functionality is correct.
    bmqtst::TestHelper::printTestName("'streamOut' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob wireRep(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::MessageProperties     p(bmqtst::TestHelperUtil::allocator());
    bmqp::MessagePropertiesInfo logic =
        bmqp::MessagePropertiesInfo::makeInvalidSchema();

    // Empty rep.
    const bdlbb::Blob& out = p.streamOut(&bufferFactory, logic);
    BMQTST_ASSERT_EQ(0, bdlbb::BlobUtil::compare(wireRep, out));

    // Non empty.
    const size_t numProps = 187;
    PropertyMap  pmap(bmqtst::TestHelperUtil::allocator());

    populateProperties(&p, &pmap, numProps);

    BMQTST_ASSERT_EQ(static_cast<int>(numProps), p.numProperties());
    BMQTST_ASSERT_EQ(numProps, pmap.size());

    encode(&wireRep, pmap);
    verify(&pmap, p);

    const bdlbb::Blob& out2 = p.streamOut(&bufferFactory, logic);

    bdlbb::PooledBlobBufferFactory bigBufferFactory(
        1024,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob buffer(&bigBufferFactory, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(0,
                     bmqp::ProtocolUtil::convertToOld(
                         &buffer,
                         &out2,
                         bmqt::CompressionAlgorithmType::e_NONE,
                         &bufferFactory,
                         bmqtst::TestHelperUtil::allocator()));
    // 'streamOut' encodes in the new style, 'encode' - in the old
    BMQTST_ASSERT_EQ(0, bdlbb::BlobUtil::compare(wireRep, buffer));

    BMQTST_ASSERT_EQ(0,
                     bmqp::ProtocolUtil::convertToOld(
                         const_cast<bdlbb::Blob*>(&out2),
                         &out2,
                         bmqt::CompressionAlgorithmType::e_NONE,
                         &bufferFactory,
                         bmqtst::TestHelperUtil::allocator()));
    // 'streamOut' encodes in the new style, 'encode' - in the old
    BMQTST_ASSERT_EQ(0, bdlbb::BlobUtil::compare(wireRep, out2));
}

static void test7_streamInOutMixTest()
{
    // Ensure that 'streamOut' works correctly if 'streamIn' was invoked before
    // on the instance.

    bmqtst::TestHelper::printTestName("'streamIn/Out Mix' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob wireRep(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::MessageProperties     p(bmqtst::TestHelperUtil::allocator());
    bmqp::MessagePropertiesInfo logic =
        bmqp::MessagePropertiesInfo::makeNoSchema();

    // First stream in a valid wire-representation in an instance.
    const size_t numProps = 37;
    PropertyMap  pmap(bmqtst::TestHelperUtil::allocator());

    // Populate with various properties.

    // Note that `dummyP` is used only because `populateProperties`
    // requires one.
    bmqp::MessageProperties dummyP(bmqtst::TestHelperUtil::allocator());

    populateProperties(&dummyP, &pmap, numProps);

    BMQTST_ASSERT_EQ(static_cast<int>(numProps), dummyP.numProperties());
    BMQTST_ASSERT_EQ(numProps, pmap.size());

    encode(&wireRep, pmap);

    BMQTST_ASSERT_EQ(0, p.streamIn(wireRep, logic.isExtended()));

    // Recall that 'MessageProperties::totalSize()' returns length of its wire
    // representation *excluding* the word-aligned padding at the end.

    int padding = 0;
    if (wireRep.length()) {
        const bdlbb::BlobBuffer& lastBuf = wireRep.buffer(
            wireRep.numDataBuffers() - 1);
        padding = lastBuf.data()[wireRep.lastDataBufferLength() - 1];
    }

    BMQTST_ASSERT_EQ((p.totalSize() + padding), wireRep.length());

    verify(&pmap, p);

    // 'wireRep' has now been correctly streamed into 'p'.

    // Add another property in 'p' so that internal wire rep becomes dirty.
    bsl::string newPropName("1111111111", bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(0, p.setPropertyAsBool(newPropName, false));
    BMQTST_ASSERT_EQ(static_cast<int>(numProps + 1), p.numProperties());

    // Now stream out 'p'.
    const bdlbb::Blob& newWireRep = p.streamOut(&bufferFactory, logic);
    if (newWireRep.length()) {
        const bdlbb::BlobBuffer& lastBuf = newWireRep.buffer(
            newWireRep.numDataBuffers() - 1);
        padding = lastBuf.data()[newWireRep.lastDataBufferLength() - 1];
    }
    BMQTST_ASSERT_EQ(newWireRep.length(), (p.totalSize() + padding));
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
    bmqtst::TestHelper::printTestName("PRINT");

    BSLMF_ASSERT(bmqt::PropertyType::e_BOOL ==
                 bmqt::PropertyType::k_LOWEST_SUPPORTED_PROPERTY_TYPE);

    BSLMF_ASSERT(bmqt::PropertyType::e_BINARY ==
                 bmqt::PropertyType::k_HIGHEST_SUPPORTED_PROPERTY_TYPE);

    const bsl::string boolN("boolPropName",
                            bmqtst::TestHelperUtil::allocator());
    const bsl::string charN("charPropName",
                            bmqtst::TestHelperUtil::allocator());
    const bsl::string shortN("shortPropName",
                             bmqtst::TestHelperUtil::allocator());
    const bsl::string intN("intPropName", bmqtst::TestHelperUtil::allocator());
    const bsl::string int64N("int64PropName",
                             bmqtst::TestHelperUtil::allocator());
    const bsl::string stringN("stringPropName",
                              bmqtst::TestHelperUtil::allocator());
    const bsl::string binaryN("binaryPropName",
                              bmqtst::TestHelperUtil::allocator());

    bool               boolV  = true;
    char               charV  = 63;
    short              shortV = 500;
    int                intV   = 333;
    bsls::Types::Int64 int64V = 123LL;
    bsl::string        stringV(3, 'A', bmqtst::TestHelperUtil::allocator());
    bsl::vector<char>  binaryV(15, 255, bmqtst::TestHelperUtil::allocator());

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
        bmqp::MessageProperties obj(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream      out(bmqtst::TestHelperUtil::allocator());
        bmqu::MemOutStream      expected(bmqtst::TestHelperUtil::allocator());

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

        BMQTST_ASSERT_EQ(out.str(), "");

        out.clear();
        obj.print(out, 0, -1);

        BMQTST_ASSERT_EQ(out.str(), expected.str());

        out.reset();
        out << obj;

        BMQTST_ASSERT_EQ(out.str(), expected.str());
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
    bmqtst::TestHelper::printTestName("COPY AND ASSIGN");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bmqp::MessageProperties obj(bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob wireRep(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    PropertyMap pmap(bmqtst::TestHelperUtil::allocator());
    bmqp::MessagePropertiesInfo logic =
        bmqp::MessagePropertiesInfo::makeNoSchema();

    const size_t numProps =
        bmqp::MessagePropertiesHeader::k_MAX_NUM_PROPERTIES;

    // Populate with various properties.

    // Note that `dummyP` is used only because `populateProperties`
    // requires one.
    bmqp::MessageProperties dummyP(bmqtst::TestHelperUtil::allocator());

    populateProperties(&dummyP, &pmap, numProps);

    BMQTST_ASSERT_EQ(numProps, pmap.size());

    encode(&wireRep, pmap);

    BMQTST_ASSERT_EQ(0, obj.streamIn(wireRep, logic.isExtended()));

    // Check copy ctr
    bmqp::MessageProperties obj1 = obj;

    BMQTST_ASSERT_EQ(obj.totalSize(), obj1.totalSize());
    BMQTST_ASSERT_EQ(obj.numProperties(), obj1.numProperties());

    PropertyMap pmap1 = pmap;
    verify(&pmap1, obj1);

    // Check assignment
    bmqp::MessageProperties obj2;
    obj2 = obj;

    BMQTST_ASSERT_EQ(obj.totalSize(), obj2.totalSize());
    BMQTST_ASSERT_EQ(obj.numProperties(), obj2.numProperties());

    PropertyMap pmap2 = pmap;
    verify(&pmap2, obj2);

    // Check self assignment
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
    PropertyMap pmap3 = pmap;
    obj2              = obj2;
    verify(&pmap3, obj2);
#pragma clang diagnostic pop
}

static void test10_empty()
{
    // Even if SDK does not send out empty MPS, streaming out/in should work.

    bmqtst::TestHelper::printTestName("'empty MPs' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bmqp::MessageProperties p(bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob wireRep(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::MessagePropertiesInfo logic(true, 1, true);

    // Empty rep.
    p.streamIn(wireRep, logic.isExtended());
    BMQTST_ASSERT_EQ(0, p.numProperties());
    BMQTST_ASSERT_EQ(0, p.totalSize());

    const bdlbb::Blob& out = p.streamOut(&bufferFactory, logic);
    BMQTST_ASSERT_EQ(0, out.length());

    BMQTST_ASSERT(!p.hasProperty("z"));
}

#ifdef BMQTST_BENCHMARK_ENABLED

struct MessagePropertiesBenchmark_getPropertyRef {
    static void bench(bslmt::Latch*   initLatch_p,
                      bslmt::Barrier* startBarrier_p,
                      bslmt::Latch*   finishLatch_p)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(initLatch_p);
        BSLS_ASSERT_OPT(startBarrier_p);
        BSLS_ASSERT_OPT(finishLatch_p);

        bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

        const size_t k_NUM_ITERATIONS = 1000000;

        bdlbb::PooledBlobBufferFactory bufferFactory(1024, alloc);
        bmqp::MessageProperties        obj(alloc);
        bdlbb::Blob                    wireRep(&bufferFactory, alloc);
        PropertyMap                    pmap(alloc);
        bmqp::MessagePropertiesInfo    logic =
            bmqp::MessagePropertiesInfo::makeNoSchema();

        const size_t numProps =
            bmqp::MessagePropertiesHeader::k_MAX_NUM_PROPERTIES;

        // Populate with various properties.

        // Note that `dummyP` is used only because `populateProperties`
        // requires one.
        bmqp::MessageProperties dummyP(alloc);

        populateProperties(&dummyP, &pmap, numProps);

        // Init vector once to allow faster iteration in benchmark
        bsl::vector<bsl::string> propertyNames(alloc);
        for (auto iter = pmap.cbegin(); iter != pmap.cend(); iter++) {
            propertyNames.push_back(iter->first);
        }

        BMQTST_ASSERT_EQ(numProps, pmap.size());

        encode(&wireRep, pmap);

        BMQTST_ASSERT_EQ(0, obj.streamIn(wireRep, logic.isExtended()));

        initLatch_p->arrive();
        startBarrier_p->wait();

        for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
            bmqp::MessageProperties props;
            props.setDeepCopy(false);
            props.streamIn(wireRep, true);

            for (size_t j = 0; j < propertyNames.size(); j++) {
                // Don't use slow `bmqtst::TestHelperUtil::allocator`:
                (void)props.getPropertyRef(propertyNames[j], 0);
            }
        }

        finishLatch_p->arrive();
    }
};

struct MessagePropertiesBenchmark_streamIn {
    static void bench(bslmt::Latch*   initLatch_p,
                      bslmt::Barrier* startBarrier_p,
                      bslmt::Latch*   finishLatch_p)
    {
        // PRECONDITIONS
        BSLS_ASSERT_OPT(initLatch_p);
        BSLS_ASSERT_OPT(startBarrier_p);
        BSLS_ASSERT_OPT(finishLatch_p);

        bslma::Allocator* alloc = bmqtst::TestHelperUtil::allocator();

        const size_t k_NUM_ITERATIONS = 10000000;

        bdlbb::PooledBlobBufferFactory bufferFactory(1024, alloc);
        bmqp::MessageProperties        obj(alloc);
        bdlbb::Blob                    wireRep(&bufferFactory, alloc);
        PropertyMap                    pmap(alloc);
        bmqp::MessagePropertiesInfo    logic =
            bmqp::MessagePropertiesInfo::makeNoSchema();

        const size_t numProps =
            bmqp::MessagePropertiesHeader::k_MAX_NUM_PROPERTIES;

        // Populate with various properties.

        // Note that `dummyP` is used only because `populateProperties`
        // requires one.
        bmqp::MessageProperties dummyP(alloc);

        populateProperties(&dummyP, &pmap, numProps);

        BMQTST_ASSERT_EQ(numProps, pmap.size());

        encode(&wireRep, pmap);

        BMQTST_ASSERT_EQ(0, obj.streamIn(wireRep, logic.isExtended()));

        initLatch_p->arrive();
        startBarrier_p->wait();

        for (size_t i = 0; i < k_NUM_ITERATIONS; ++i) {
            bmqp::MessageProperties props;
            props.setDeepCopy(false);
            props.streamIn(wireRep, true);
        }

        finishLatch_p->arrive();
    }
};

template <size_t NUM_THREADS, typename BENCHMARK>
static void testN1_benchmark(benchmark::State& state)
// ------------------------------------------------------------------------
// MESSAGE PROPERTIES PERFORMANCE TEST
//
// Plan: spawn NUM_THREADS and measure the time taken for BENCHMARK::bench
//
// Testing:
//  Performance
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("MESSAGE PROPERTIES PERFORMANCE TEST");

    bslmt::Latch   initThreadLatch(NUM_THREADS);
    bslmt::Barrier startBenchmarkBarrier(NUM_THREADS + 1);
    bslmt::Latch   finishBenchmarkLatch(NUM_THREADS);

    bslmt::ThreadGroup threadGroup(bmqtst::TestHelperUtil::allocator());
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        const int rc = threadGroup.addThread(
            bdlf::BindUtil::bindS(bmqtst::TestHelperUtil::allocator(),
                                  &(BENCHMARK::bench),
                                  &initThreadLatch,
                                  &startBenchmarkBarrier,
                                  &finishBenchmarkLatch));
        BMQTST_ASSERT_EQ_D(i, rc, 0);
    }

    initThreadLatch.wait();

    size_t iter = 0;
    for (auto _ : state) {
        // Benchmark time start

        // We don't support running multi-iteration benchmarks because we
        // prepare and start complex tasks in separate threads.
        // Once these tasks are finished, we cannot simply re-run them without
        // reinitialization, and it goes against benchmark library design.
        // Make sure we run this only once.
        BSLS_ASSERT_OPT(0 == iter++ && "Must be run only once");

        startBenchmarkBarrier.wait();
        finishBenchmarkLatch.wait();

        // Benchmark time end
    }

    threadGroup.joinAll();
}

#endif  // BMQTST_BENCHMARK_ENABLED

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

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
    case -1: {
#ifdef BMQTST_BENCHMARK_ENABLED
        // Not necessary to check performance in multiple threads
        BENCHMARK(testN1_benchmark<1, MessagePropertiesBenchmark_streamIn>)
            ->Name("bmqp::MessageProperties::streamIn")
            ->Iterations(1)
            ->Repetitions(10)
            ->Unit(benchmark::kMillisecond);

        BENCHMARK(
            testN1_benchmark<1, MessagePropertiesBenchmark_getPropertyRef>)
            ->Name("bmqp::MessageProperties::getPropertyRef")
            ->Iterations(1)
            ->Repetitions(10)
            ->Unit(benchmark::kMillisecond);

        benchmark::Initialize(&argc, argv);
        benchmark::RunSpecifiedBenchmarks();
#else
        cerr << "WARNING: BENCHMARK '" << _testCase
             << "' IS NOT SUPPORTED ON THIS PLATFORM." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
#endif  // BMQTST_BENCHMARK_ENABLED
    } break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);

    // Check for default allocator is explicitly disabled as
    // 'bmqp::MessageProperties' or one of its data members may allocate
    // temporaries with default allocator.
}
