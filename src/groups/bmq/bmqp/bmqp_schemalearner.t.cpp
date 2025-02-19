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

// bmqp_schemalearner.t.cpp                                           -*-C++-*-

// BMQ
#include <bmqp_messageproperties.h>
#include <bmqp_protocolutil.h>
#include <bmqp_schemalearner.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_random.h>
#include <bdlbb_pooledblobbufferfactory.h>
#include <bslstl_map.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_multiplexingTest()
{
    // Two distinct sources translate the same id ('1').  The result must be
    // unique id in each case.
    //
    bmqp::SchemaLearner theLearner(bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner::Context clientSession1 = theLearner.createContext();
    bmqp::SchemaLearner::Context clientSession2 = theLearner.createContext();

    BMQTST_ASSERT_NE(clientSession1, clientSession2);

    {
        // Verify that invalid schema (new-style-no-schema) does not translate.
        bmqp::MessagePropertiesInfo in =
            bmqp::MessagePropertiesInfo::makeInvalidSchema();
        bmqp::MessagePropertiesInfo out = theLearner.multiplex(clientSession1,
                                                               in);

        BMQTST_ASSERT(in == out);
    }

    bmqp::MessagePropertiesInfo info(true, 1, false);

    bmqp::MessagePropertiesInfo translated1 =
        theLearner.multiplex(clientSession1, info);
    bmqp::MessagePropertiesInfo translated2 =
        theLearner.multiplex(clientSession2, info);

    BMQTST_ASSERT_NE(translated1.schemaId(), translated2.schemaId());

    // Subsequent calls get the same result
    BMQTST_ASSERT_EQ(translated1.schemaId(),
                     theLearner.multiplex(clientSession1, info).schemaId());

    BMQTST_ASSERT_EQ(translated2.schemaId(),
                     theLearner.multiplex(clientSession2, info).schemaId());
}

static void test2_readingTest()
{
    // Simulate ClientSession receiving, translating, and reading MPs
    // in Broker's context.
    // The resulting schema must be the same until recycling is indicated.
    //

    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner theLearner(bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner::Context   queueEngine(theLearner.createContext());
    bmqp::SchemaLearner::Context   clientSession(theLearner.createContext());
    bmqp::MessageProperties        in(bmqtst::TestHelperUtil::allocator());
    bmqp::MessagePropertiesInfo    input(true, 1, false);
    bmqp::MessagePropertiesInfo    recycledInput(true, 1, true);

    const int num = bmqp::MessageProperties::k_MAX_NUM_PROPERTIES;

    for (int i = 0; i < num; ++i) {
        bsl::string name = bsl::to_string(i);
        BMQTST_ASSERT_EQ(0, in.setPropertyAsString(name, name));
    }

    const bdlbb::Blob       blob = in.streamOut(&bufferFactory, input);
    bmqp::MessageProperties out(bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(
        0,
        theLearner.read(queueEngine,
                        &out,
                        theLearner.multiplex(clientSession, input),
                        blob));

    bmqp::MessageProperties::SchemaPtr schema1 = out.makeSchema(
        bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(
        0,
        theLearner.read(queueEngine,
                        &out,
                        theLearner.multiplex(clientSession, input),
                        blob));

    BMQTST_ASSERT_EQ(schema1,
                     out.makeSchema(bmqtst::TestHelperUtil::allocator()));
    // subsequent call returns the same Schema

    int start = bsl::rand() % num;

    for (int i = 0; i < num; ++i) {
        int j = i + start;
        if (j >= num) {
            j -= num;
        }
        bsl::string name = bsl::to_string(j);

        BMQTST_ASSERT_EQ(out.getPropertyAsString(name), name);
    }

    BMQTST_ASSERT_EQ(
        0,
        theLearner.read(queueEngine,
                        &out,
                        theLearner.multiplex(clientSession, recycledInput),
                        blob));

    bmqp::MessageProperties::SchemaPtr schema2;
    schema2 = out.makeSchema(bmqtst::TestHelperUtil::allocator());
    BMQTST_ASSERT_NE(schema1, schema2);
    // ...unless the input is recycled

    BMQTST_ASSERT(schema1);
    BMQTST_ASSERT(schema2);

    int index;

    for (int i = 0; i < num; ++i) {
        bsl::string name = bsl::to_string(i);

        BMQTST_ASSERT(schema1->loadIndex(&index, name));
    }
    BMQTST_ASSERT(!schema1->loadIndex(&index, "a"));

    for (int i = 0; i < num; ++i) {
        bsl::string name = bsl::to_string(i);

        BMQTST_ASSERT(schema2->loadIndex(&index, name));
    }

    BMQTST_ASSERT(!schema2->loadIndex(&index, "a"));
}

static void test3_observingTest()
{
    // While reading the same MPs in the same context, the result must be the
    // same until observing recycling indication.
    //
    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner theLearner(bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner::Context   server(theLearner.createContext());

    bmqp::MessageProperties     in(bmqtst::TestHelperUtil::allocator());
    bmqp::MessagePropertiesInfo input(true, 1, false);

    in.setPropertyAsString("z", "z");
    in.setPropertyAsString("y", "y");
    in.setPropertyAsString("x", "x");

    const bdlbb::Blob       blob = in.streamOut(&bufferFactory, input);
    bmqp::MessageProperties out1(bmqtst::TestHelperUtil::allocator());
    bmqp::MessageProperties out2(bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(0, theLearner.read(server, &out1, input, blob));
    BMQTST_ASSERT_EQ(0, theLearner.read(server, &out2, input, blob));

    bmqp::MessageProperties::SchemaPtr schema1 = out1.makeSchema(
        bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(schema1,
                     out2.makeSchema(bmqtst::TestHelperUtil::allocator()));
    // subsequent call returns the same Schema

    BMQTST_ASSERT_EQ(0, theLearner.read(server, &out2, input, blob));

    BMQTST_ASSERT_EQ(schema1,
                     out2.makeSchema(bmqtst::TestHelperUtil::allocator()));
    // subsequent call returns the same Schema

    bmqp::MessagePropertiesInfo recycledInput(true, 1, true);

    theLearner.learn(server, recycledInput, bdlbb::Blob());

    BMQTST_ASSERT_EQ(0, theLearner.read(server, &out2, input, blob));

    BMQTST_ASSERT_NE(schema1,
                     out2.makeSchema(bmqtst::TestHelperUtil::allocator()));
    // ...unless the input is recycled
}

static void test4_demultiplexingTest()
{
    // Demultiplexing (PUSH) indicates recycling first, then no recycling until
    // multiplexing (PUT) indicates recycling.

    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner theLearner(bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner::Context   queueHandle(theLearner.createContext());

    bmqp::MessagePropertiesInfo muxIn(true, 1, false);
    bmqp::MessagePropertiesInfo recycledMuxIn(true, 1, true);

    bmqp::MessagePropertiesInfo demuxOut;

    demuxOut = theLearner.demultiplex(queueHandle, muxIn);
    BMQTST_ASSERT(demuxOut.isRecycled());
    BMQTST_ASSERT_EQ(muxIn.schemaId(), demuxOut.schemaId());

    demuxOut = theLearner.demultiplex(queueHandle, muxIn);

    BMQTST_ASSERT(!demuxOut.isRecycled());
    BMQTST_ASSERT_EQ(muxIn.schemaId(), demuxOut.schemaId());

    demuxOut = theLearner.demultiplex(queueHandle, recycledMuxIn);

    BMQTST_ASSERT(demuxOut.isRecycled());
    BMQTST_ASSERT_EQ(muxIn.schemaId(), demuxOut.schemaId());
}

static void test5_emptyMPs()
{
    // Even if SDK does not send out empty MPS, streaming out/in should work.

    bmqtst::TestHelper::printTestName("'empty MPs' TEST");

    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner theLearner(bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner::Context   context(theLearner.createContext());

    bmqp::MessageProperties p(bmqtst::TestHelperUtil::allocator());
    bdlbb::Blob wireRep(&bufferFactory, bmqtst::TestHelperUtil::allocator());
    bmqp::MessagePropertiesInfo logic(true, 1, true);

    // Empty rep.
    BMQTST_ASSERT_EQ(0, theLearner.read(context, &p, logic, wireRep));

    BMQTST_ASSERT_EQ(0, p.numProperties());
    BMQTST_ASSERT_EQ(0, p.totalSize());

    const bdlbb::Blob& out = p.streamOut(&bufferFactory, logic);
    BMQTST_ASSERT_EQ(0, out.length());

    BMQTST_ASSERT(!p.hasProperty("z"));
}

static void test6_partialRead()
{
    // Read known schema partially.  Change one property and then continue
    // reading.
    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner theLearner(bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner::Context   context(theLearner.createContext());

    bmqp::MessageProperties     in(bmqtst::TestHelperUtil::allocator());
    bmqp::MessagePropertiesInfo input(true, 1, false);
    const char                  x[]   = "x";
    const char                  y[]   = "y";
    const char                  z[]   = "z";
    const char                  mod[] = "mod";

    in.setPropertyAsString("z", z);
    in.setPropertyAsString("y", y);
    in.setPropertyAsString("x", x);

    const bdlbb::Blob       blob = in.streamOut(&bufferFactory, input);
    bmqp::MessageProperties out1(bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(0, theLearner.read(context, &out1, input, blob));

    // 1st setProperty w/o getProperty and then getProperty
    {
        bmqp::MessageProperties out2(bmqtst::TestHelperUtil::allocator());

        // The second read is optimized (only one MPS header)
        BMQTST_ASSERT_EQ(0, theLearner.read(context, &out2, input, blob));

        BMQTST_ASSERT_EQ(0, out2.setPropertyAsString("y", mod));
        BMQTST_ASSERT_EQ(out1.totalSize() + sizeof(mod) - sizeof(y),
                         out2.totalSize());

        BMQTST_ASSERT_EQ(out2.getPropertyAsString("z"), z);
    }

    // 2nd getProperty, setProperty and then load all
    {
        bmqp::MessageProperties out3(bmqtst::TestHelperUtil::allocator());

        // The third read is optimized (only one MPS header)
        BMQTST_ASSERT_EQ(0, theLearner.read(context, &out3, input, blob));

        BMQTST_ASSERT_EQ(y, out3.getPropertyAsString("y"));
        BMQTST_ASSERT_EQ(0, out3.setPropertyAsString("y", mod));

        bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
        out3.print(os, 0, -1);

        PV(os.str());

        bmqp::MessagePropertiesIterator it(&out3);

        BMQTST_ASSERT(it.hasNext());
        BMQTST_ASSERT_EQ(it.getAsString(), x);
        BMQTST_ASSERT(it.hasNext());
        BMQTST_ASSERT_EQ(it.getAsString(), mod);
        BMQTST_ASSERT(it.hasNext());
        BMQTST_ASSERT_EQ(it.getAsString(), z);
    }

    // 3rd getProperty, setProperty and then getProperty
    {
        bmqp::MessageProperties out4(bmqtst::TestHelperUtil::allocator());

        // The fourth read is optimized (only one MPS header)
        BMQTST_ASSERT_EQ(0, theLearner.read(context, &out4, input, blob));

        BMQTST_ASSERT_EQ(y, out4.getPropertyAsString("y"));
        BMQTST_ASSERT_EQ(0, out4.setPropertyAsString("y", mod));
        BMQTST_ASSERT_EQ(out1.totalSize() + sizeof(mod) - sizeof(y),
                         out4.totalSize());

        BMQTST_ASSERT_EQ(out4.getPropertyAsString("z"), z);
    }
}

static void test7_removeBeforeRead()
{
    // Read known schema partially.  Remove one property and then continue
    // reading.
    bdlbb::PooledBlobBufferFactory bufferFactory(
        128,
        bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner theLearner(bmqtst::TestHelperUtil::allocator());
    bmqp::SchemaLearner::Context   context(theLearner.createContext());

    bmqp::MessageProperties     in(bmqtst::TestHelperUtil::allocator());
    bmqp::MessagePropertiesInfo input(true, 1, false);

    const int   numProps       = 3;
    const char* name[numProps] = {"x", "y", "z"};
    const char  mod[]          = "mod";

    for (int iProperty = 0; iProperty < numProps; ++iProperty) {
        in.setPropertyAsString(name[iProperty], name[iProperty]);
    }

    const bdlbb::Blob       blob = in.streamOut(&bufferFactory, input);
    bmqp::MessageProperties out1(bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(0, theLearner.read(context, &out1, input, blob));

    for (int iProperty = 0; iProperty < numProps; ++iProperty) {
        const char* current = name[iProperty];
        for (int iScenario = 0; iScenario < 3; ++iScenario) {
            bmqp::MessageProperties out2(bmqtst::TestHelperUtil::allocator());

            // All subsequent reads are optimized (only one MPS header)
            BMQTST_ASSERT_EQ(0, theLearner.read(context, &out2, input, blob));
            if (iScenario) {
                // read before remove
                BMQTST_ASSERT_EQ(name[iProperty],
                                 out2.getPropertyAsString(current));

                if (iScenario == 2) {
                    // modify before remove
                    BMQTST_ASSERT_EQ(0,
                                     out2.setPropertyAsString(current, mod));
                    BMQTST_ASSERT_EQ(out1.totalSize() + bsl::strlen(mod) -
                                         bsl::strlen(current),
                                     out2.totalSize());
                }
            }
            BMQTST_ASSERT(out2.remove(current));
            BMQTST_ASSERT_EQ(out1.totalSize() -
                                 sizeof(bmqp::MessagePropertyHeader) -
                                 bsl::strlen(current) - bsl::strlen(current),
                             out2.totalSize());

            BMQTST_ASSERT(!out2.hasProperty(current));

            for (int i = 0; i < numProps; ++i) {
                if (i == iProperty) {
                    BMQTST_ASSERT(!out2.hasProperty(name[i]));
                }
                else {
                    BMQTST_ASSERT_EQ(out2.getPropertyAsString(name[i]),
                                     name[i]);
                }
            }

            bmqu::MemOutStream os(bmqtst::TestHelperUtil::allocator());
            out2.print(os, 0, -1);

            PV(os.str());

            {
                bmqp::MessagePropertiesIterator it(&out2);

                BMQTST_ASSERT(it.hasNext());
                BMQTST_ASSERT(it.hasNext());
                BMQTST_ASSERT(!it.hasNext());
            }

            // Add the property back
            BMQTST_ASSERT_EQ(0, out2.setPropertyAsString(current, mod));

            BMQTST_ASSERT_EQ(out1.totalSize() + bsl::strlen(mod) -
                                 bsl::strlen(current),
                             out2.totalSize());

            for (int i = 0; i < numProps; ++i) {
                if (i == iProperty) {
                    BMQTST_ASSERT_EQ(out2.getPropertyAsString(name[i]), mod);
                }
                else {
                    BMQTST_ASSERT_EQ(out2.getPropertyAsString(name[i]),
                                     name[i]);
                }
            }

            {
                bmqp::MessagePropertiesIterator it(&out2);

                BMQTST_ASSERT(it.hasNext());
                BMQTST_ASSERT(it.hasNext());
                BMQTST_ASSERT(it.hasNext());
                BMQTST_ASSERT(!it.hasNext());
            }
        }
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);
    bmqp::ProtocolUtil::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 7: test7_removeBeforeRead(); break;
    case 6: test6_partialRead(); break;
    case 5: test5_emptyMPs(); break;
    case 4: test4_demultiplexingTest(); break;
    case 3: test3_observingTest(); break;
    case 2: test2_readingTest(); break;
    case 1: test1_multiplexingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqp::ProtocolUtil::shutdown();
    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
