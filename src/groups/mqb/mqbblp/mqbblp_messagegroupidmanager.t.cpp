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

// mqbblp_messagegroupidmanager.t.cpp                                 -*-C++-*-

#include <mqbblp_messagegroupidmanager.h>

// MQB
#include <mqbcmd_humanprinter.h>
#include <mqbcmd_messages.h>
#include <mqbi_dispatcher.h>
#include <mqbi_queue.h>
#include <mqbmock_queue.h>
#include <mqbmock_queuehandle.h>

#include <bmqu_memoutstream.h>

// BDE
#include <bdlma_localsequentialallocator.h>
#include <bsl_algorithm.h>
#include <bsl_cstdlib.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bslma_managedptr.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;
using namespace mqbblp;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

typedef MessageGroupIdManager::MsgGroupId MsgGroupId;
typedef MessageGroupIdManager::Handle     Handle;
typedef MessageGroupIdManager::Time       Time;

const Time k_TIMEOUT(100);
const int  k_MAX_NUMBER_OF_MAPPINGS(-1);
const Time k_T0(10);

// Declared as pointers to avoid 'exit-time destructor' warning.  Created as
// stack variables in main.
const MsgGroupId* k_GID1;
const MsgGroupId* k_GID2;
const MsgGroupId* k_GID3;
const MsgGroupId* k_GID4;

Handle _(const int handle)
{
    return reinterpret_cast<Handle>(handle);
}

MsgGroupId msgGroupIdFromInt(const int id)
{
    bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
    ss << id;
    return MsgGroupId(ss.str(), bmqtst::TestHelperUtil::allocator());
}

void addHandles(MessageGroupIdManager* obj, const int handles = 1000)
{
    BSLS_ASSERT_SAFE(obj);

    for (int i = 0; i < handles; ++i) {
        obj->addHandle(_(i), k_T0);
    }
}

void allocateSomeGroups(MessageGroupIdManager* obj,
                        const int              handles,
                        const int              groups)
{
    typedef MessageGroupIdManager::IdsForHandle IdsForHandle;

    BSLS_ASSERT_SAFE(obj);

    // Simplifies the test
    BSLS_ASSERT_SAFE((groups % handles) == 0);

    // Add a few handles
    bsl::vector<Handle> handlesBefore(handles,
                                      bmqtst::TestHelperUtil::allocator());
    for (int i = 0; i < groups; ++i) {
        const Handle h = obj->getHandle(msgGroupIdFromInt(i), k_T0);
        if (i < handles) {
            handlesBefore[i] = h;
        }
        else {
            // Confirm they wrap as expected
            BMQTST_ASSERT_EQ(h, handlesBefore[i % handles]);
        }
    }

    // We expect 'groups' equally assigned to 'handles' handles.
    const int groupsPerHandle = groups / handles;
    for (int i = 0; i < handles; ++i) {
        IdsForHandle gids(bmqtst::TestHelperUtil::allocator());
        obj->idsForHandle(&gids, _(i));
        BMQTST_ASSERT_EQ(static_cast<int>(gids.size()), groupsPerHandle);
        for (IdsForHandle::const_iterator j = gids.begin(); j != gids.end();
             ++j) {
            const int msgGroupId = ::atoi(j->c_str());
            BMQTST_ASSERT_EQ(msgGroupId % handles, i);
        }
    }
}

class MyDispatcherClient : public mqbi::DispatcherClient {
  private:
    bsl::string                d_description;
    mqbi::DispatcherClientData d_dispatcherClientData;

  public:
    MyDispatcherClient(const bsl::string& description)
    : d_description(description)
    {
    }

    ~MyDispatcherClient() BSLS_KEYWORD_OVERRIDE {}

    mqbi::Dispatcher* dispatcher() BSLS_KEYWORD_OVERRIDE { return 0; }

    mqbi::DispatcherClientData& dispatcherClientData() BSLS_KEYWORD_OVERRIDE
    {
        return d_dispatcherClientData;
    }

    void onDispatcherEvent(BSLS_ANNOTATION_UNUSED const mqbi::DispatcherEvent&
                               event) BSLS_KEYWORD_OVERRIDE
    {
    }

    void flush() BSLS_KEYWORD_OVERRIDE {}

    const mqbi::Dispatcher* dispatcher() const BSLS_KEYWORD_OVERRIDE
    {
        return 0;
    }

    const mqbi::DispatcherClientData&
    dispatcherClientData() const BSLS_KEYWORD_OVERRIDE
    {
        return d_dispatcherClientData;
    }

    const bsl::string& description() const BSLS_KEYWORD_OVERRIDE
    {
        return d_description;
    }
};

bslma::ManagedPtr<mqbi::QueueHandle> queueHandle(
    const bsl::shared_ptr<mqbi::QueueHandleRequesterContext>& clientContext,
    const bsl::shared_ptr<mqbi::Queue>&                       queue,
    bslma::Allocator*                                         allocator_p)
{
    return bslma::ManagedPtr<mqbi::QueueHandle>(
        new (*allocator_p) mqbmock::QueueHandle(
            queue,                                  // queue
            clientContext,                          // clientContext
            0,                                      // domainStats
            bmqp_ctrlmsg::QueueHandleParameters(),  // handleParameters
            allocator_p)                            // allocator
        ,
        allocator_p);
};

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_sameGroupIdSameHandleTest()
// ------------------------------------------------------------------------
// SAME GROUP ID SAME HANDLE TEST
//
// Concerns:
//   'getHandle()' should return the same handle for the same
//   Message Group Id
//
// Plan:
//   Create the conditions and assert the behavior
//
// Testing:
//   getHandle
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("SAME GROUP ID SAME HANDLE TEST");

    MessageGroupIdManager obj(k_TIMEOUT,
                              k_MAX_NUMBER_OF_MAPPINGS,
                              MessageGroupIdManager::k_REBALANCE_OFF,
                              bmqtst::TestHelperUtil::allocator());
    const int             k_NUMBER_OF_HANDLES = 5;
    addHandles(&obj, k_NUMBER_OF_HANDLES);

    // Verify accessors
    BMQTST_ASSERT_EQ(obj.timeout(), k_TIMEOUT);
    BMQTST_ASSERT_EQ(obj.handlesCount(), k_NUMBER_OF_HANDLES);
    BMQTST_ASSERT_EQ(obj.maxMsgGroupIds(), k_MAX_NUMBER_OF_MAPPINGS);

    const Handle h1  = obj.getHandle(*k_GID1, k_T0);
    const Handle h1b = obj.getHandle(*k_GID1, k_T0);

    BMQTST_ASSERT_EQ(h1, h1b);
}

static void test2_differentGroupIdsHaveDifferentHandlesTest()
// ------------------------------------------------------------------------
// DIFFERENT GROUP IDS HAVE DIFFERENT HANDLES TEST
//
// Concerns:
//   'getHandle()' should return different handles for different Message
//   Group Ids
//
// Plan:
//   Create the conditions and assert the behavior
//
// Testing:
//   getHandle
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "DIFFERENT GROUP IDS HAVE DIFFERENT HANDLES TEST");

    MessageGroupIdManager obj(k_TIMEOUT,
                              k_MAX_NUMBER_OF_MAPPINGS,
                              MessageGroupIdManager::k_REBALANCE_OFF,
                              bmqtst::TestHelperUtil::allocator());
    addHandles(&obj);

    const Handle h1 = obj.getHandle(*k_GID1, k_T0);
    const Handle h2 = obj.getHandle(*k_GID2, k_T0);

    BMQTST_ASSERT_NE(h1, h2);
}

static void test3_timeoutOldMappingsTest()
// ------------------------------------------------------------------------
// TIMEOUT OLD MAPPINGS TEST
//
// Concerns:
//   'getHandle()' should timeout mappings older than 'k_TIMEOUT'
//
// Plan:
//   Create the conditions where Message Group Ids GID1 and GID2 have both
//   expired.  Re-allocating GID2 should then be allocated to the first
//   free handle i.e. what used to be the handle for GID1.  After that and
//   before further timeout, the handle for GID2 should remain the same
//
// Testing:
//   getHandle
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("TIMEOUT OLD MAPPINGS TEST");

    MessageGroupIdManager obj(k_TIMEOUT,
                              k_MAX_NUMBER_OF_MAPPINGS,
                              MessageGroupIdManager::k_REBALANCE_OFF,
                              bmqtst::TestHelperUtil::allocator());
    addHandles(&obj);

    const Handle h1 = obj.getHandle(*k_GID1, k_T0);
    const Handle h2 = obj.getHandle(*k_GID2, k_T0);
    const Handle h3 = obj.getHandle(*k_GID2, k_T0 + k_TIMEOUT);
    const Handle h4 = obj.getHandle(*k_GID2, k_T0 + k_TIMEOUT + 1);

    BMQTST_ASSERT_NE(h1, h2);
    BMQTST_ASSERT_NE(h2, h3);
    BMQTST_ASSERT_EQ(h3, h1);
    BMQTST_ASSERT_EQ(h3, h4);
}

static void test4_groupIdExpiresAfterTtlTest()
// ------------------------------------------------------------------------
// GROUP ID EXPIRES AFTER TTL TEST
//
// Concerns:
//   'getHandle()' produces deterministic ordering
//
// Plan:
//   Allocate GID1 and GID2.  If after expiry they're allocated in the
//   reverse order, then handles should be swapped compared to before
//   expiry
//
// Testing:
//   getHandle
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("GROUP ID EXPIRES AFTER TTL TEST");

    MessageGroupIdManager obj(k_TIMEOUT,
                              k_MAX_NUMBER_OF_MAPPINGS,
                              MessageGroupIdManager::k_REBALANCE_OFF,
                              bmqtst::TestHelperUtil::allocator());
    addHandles(&obj);

    const Handle h1  = obj.getHandle(*k_GID1, k_T0);
    const Handle h2  = obj.getHandle(*k_GID2, k_T0);
    const Handle h2b = obj.getHandle(*k_GID2, k_T0 + k_TIMEOUT);
    const Handle h1b = obj.getHandle(*k_GID1, k_T0 + k_TIMEOUT);

    BMQTST_ASSERT_NE(h1, h2);
    BMQTST_ASSERT_NE(h1b, h2b);

    BMQTST_ASSERT_NE(h1, h1b);
    BMQTST_ASSERT_NE(h2, h2b);
    BMQTST_ASSERT_EQ(h1, h2b);
    BMQTST_ASSERT_EQ(h2, h1b);
}

static void test5_longLivedMoreThanMaxGroupsGcTest()
// ------------------------------------------------------------------------
// LONG LIVED MORE THAN MAX GROUPS GC TEST
//
// Concerns:
//   'getHandle()' has a limit of max mappings remembered.  When rebalance
//   is on, this means that we discard old mappings if this limit is
//   crossed
//
// Plan:
//   Set the limit to 2. Allocate GID1, GID2 and GID3 and make sure they're
//   all allocated as expected
//
// Testing:
//   getHandle
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "LONG LIVED MORE THAN MAX GROUPS GC TEST");

    const int             k_MY_MAX_NUMBER_OF_MAPPINGS = 2;
    MessageGroupIdManager obj(k_TIMEOUT,
                              k_MY_MAX_NUMBER_OF_MAPPINGS,
                              MessageGroupIdManager::k_REBALANCE_ON,
                              bmqtst::TestHelperUtil::allocator());
    addHandles(&obj);

    const Handle h1  = obj.getHandle(*k_GID1, k_T0 + 1);
    const Handle h2  = obj.getHandle(*k_GID2, k_T0 + 2);
    const Handle h1b = obj.getHandle(*k_GID1, k_T0 + 3);
    const Handle h3  = obj.getHandle(*k_GID3, k_T0 + 4);
    const Handle h4  = obj.getHandle(*k_GID2, k_T0 + 5);

    BMQTST_ASSERT_EQ(h1, _(0));
    BMQTST_ASSERT_EQ(h2, _(1));
    BMQTST_ASSERT_EQ(h1b, _(0));
    BMQTST_ASSERT_EQ(h3, _(1));  // There was eviction of GID2 (oldest), here
    BMQTST_ASSERT_EQ(h4, _(0));  // Again - GID1 (oldest) is evicted
}

static void test6_shortLivedMoreThanMaxGroupsGcTest()
// ------------------------------------------------------------------------
// SHORT LIVED MORE THAN MAX GROUPS GC TEST
//
// Concerns:
//   'getHandle()' has a limit of max mappings remembered.  When rebalance
//   is off, this is a soft limit meaning that old mappings will remain
//   active till they expire.
//
// Plan:
//   Set the limit to 1. Allocate GID1 and GID2 and make sure they're all
//   allocated as expected
//
// Testing:
//   getHandle, exceedsMaxMsgGroupIdsLimit
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "SHORT LIVED MORE THAN MAX GROUPS GC TEST");

    const int             k_MY_MAX_NUMBER_OF_MAPPINGS = 1;
    MessageGroupIdManager obj(k_TIMEOUT,
                              k_MY_MAX_NUMBER_OF_MAPPINGS,
                              MessageGroupIdManager::k_REBALANCE_OFF,
                              bmqtst::TestHelperUtil::allocator());
    addHandles(&obj);

    const Handle h1 = obj.getHandle(*k_GID1, k_T0);
    BMQTST_ASSERT_EQ(false, obj.exceedsMaxMsgGroupIdsLimit());

    const Handle h2 = obj.getHandle(*k_GID2, k_T0);
    BMQTST_ASSERT_EQ(true, obj.exceedsMaxMsgGroupIdsLimit());

    const Handle h1b = obj.getHandle(*k_GID1, k_T0);
    const Handle h2b = obj.getHandle(*k_GID2, k_T0);
    const Handle h2c = obj.getHandle(*k_GID2, k_T0);
    const Handle h1c = obj.getHandle(*k_GID1, k_T0 + 5);
    BMQTST_ASSERT_EQ(true, obj.exceedsMaxMsgGroupIdsLimit());

    // This clears everything that timed-out i.e. 'GID2'. Thus we're now back
    // in capacity.
    const Time   texp = k_T0 + k_TIMEOUT + 2;
    const Handle h1d  = obj.getHandle(*k_GID1, texp);
    BMQTST_ASSERT_EQ(false, obj.exceedsMaxMsgGroupIdsLimit());

    BMQTST_ASSERT_NE(h1, h2);
    BMQTST_ASSERT_EQ(h1, h1b);
    BMQTST_ASSERT_EQ(h1, h1c);
    BMQTST_ASSERT_EQ(h1, h1d);
    BMQTST_ASSERT_EQ(h2, h2b);
    BMQTST_ASSERT_EQ(h2, h2c);
}

static void test7_retrievesGroupIdsFromHandlesTest()
// ------------------------------------------------------------------------
// RETRIEVES GROUP IDS FROM HANDLES TEST
//
// Concerns:
//   'getHandle()' should keep track of the Message Group Ids for each
//   handle.  This is exposed by the 'idsForHandle()' call.  We confirm
//   that it returns groups as expected
//
// Plan:
//   Make limited number of handles (just 2) available.  Allocate GID1-4
//   and confirm they're mapped to handles with the expected properties
//
// Testing:
//   idsForHandle
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("RETRIEVES GROUP IDS FROM HANDLES TEST");

    MessageGroupIdManager obj(k_TIMEOUT,
                              k_MAX_NUMBER_OF_MAPPINGS,
                              MessageGroupIdManager::k_REBALANCE_ON,
                              bmqtst::TestHelperUtil::allocator());
    const int             JUST_FEW = 2;
    addHandles(&obj, JUST_FEW);

    const Handle h1 = obj.getHandle(*k_GID1, k_T0);
    const Handle h2 = obj.getHandle(*k_GID2, k_T0);
    const Handle h3 = obj.getHandle(*k_GID3, k_T0 + 1);
    const Handle h4 = obj.getHandle(*k_GID4, k_T0 + 1);

    BMQTST_ASSERT_EQ(h1, h3);
    BMQTST_ASSERT_EQ(h2, h4);
    BMQTST_ASSERT_NE(h1, h2);

    // Confirm the handle view is as expected
    MessageGroupIdManager::IdsForHandle gids(
        bmqtst::TestHelperUtil::allocator());
    obj.idsForHandle(&gids, h1);

    BMQTST_ASSERT_EQ(gids.size(), 2u);
    BMQTST_ASSERT_EQ(gids.find(*k_GID1) != gids.end(), true);
    BMQTST_ASSERT_EQ(gids.find(*k_GID3) != gids.end(), true);

    gids.clear();
    obj.idsForHandle(&gids, h2);
    BMQTST_ASSERT_EQ(gids.size(), 2u);
    BMQTST_ASSERT_EQ(gids.find(*k_GID2) != gids.end(), true);
    BMQTST_ASSERT_EQ(gids.find(*k_GID4) != gids.end(), true);

    // Trigger timeouts of the first two (GID1-2)
    (void)obj.getHandle(*k_GID3, k_T0 + k_TIMEOUT);

    // Confirm the handle view is as expected
    gids.clear();
    obj.idsForHandle(&gids, h1);
    BMQTST_ASSERT_EQ(gids.size(), 1u);
    BMQTST_ASSERT_EQ(gids.find(*k_GID3) != gids.end(), true);
    gids.clear();
    obj.idsForHandle(&gids, h2);
    BMQTST_ASSERT_EQ(gids.size(), 1u);
    BMQTST_ASSERT_EQ(gids.find(*k_GID4) != gids.end(), true);
}

static void test8_rebalanceWhenAddingHandleTest()
// ------------------------------------------------------------------------
// REBALANCE WHEN ADDING HANDLE TEST
//
// Concerns:
//   When we add more handles with 'addHandle()' and if rebalance is on,
//   existing groups should be relocated to the new handle.
//
// Plan:
//   Start with 3 handles.  Allocate 120 Message Group Ids and confirm
//   there are 40 Message Group Ids per handle.  Then add one extra handle
//   and confirm that there are 30 Message Group Ids per handle.
//
// Testing:
//   addHandle
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("REBALANCE WHEN ADDING HANDLE TEST");

    typedef MessageGroupIdManager::IdsForHandle IdsForHandle;

    const int k_HANDLES_LIMIT_BEFORE = 3;
    const int k_HANDLES_LIMIT_AFTER  = k_HANDLES_LIMIT_BEFORE + 1;
    const int k_MSG_GROUP_IDS_COUNT  = k_HANDLES_LIMIT_BEFORE *
                                      k_HANDLES_LIMIT_AFTER * 10;
    MessageGroupIdManager obj(k_TIMEOUT,
                              k_MAX_NUMBER_OF_MAPPINGS,
                              MessageGroupIdManager::k_REBALANCE_ON,
                              bmqtst::TestHelperUtil::allocator());
    addHandles(&obj, k_HANDLES_LIMIT_BEFORE);

    allocateSomeGroups(&obj, k_HANDLES_LIMIT_BEFORE, k_MSG_GROUP_IDS_COUNT);

    BSLS_ASSERT_SAFE((k_MSG_GROUP_IDS_COUNT % k_HANDLES_LIMIT_AFTER) == 0);

    bsl::vector<IdsForHandle> before(k_HANDLES_LIMIT_BEFORE,
                                     bmqtst::TestHelperUtil::allocator());
    for (int i = 0; i < k_HANDLES_LIMIT_BEFORE; ++i) {
        obj.idsForHandle(&before[i], _(i));
    }

    // A new handle arrives
    obj.addHandle(_(3), k_T0);

    // Message Group Ids have been re-allocated
    const unsigned k_GROUPS_PER_HANDLE_AFTER = k_MSG_GROUP_IDS_COUNT /
                                               k_HANDLES_LIMIT_AFTER;

    bsl::vector<IdsForHandle> after(k_HANDLES_LIMIT_AFTER,
                                    bmqtst::TestHelperUtil::allocator());
    for (int i = 0; i < k_HANDLES_LIMIT_AFTER; ++i) {
        obj.idsForHandle(&after[i], _(i));

        BMQTST_ASSERT_EQ(after[i].size(), k_GROUPS_PER_HANDLE_AFTER);
    }

    // Most elements should remain the same after rebalancing in the common
    // case described here.
    for (int i = 0; i < k_HANDLES_LIMIT_BEFORE; ++i) {
        typedef bsl::vector<MessageGroupIdManager::MsgGroupId>
                         IntersectionType;
        IntersectionType intersection(k_MSG_GROUP_IDS_COUNT,
                                      bmqtst::TestHelperUtil::allocator());
        const IntersectionType::const_iterator end = bsl::set_intersection(
            before[i].begin(),
            before[i].end(),
            after[i].begin(),
            after[i].end(),
            intersection.begin());
        const int intersectionLength = end - intersection.begin();

        // No rounding errors due to 'MSG_GROUP_IDS_COUNT' being (by design) a
        // multiple of both HANDLES_LIMIT_BEFORE and HANDLES_LIMIT_AFTER
        const int idsBefore = (k_MSG_GROUP_IDS_COUNT / k_HANDLES_LIMIT_BEFORE);
        const int idsAfter  = (k_MSG_GROUP_IDS_COUNT / k_HANDLES_LIMIT_AFTER);

        BMQTST_ASSERT_EQ(intersectionLength,
                         idsBefore - (idsBefore - idsAfter));
    }
}

static void test9_noRebalanceWhenRemovingHandleTest()
// ------------------------------------------------------------------------
// NO REBALANCE WHEN REMOVING HANDLE TEST
//
// Concerns:
//   When we remove a handle with 'removeHandle()', regardless of rebalance
//   mode, any records for Message Group Ids allocated to that handle will
//   disappear allowing free re-allocation when needed next time.
//
// Plan:
//   Start with 3 handles and allocate 30 handles on them.When we remove a
//   handle, there should be no change for Message Group Ids for unaffected
//   handles, while all information for the deleted handle should have been
//   discarded.
//
// Testing:
//   removeHandle
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "NO REBALANCE WHEN REMOVING HANDLE TEST");

    typedef MessageGroupIdManager::IdsForHandle IdsForHandle;

    for (int rebalanceMode = 0; rebalanceMode < 2; ++rebalanceMode) {
        const int             HANDLES_LIMIT_BEFORE = 3;
        const int             MSG_GROUP_IDS_COUNT  = 10 * HANDLES_LIMIT_BEFORE;
        MessageGroupIdManager obj(
            k_TIMEOUT,
            k_MAX_NUMBER_OF_MAPPINGS,
            (rebalanceMode == 0 ? MessageGroupIdManager::k_REBALANCE_OFF
                                : MessageGroupIdManager::k_REBALANCE_ON),
            bmqtst::TestHelperUtil::allocator());
        addHandles(&obj, HANDLES_LIMIT_BEFORE);
        allocateSomeGroups(&obj, HANDLES_LIMIT_BEFORE, MSG_GROUP_IDS_COUNT);

        bsl::vector<IdsForHandle> before(HANDLES_LIMIT_BEFORE,
                                         bmqtst::TestHelperUtil::allocator());
        for (int i = 0; i < HANDLES_LIMIT_BEFORE; ++i) {
            obj.idsForHandle(&before[i], _(i));
        }

        // Remove a handle
        const int toRemove = 1;
        obj.removeHandle(_(toRemove));

        // Message Group Ids should remain the same as before with the
        // exception of the removed handle
        bsl::vector<IdsForHandle> after(HANDLES_LIMIT_BEFORE,
                                        bmqtst::TestHelperUtil::allocator());
        for (int i = 0; i < HANDLES_LIMIT_BEFORE; ++i) {
            obj.idsForHandle(&after[i], _(i));
            if (i == toRemove) {
                BMQTST_ASSERT_EQ(0u, after[i].size())
            }
            else {
                BMQTST_ASSERT_EQ(before[i], after[i])
            }
        }
    }
}

static void test10_chooseLeastUsedHandleAfterRemoveTest()
// ------------------------------------------------------------------------
// CHOOSE LEAST USED HANDLE AFTER REMOVE TEST
//
// Concerns:
//   When we remove a handle with 'removeHandle()', regardless of rebalance
//   mode, when we add a new or previously expired handle, it gets mapped
//   to a random handle.
//
// Plan:
//   Add 3 handles and map a few Message Group Ids to them. Then remove a
//   handle.  When I re-add a previously seen Message Group Id that was
//   mapped on the removed handle, it is mapped to the least used handle
//   regardless of it's previous allocation.  When I add more handles,
//   distribution continues as expected.
//
// Testing:
//   getHandle, addHandle, removeHandle
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "CHOOSE LEAST USED HANDLE AFTER REMOVE TEST");

    for (int rebalanceMode = 0; rebalanceMode < 2; ++rebalanceMode) {
        MessageGroupIdManager obj(
            k_TIMEOUT,
            k_MAX_NUMBER_OF_MAPPINGS,
            (rebalanceMode == 0 ? MessageGroupIdManager::k_REBALANCE_OFF
                                : MessageGroupIdManager::k_REBALANCE_ON),
            bmqtst::TestHelperUtil::allocator());

        obj.addHandle(_(1), k_T0);
        obj.addHandle(_(2), k_T0);
        obj.addHandle(_(3), k_T0);

        BMQTST_ASSERT_EQ(obj.getHandle(msgGroupIdFromInt(17), k_T0), _(1));
        BMQTST_ASSERT_EQ(obj.getHandle(msgGroupIdFromInt(18), k_T0), _(2));
        BMQTST_ASSERT_EQ(obj.getHandle(msgGroupIdFromInt(19), k_T0), _(3));
        BMQTST_ASSERT_EQ(obj.getHandle(msgGroupIdFromInt(20), k_T0), _(1));

        obj.removeHandle(_(2));

        // Where did 18 end up? Nowhere - re-allocation(!)
        BMQTST_ASSERT_EQ(obj.getHandle(msgGroupIdFromInt(18), k_T0), _(3));

        obj.addHandle(_(4), k_T0);

        BMQTST_ASSERT_EQ(obj.getHandle(msgGroupIdFromInt(21), k_T0), _(4));
        BMQTST_ASSERT_EQ(obj.getHandle(msgGroupIdFromInt(22), k_T0), _(4));
        BMQTST_ASSERT_EQ(obj.getHandle(msgGroupIdFromInt(23), k_T0), _(1));
        BMQTST_ASSERT_EQ(obj.getHandle(msgGroupIdFromInt(24), k_T0), _(3));
        BMQTST_ASSERT_EQ(obj.getHandle(msgGroupIdFromInt(25), k_T0), _(4));
    }
}

static void test11_stronglyUnbalancedChainOnRebalanceTest()
// ------------------------------------------------------------------------
// STRONGLY UNBALANCED CHAIN ON REBALANCE TEST
//
// Concerns:
//   Rebalance works nicely when handles aren't nicely balanced to start
//   with.
//
// Plan:
//   Add 3 handles and 33 Message Group ids (11 for each).  Make most of
//   them expire but the ones in handle '0'.  Then add a new handle to
//   force rebalance that should result handles with at most 9/4 = 3
//   groups per handle.  This means that 8 will have to be removed from
//   handle 0 and be mapped to handles 1-3.
//
// Testing:
//   getHandle, addHandle
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName(
        "STRONGLY UNBALANCED CHAIN ON REBALANCE TEST");

    typedef MessageGroupIdManager::IdsForHandle IdsForHandle;

    const int HANDLES_COUNT        = 3;
    const int GROUP_IDS_PER_HANDLE = 11;
    const int MSG_GROUP_IDS_COUNT  = GROUP_IDS_PER_HANDLE * HANDLES_COUNT;

    MessageGroupIdManager obj(k_TIMEOUT,
                              k_MAX_NUMBER_OF_MAPPINGS,
                              MessageGroupIdManager::k_REBALANCE_ON,
                              bmqtst::TestHelperUtil::allocator());

    // Add handles
    for (int i = 0; i < HANDLES_COUNT; ++i) {
        obj.addHandle(_(i), k_T0);
    }

    // Add Message Group Ids
    for (int i = 0; i < MSG_GROUP_IDS_COUNT; ++i) {
        BMQTST_ASSERT_EQ(obj.getHandle(msgGroupIdFromInt(i), k_T0),
                         _(i % HANDLES_COUNT));
    }

    // Ping ids for handle 0 to retain them after the next Garbage Collection.
    const Time justBeforeTimeout = k_T0 + k_TIMEOUT - 1;
    for (int i = 0; i < GROUP_IDS_PER_HANDLE; ++i) {
        const int idForHandle0 = i * HANDLES_COUNT;
        obj.getHandle(msgGroupIdFromInt(idForHandle0), justBeforeTimeout);
    }

    // This reference is at Timeout, which means that all but handle-0 Message
    // Group Ids should be cleaned-up after this.  The sizes should look like:
    //
    //  / 11 of them here. The rest has timed-out
    // *
    // ...
    // *
    // *
    // h0  h1  h2
    obj.getHandle(msgGroupIdFromInt(0), k_T0 + k_TIMEOUT);

    IdsForHandle gids(bmqtst::TestHelperUtil::allocator());
    obj.idsForHandle(&gids, _(0));
    BMQTST_ASSERT_EQ(GROUP_IDS_PER_HANDLE, static_cast<int>(gids.size()));
    BMQTST_ASSERT_EQ(GROUP_IDS_PER_HANDLE, obj.msgGroupIdsCount());

    // Let's add a handle. This forces rebalance.
    obj.addHandle(_(3), k_T0);

    // The sizes after should be as follows:
    //
    //    / original ones - didn't move
    //   /   / rebalanced from h0
    //  /   /   / rebalanced from h0
    // *   *   *    / rebalanced from h0 (there weren't enough - so just 2)
    // *   *   *   *
    // *   *   *   *
    // h0  h1  h2  h3
    const int expectedSizes[] = {3, 3, 3, 2};
    for (int i = 0; i < (HANDLES_COUNT + 1); ++i) {
        gids.clear();
        obj.idsForHandle(&gids, _(i));
        BMQTST_ASSERT_EQ_D(i, expectedSizes[i], static_cast<int>(gids.size()));
    }
}

static void test12_printerTest()
{
    bmqtst::TestHelper::printTestName("PRINTER TEST");

    MessageGroupIdManager obj(k_TIMEOUT,
                              k_MAX_NUMBER_OF_MAPPINGS,
                              MessageGroupIdManager::k_REBALANCE_ON,
                              bmqtst::TestHelperUtil::allocator());

    typedef bsl::shared_ptr<mqbi::Queue> QueuePtr;
    QueuePtr queue(new (*bmqtst::TestHelperUtil::allocator())
                       mqbmock::Queue(0, bmqtst::TestHelperUtil::allocator()),
                   bmqtst::TestHelperUtil::allocator());

    // Add handles
    MyDispatcherClient dc1("1"), dc2("2"), dc3("3");
    bsl::shared_ptr<mqbi::QueueHandleRequesterContext> context1(
        new (*bmqtst::TestHelperUtil::allocator())
            mqbi::QueueHandleRequesterContext(
                bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<mqbi::QueueHandleRequesterContext> context2(
        new (*bmqtst::TestHelperUtil::allocator())
            mqbi::QueueHandleRequesterContext(
                bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    bsl::shared_ptr<mqbi::QueueHandleRequesterContext> context3(
        new (*bmqtst::TestHelperUtil::allocator())
            mqbi::QueueHandleRequesterContext(
                bmqtst::TestHelperUtil::allocator()),
        bmqtst::TestHelperUtil::allocator());
    context1->setClient(&dc1);
    context2->setClient(&dc2);
    context3->setClient(&dc3);

    // LocalSequentialAllocator ensures that the pointers are in-order
    typedef bslma::ManagedPtr<mqbi::QueueHandle> QueueHandleMp;

    bdlma::LocalSequentialAllocator<10000> lsa;
    QueueHandleMp h1(queueHandle(context1, queue, &lsa));
    QueueHandleMp h2(queueHandle(context2, queue, &lsa));
    QueueHandleMp h3(queueHandle(context3, queue, &lsa));

    BSLS_ASSERT_SAFE(h1.get() < h2.get());
    BSLS_ASSERT_SAFE(h2.get() < h3.get());

    obj.addHandle(h1.get(), k_T0);
    obj.addHandle(h2.get(), k_T0);
    obj.addHandle(h3.get(), k_T0);

    // Add Message Group Ids
    for (int i = 0; i < 10; ++i) {
        (void)obj.getHandle(msgGroupIdFromInt(i), k_T0);
    }

    // Dump internals
    const Time         k_at = k_T0 + 5;
    bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
    mqbcmd::Result     result(bmqtst::TestHelperUtil::allocator());
    obj.loadInternals(&result.makeMessageGroupIdHelper(), k_at);
    mqbcmd::HumanPrinter::print(ss, result, 1);

    const char* expected = "    MessageGroupIdManager:\n"
                           "    timeout........: 100\n"
                           "    maxMsgGroupIds.: -1\n"
                           "    rebalance......: true\n"
                           "    status.........: [\n"
                           "        Lru:\n"
                           "        [\n"
                           "            1#0 = \"5 ns\"\n"
                           "            2#1 = \"5 ns\"\n"
                           "            3#2 = \"5 ns\"\n"
                           "            1#3 = \"5 ns\"\n"
                           "            2#4 = \"5 ns\"\n"
                           "            3#5 = \"5 ns\"\n"
                           "            1#6 = \"5 ns\"\n"
                           "            2#7 = \"5 ns\"\n"
                           "            3#8 = \"5 ns\"\n"
                           "            1#9 = \"5 ns\"\n"
                           "        ]\n"
                           "        Handles:\n"
                           "        [\n"
                           "            2 = 3\n"
                           "            3 = 3\n"
                           "            1 = 4\n"
                           "        ]\n"
                           "    ]";

    BMQTST_ASSERT_EQ(ss.str(), expected);
}

static void test13_largeMessageGroupIdsUseAllocator()
// ------------------------------------------------------------------------
// LARGE MESSAGE GROUP IDS USE ALLOCATOR
//
// Concerns:
//   When Message Group Ids are large strings, allocators should
//   successfully be used on various data structures.
//
// Plan:
//   Repeat and add/remove/rebalance circle with large Message Group Ids.
//
// Testing:
//   getHandle, addHandle
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("LARGE MESSAGE GROUP IDS USE ALLOCATOR");

    for (int rebalanceMode = 0; rebalanceMode < 2; ++rebalanceMode) {
        MessageGroupIdManager obj(
            k_TIMEOUT,
            k_MAX_NUMBER_OF_MAPPINGS,
            (rebalanceMode == 0 ? MessageGroupIdManager::k_REBALANCE_OFF
                                : MessageGroupIdManager::k_REBALANCE_ON),
            bmqtst::TestHelperUtil::allocator());

        addHandles(&obj, 3);

        for (int i = 0; i < 10; ++i) {
            bmqu::MemOutStream ss(bmqtst::TestHelperUtil::allocator());
            ss << "01234567890123456789012345678901234567890123456789" << i;
            (void)obj.getHandle(
                bsl::string(ss.str(), bmqtst::TestHelperUtil::allocator()),
                k_T0);
        }

        // Remove a handle
        const int toAdd = 4;
        obj.addHandle(_(toAdd), k_T0);
    }
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    const MsgGroupId gid1("1", bmqtst::TestHelperUtil::allocator());
    const MsgGroupId gid2("2", bmqtst::TestHelperUtil::allocator());
    const MsgGroupId gid3("3", bmqtst::TestHelperUtil::allocator());
    const MsgGroupId gid4("4", bmqtst::TestHelperUtil::allocator());

    k_GID1 = &gid1;
    k_GID2 = &gid2;
    k_GID3 = &gid3;
    k_GID4 = &gid4;

    switch (_testCase) {
    case 0:
    case 13: test13_largeMessageGroupIdsUseAllocator(); break;
    case 12: test12_printerTest(); break;
    case 11: test11_stronglyUnbalancedChainOnRebalanceTest(); break;
    case 10: test10_chooseLeastUsedHandleAfterRemoveTest(); break;
    case 9: test9_noRebalanceWhenRemovingHandleTest(); break;
    case 8: test8_rebalanceWhenAddingHandleTest(); break;
    case 7: test7_retrievesGroupIdsFromHandlesTest(); break;
    case 6: test6_shortLivedMoreThanMaxGroupsGcTest(); break;
    case 5: test5_longLivedMoreThanMaxGroupsGcTest(); break;
    case 4: test4_groupIdExpiresAfterTtlTest(); break;
    case 3: test3_timeoutOldMappingsTest(); break;
    case 2: test2_differentGroupIdsHaveDifferentHandlesTest(); break;
    case 1: test1_sameGroupIdSameHandleTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
