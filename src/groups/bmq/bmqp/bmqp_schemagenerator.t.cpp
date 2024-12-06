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

// bmqp_schemagenerator.t.cpp                                         -*-C++-*-

// BMQ
#include <bmqp_schemagenerator.h>

// BDE
#include <bdlb_random.h>
#include <bslstl_map.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

typedef bmqp::MessagePropertiesInfo::SchemaIdType SchemaIdType;

const SchemaIdType NO_SCHEMA = bmqp::MessagePropertiesInfo::k_NO_SCHEMA;

/// instead of `bmqp::MessagePropertiesHeader::k_MAX_SCHEMA`
const SchemaIdType MAX_SCHEMA = 100;

/// instead of `bmqp::MessageProperties::k_MAX_NUM_PROPERTIES`
const int NUM_PROPERTIES = 10;

/// n + (n - 1) + (n - 2) + .. + 2 + 1 == n * (n + 1) /2
const int NUM_COMBINATIONS = NUM_PROPERTIES * (NUM_PROPERTIES + 1) / 2;

/// (lap, combination) pair identifies unique Properties sequence in the
/// range exceeding MAX_SCHEMA (to test recycling).
const int NUM_LAPS = 3;

const int NUM_RECYCLED = NUM_COMBINATIONS * NUM_LAPS - MAX_SCHEMA;

// keep these on heap rather than on stack
static int length[NUM_COMBINATIONS];

/// shuffled integers in the range [0..NUM_COMBINATIONS)
static int sequence[NUM_COMBINATIONS];

static void generateMessageProperties(bmqp::MessageProperties* mps,
                                      int                      lap,
                                      int                      combination)
{
    // (test, combination) pair identifies unique Properties sequence.
    // Randomize calling 'setPropertyAsString'
    // Note that MessageProperties::d_properties is a map (not a HT)

    int property = bsl::rand() % length[combination];
    for (int l = 0; l < length[combination]; ++l) {
        bsl::string name("property_", bmqtst::TestHelperUtil::allocator());
        name += bsl::to_string(lap);
        name += "_";
        name += bsl::to_string(sequence[combination] + property);

        BMQTST_ASSERT_EQ(0, mps->setPropertyAsString(name, name));

        if (++property == length[combination]) {
            property = 0;
        }
    }
}

static void test1_breathingTest()
{
    int                         count = 0;
    bsl::map<SchemaIdType, int> ids(bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaGenerator theGenerator(bmqtst::TestHelperUtil::allocator());

    theGenerator._setCapacity(MAX_SCHEMA);

    // generate all possible sequences of 'NUM_PROPERTIES' with lengths ranging
    // from '1' to 'NUM_PROPERTIES'
    for (int start = 0; start < NUM_PROPERTIES; ++start) {
        // 'start' of next sequence
        for (int len = 1; len <= (NUM_PROPERTIES - start); ++len, ++count) {
            // 'length' of next sequence
            length[count] = len;
        }
    }
    BSLS_ASSERT_SAFE(count == NUM_COMBINATIONS);

    // So far, have not reached the NUM_SEQUENCES, because of '255' limit of
    // k_MAX_NUM_PROPERTIES.
    // Use two 'MessageProperties' to over-reach 'k_MAX_SCHEMA'
    BSLS_ASSERT_SAFE(NUM_LAPS * count > MAX_SCHEMA + 33);

    for (int i = 0; i < NUM_COMBINATIONS; ++i) {
        sequence[i] = i;
    }

    // Shuffle by swapping two random properties (NUM_SEQUENCES / 2) times
    for (int i = NUM_COMBINATIONS >> 1; i > 0; --i) {
        int i1  = bsl::rand() % NUM_COMBINATIONS;
        int i2  = bsl::rand() % NUM_COMBINATIONS;
        int seq = sequence[i1];
        int len = length[i1];

        sequence[i1] = sequence[i2];
        sequence[i2] = seq;
        length[i1]   = length[i2];
        length[i2]   = len;
    }
    count = 0;
    // Use three 'MessageProperties' (there is the limit of 255)
    for (int lap = 0; lap < NUM_LAPS; ++lap) {
        // Up till 'k_MAX_SCHEMA' everything is unique (one time only)
        for (int i = 0; i < NUM_COMBINATIONS; ++i, ++count) {
            BSLS_ASSERT_SAFE(length[i]);

            bmqp::MessageProperties mps(bmqtst::TestHelperUtil::allocator());
            // For example, given {a, b, c} properties:
            //  [a]
            //  [a, b]
            //  [a, b, c]
            //  [b]
            //  [b, c]
            //  [c]

            generateMessageProperties(&mps, lap, i);

            bmqp::MessagePropertiesInfo logic = theGenerator.getSchemaId(&mps);
            SchemaIdType                schemaId = logic.schemaId();

            BMQTST_ASSERT_LE(schemaId, MAX_SCHEMA);
            BMQTST_ASSERT_NE(schemaId, NO_SCHEMA);
            ids[schemaId] = count;

            BMQTST_ASSERT(logic.isRecycled());  // either first use or recycled

            // Call 'getSchema' again
            logic = theGenerator.getSchemaId(&mps);
            BMQTST_ASSERT_EQ(schemaId, logic.schemaId());
            BMQTST_ASSERT(!logic.isRecycled());  // second use
        }
    }
    // Some number of first test combinations (65) got recycled.
    // Repeat them again and verify, they got new IDs.
    for (int i = 0; i < 33; ++i) {
        bmqp::MessageProperties mps(bmqtst::TestHelperUtil::allocator());
        generateMessageProperties(&mps, 0, i);
        // Sequence numbering starts from '0', schema's - from '1'

        // 'i'              is a sequence of MPs
        // 'i + 1'          is the expected id
        // 'schema1.d_id'   is post-recycling id
        // 'ids[i + 1]'     is the last sequence referenced by 'i + 1'

        BMQTST_ASSERT_NE(ids[i + 1], i);  // 'i + 1' was recycled

        bmqp::MessagePropertiesInfo logic = theGenerator.getSchemaId(&mps);
        BMQTST_ASSERT(logic.isRecycled());
        // We continue recycling
        BMQTST_ASSERT_EQ(logic.schemaId(), SchemaIdType(i + 1 + NUM_RECYCLED));

        // Call again to make number of hits equal to the rest (2)
        logic = theGenerator.getSchemaId(&mps);
        BMQTST_ASSERT(!logic.isRecycled());
        // We continue recycling
        BMQTST_ASSERT_EQ(logic.schemaId(), SchemaIdType(i + 1 + NUM_RECYCLED));
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
