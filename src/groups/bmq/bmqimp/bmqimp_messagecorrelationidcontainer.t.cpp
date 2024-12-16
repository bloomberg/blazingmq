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

// bmqimp_messagecorrelationidcontainer.t.cpp                         -*-C++-*-
#include <bmqimp_messagecorrelationidcontainer.h>

// BMQ
#include <bmqimp_queue.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqt_correlationid.h>
#include <bmqt_messageguid.h>

// BDE
#include <bdlf_bind.h>
#include <bsl_functional.h>
#include <bsl_unordered_map.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

typedef bmqimp::MessageCorrelationIdContainer::QueueAndCorrelationId QAC;
typedef bmqimp::MessageCorrelationIdContainer::KeyIdsCb              Callback;
typedef bsl::unordered_map<bmqt::MessageGUID, QAC> CorrelationIdsMap;

/// Provide a helper mechanism to test the `iterateAndInvoke` method of the
/// `bmqimp::MessageCorrelationIdContainer`
struct IterateAndInvokeHelper {
    CorrelationIdsMap d_corrIdMap;  // Internal map that is used to copy
                                    // all the keys-value pairs from the
                                    // messageCorrelationIdContainer into
                                    // it. It accurately reflects the
                                    // internals of the
                                    // messageCorrelationIdContainer only
                                    // when 'iterateAndInvoke' has been
                                    // invoked just before accessing it.

    IterateAndInvokeHelper(bslma::Allocator* allocator)
    : d_corrIdMap(allocator)
    {
        // NOTHING
    }

    /// Inserts the specified `handle` and `qac` pair into the map of this
    /// object.  The specified `deleteVisitedItem` relates to the item with
    /// the specified `handle`.  It is always set to `false` in this visitor
    /// implementation.
    bool insertIntoMap(bool*                   deleteVisitedItem,
                       const bmqt::MessageGUID handle,
                       const QAC&              qac)
    {
        d_corrIdMap[handle] = qac;
        *deleteVisitedItem  = false;

        return false;  // do not interrupt
    }
};

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_addFindRemove()
{
    bmqtst::TestHelper::printTestName("ADD FIND REMOVE");

    bmqimp::MessageCorrelationIdContainer container(
        bmqtst::TestHelperUtil::allocator());

    bmqt::MessageGUID guid = bmqp::MessageGUIDGenerator::testGUID();
    bmqt::MessageGUID emptyGuid;

    container.add(guid, bmqt::CorrelationId(1), bmqp::QueueId(1));

    {
        PVV("Find");
        bmqt::CorrelationId corrId;
        BMQTST_ASSERT_EQ(container.find(&corrId, guid), 0);
        BMQTST_ASSERT_EQ(corrId, bmqt::CorrelationId(1));
    }

    {
        PVV("Negative find");
        bmqt::CorrelationId corrId;
        BMQTST_ASSERT_NE(container.find(&corrId, emptyGuid), 0);
    }

    {
        PVV("Remove");
        bmqt::CorrelationId corrId;
        BMQTST_ASSERT_EQ(container.remove(guid, &corrId), 0);
        BMQTST_ASSERT_EQ(corrId, bmqt::CorrelationId(1));
        BMQTST_ASSERT_EQ(container.size(), 0U);

        BMQTST_ASSERT_NE(container.remove(guid, &corrId), 0);
    }

    {
        PVV("Rewriting add");
        container.add(guid, bmqt::CorrelationId(2), bmqp::QueueId(1));
        container.add(guid, bmqt::CorrelationId(2), bmqp::QueueId(1));
        BMQTST_ASSERT_EQ(container.size(), 1U);
    }

    {
        PVV("Repeated add");
        container.add(guid, bmqt::CorrelationId(2), bmqp::QueueId(1));
        container.add(emptyGuid, bmqt::CorrelationId(2), bmqp::QueueId(1));
        BMQTST_ASSERT_EQ(container.size(), 2U);
    }

    {
        PVV("Clear");
        container.reset();
        BMQTST_ASSERT_EQ(container.size(), 0U);
    }
}

static void test2_iterateAndInvoke()
{
    bmqtst::TestHelper::printTestName("ITERATE AND INVOKE");

    bmqimp::MessageCorrelationIdContainer container(
        bmqtst::TestHelperUtil::allocator());
    IterateAndInvokeHelper helper(bmqtst::TestHelperUtil::allocator());

    bmqt::MessageGUID guid1 = bmqp::MessageGUIDGenerator::testGUID();
    bmqt::MessageGUID guid2 = bmqp::MessageGUIDGenerator::testGUID();

    Callback callback = bdlf::BindUtil::bind(
        &IterateAndInvokeHelper::insertIntoMap,
        &helper,
        bdlf::PlaceHolders::_1,
        bdlf::PlaceHolders::_2,
        bdlf::PlaceHolders::_3);

    // Insert into container
    container.add(guid1, bmqt::CorrelationId(1), bmqp::QueueId(1));
    container.add(guid2, bmqt::CorrelationId(2), bmqp::QueueId(2));

    // Recreate the map to verify that the internals of the container are
    // correct and that iterateAndInvoke is invoked correctly.
    container.iterateAndInvoke(callback);

    BMQTST_ASSERT_EQ(container.size(), helper.d_corrIdMap.size());
    BMQTST_ASSERT_EQ(container.size(), 2U);

    BMQTST_ASSERT_EQ(helper.d_corrIdMap[guid1].d_correlationId,
                     bmqt::CorrelationId(1));
    BMQTST_ASSERT_EQ(helper.d_corrIdMap[guid1].d_queueId, bmqp::QueueId(1));

    BMQTST_ASSERT_EQ(helper.d_corrIdMap[guid2].d_correlationId,
                     bmqt::CorrelationId(2));
    BMQTST_ASSERT_EQ(helper.d_corrIdMap[guid2].d_queueId, bmqp::QueueId(2));
}

static void test3_associate()
{
    bmqtst::TestHelper::printTestName("ASSOCIATE");

    bmqimp::MessageCorrelationIdContainer container(
        bmqtst::TestHelperUtil::allocator());
    IterateAndInvokeHelper helper(bmqtst::TestHelperUtil::allocator());

    bmqt::MessageGUID guid1 = bmqp::MessageGUIDGenerator::testGUID();
    bmqt::MessageGUID guid2 = bmqp::MessageGUIDGenerator::testGUID();
    bmqt::MessageGUID emptyGuid;

    Callback callback = bdlf::BindUtil::bind(
        &IterateAndInvokeHelper::insertIntoMap,
        &helper,
        bdlf::PlaceHolders::_1,
        bdlf::PlaceHolders::_2,
        bdlf::PlaceHolders::_3);

    // Insert into container
    container.add(guid1, bmqt::CorrelationId(1), bmqp::QueueId(1));
    container.add(guid2,
                  bmqt::CorrelationId(2),
                  bmqp::QueueId(bmqimp::Queue::k_INVALID_QUEUE_ID));

    {
        PVV("Successful associate");
        container.iterateAndInvoke(callback);

        BMQTST_ASSERT_EQ(container.size(), helper.d_corrIdMap.size());
        BMQTST_ASSERT_EQ(container.size(), 2U);

        BMQTST_ASSERT_EQ(helper.d_corrIdMap[guid1].d_correlationId,
                         bmqt::CorrelationId(1));
        BMQTST_ASSERT_EQ(helper.d_corrIdMap[guid1].d_queueId,
                         bmqp::QueueId(1));

        BMQTST_ASSERT_EQ(helper.d_corrIdMap[guid2].d_queueId.id(),
                         bmqimp::Queue::k_INVALID_QUEUE_ID);
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
    case 3: test3_associate(); break;
    case 2: test2_iterateAndInvoke(); break;
    case 1: test1_addFindRemove(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
