// Copyright 2026 Bloomberg Finance L.P.
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

// mqbraft_clusteravailability.t.cpp -*-C++-*-
#include <mqbraft_clusteravailability.h>

// BMQ
#include <bmqp_ctrlmsg_messages.h>

// BDE
#include <bsl_iostream.h>
#include <bsl_string.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

using namespace BloombergLP;
using namespace BloombergLP::mqbraft;
using namespace bsl;

// ============================================================================
//                            TEST HELPERS UTILITY
// ----------------------------------------------------------------------------

namespace {

typedef ClusterAvailability::Action Action;

/// CSL term of the last `setCsl`.
bsls::Types::Uint64 g_cslTerm = 1;

/// Report to the specified `obj` a CSL leader in a new CSL term; the
/// specified `isSelfLeader` tells whether self is the CSL leader.  If the
/// specified `isCaughtUp` is `true`, also report an advisory of that term.
void setCsl(ClusterAvailability* obj, bool isSelfLeader, bool isCaughtUp)
{
    ++g_cslTerm;
    obj->onCslLeader(true, isSelfLeader, g_cslTerm);
    if (isCaughtUp) {
        ClusterAvailability::PartitionPrimaryInfos infos(
            bmqtst::TestHelperUtil::allocator());
        obj->onLeaseIdsConfirmed(infos, g_cslTerm, g_cslTerm);
    }
}

/// Report the specified `leaseId` of the specified `partitionId` as
/// confirmed to the specified `obj`, by an advisory of the current CSL term.
void confirm(ClusterAvailability* obj, int partitionId, unsigned int leaseId)
{
    ClusterAvailability::PartitionPrimaryInfos infos(
        bmqtst::TestHelperUtil::allocator());
    bmqp_ctrlmsg::PartitionPrimaryInfo info;
    info.partitionId()    = partitionId;
    info.primaryNodeId()  = 1;
    info.primaryLeaseId() = leaseId;
    infos.push_back(info);
    obj->onLeaseIdsConfirmed(infos, g_cslTerm, g_cslTerm);
}

}  // close unnamed namespace

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_breathingTest()
{
    bmqtst::TestHelper::printTestName("BREATHING TEST");

    ClusterAvailability obj(2, bmqtst::TestHelperUtil::allocator());

    BMQTST_ASSERT_EQ(0U, obj.leaseId(0));
    BMQTST_ASSERT_EQ(0U, obj.confirmedLeaseId(0));
    BMQTST_ASSERT(!obj.isActive(0));

    BMQTST_ASSERT_EQ(Action::e_NONE, obj.onPartitionPrimary(1, 7, false));
    BMQTST_ASSERT_EQ(0U, obj.leaseId(0));
    BMQTST_ASSERT_EQ(7U, obj.leaseId(1));

    obj.onPartitionNoPrimary(1);
    BMQTST_ASSERT_EQ(0U, obj.leaseId(1));

    confirm(&obj, 1, 7);
    BMQTST_ASSERT_EQ(0U, obj.confirmedLeaseId(0));
    BMQTST_ASSERT_EQ(7U, obj.confirmedLeaseId(1));
}

static void test2_waitReasons()
{
    bmqtst::TestHelper::printTestName("WAIT REASONS");

    ClusterAvailability obj(1, bmqtst::TestHelperUtil::allocator());

    // No primary.
    setCsl(&obj, false, true);
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.evaluatePartition(0));

    // CSL not caught up.
    setCsl(&obj, false, false);
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.onPartitionPrimary(0, 5, true));

    // LeaseId not confirmed.
    setCsl(&obj, false, true);
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.evaluatePartition(0));

    // A different leaseId confirmed.
    confirm(&obj, 0, 4);
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.evaluatePartition(0));

    // The CSL leader stops.
    confirm(&obj, 0, 5);
    obj.onCslLeaderStopping();
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.evaluatePartition(0));

    // None of the waits above asked for the sync point.
    setCsl(&obj, false, true);
    BMQTST_ASSERT_EQ(Action::e_REQUEST_SYNC_POINT, obj.evaluatePartition(0));
}

static void test3_selfPrimary()
{
    bmqtst::TestHelper::printTestName("SELF PRIMARY");

    ClusterAvailability obj(1, bmqtst::TestHelperUtil::allocator());

    setCsl(&obj, false, true);
    confirm(&obj, 0, 5);

    // The leaseId is confirmed already: ask for the sync point.
    BMQTST_ASSERT_EQ(Action::e_REQUEST_SYNC_POINT,
                     obj.onPartitionPrimary(0, 5, true));

    // Do not ask again.
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.evaluatePartition(0));

    // The only partition becomes active: become available.
    BMQTST_ASSERT_EQ(Action::e_ACTIVATE_AND_BECOME_AVAILABLE,
                     obj.onSyncPointApplied(0, 5));
    BMQTST_ASSERT(obj.isActive(0));
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.evaluatePartition(0));

    // A new leaseId makes the partition not active; becoming active again
    // does not make self available again.
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.onPartitionPrimary(0, 6, true));
    BMQTST_ASSERT(!obj.isActive(0));
    confirm(&obj, 0, 6);
    BMQTST_ASSERT_EQ(Action::e_REQUEST_SYNC_POINT, obj.evaluatePartition(0));
    BMQTST_ASSERT_EQ(Action::e_ACTIVATE, obj.onSyncPointApplied(0, 6));
}

static void test4_syncPointAppliedFirst()
{
    bmqtst::TestHelper::printTestName("SYNC POINT APPLIED FIRST");

    // Self, as the primary, applies the sync point before the CSL confirms
    // the leaseId.  The CSL event must not ask for the sync point again.
    ClusterAvailability obj(1, bmqtst::TestHelperUtil::allocator());

    setCsl(&obj, false, true);
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.onPartitionPrimary(0, 5, true));
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.onSyncPointApplied(0, 5));

    confirm(&obj, 0, 5);
    BMQTST_ASSERT_EQ(Action::e_ACTIVATE, obj.evaluatePartition(0));
    BMQTST_ASSERT(obj.becomeAvailable());
    BMQTST_ASSERT(!obj.becomeAvailable());
}

static void test5_otherRoles()
{
    bmqtst::TestHelper::printTestName("OTHER ROLES");

    {
        // Neither the primary nor the CSL leader: the CSL event that
        // confirms the leaseId activates the partition.
        ClusterAvailability obj(1, bmqtst::TestHelperUtil::allocator());
        setCsl(&obj, false, true);
        BMQTST_ASSERT_EQ(Action::e_NONE, obj.onPartitionPrimary(0, 5, false));

        confirm(&obj, 0, 5);
        BMQTST_ASSERT_EQ(Action::e_ACTIVATE, obj.evaluatePartition(0));
        BMQTST_ASSERT(obj.becomeAvailable());
    }
    {
        // The CSL leader waits until the sync point is applied, and never
        // asks for the sync point.
        ClusterAvailability obj(1, bmqtst::TestHelperUtil::allocator());
        setCsl(&obj, true, true);
        confirm(&obj, 0, 5);

        BMQTST_ASSERT_EQ(Action::e_NONE, obj.onPartitionPrimary(0, 5, false));
        BMQTST_ASSERT_EQ(Action::e_NONE, obj.evaluatePartition(0));

        BMQTST_ASSERT_EQ(Action::e_ACTIVATE_AND_BECOME_AVAILABLE,
                         obj.onSyncPointApplied(0, 5));
    }
    {
        // Losing the primary makes the partition not active.
        ClusterAvailability obj(1, bmqtst::TestHelperUtil::allocator());
        setCsl(&obj, false, true);
        confirm(&obj, 0, 5);
        BMQTST_ASSERT_EQ(Action::e_ACTIVATE_AND_BECOME_AVAILABLE,
                         obj.onPartitionPrimary(0, 5, false));

        obj.onPartitionNoPrimary(0);
        BMQTST_ASSERT(!obj.isActive(0));
        BMQTST_ASSERT_EQ(Action::e_NONE, obj.evaluatePartition(0));
    }
}

static void test6_cslEventSeveralPartitions()
{
    bmqtst::TestHelper::printTestName("CSL EVENT SEVERAL PARTITIONS");

    // Self is the primary of partition 1 and neither the primary nor the
    // CSL leader for partition 0.  One CSL event (the advisory commits)
    // activates partition 0 and asks for the sync point of partition 1.
    ClusterAvailability obj(2, bmqtst::TestHelperUtil::allocator());

    setCsl(&obj, false, true);
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.onPartitionPrimary(0, 3, false));
    BMQTST_ASSERT_EQ(Action::e_NONE, obj.onPartitionPrimary(1, 4, true));

    confirm(&obj, 0, 3);
    confirm(&obj, 1, 4);
    BMQTST_ASSERT_EQ(Action::e_ACTIVATE, obj.evaluatePartition(0));
    BMQTST_ASSERT_EQ(Action::e_REQUEST_SYNC_POINT, obj.evaluatePartition(1));
    BMQTST_ASSERT(!obj.becomeAvailable());

    BMQTST_ASSERT_EQ(Action::e_ACTIVATE_AND_BECOME_AVAILABLE,
                     obj.onSyncPointApplied(1, 4));
    BMQTST_ASSERT(!obj.becomeAvailable());
}

static void test7_needsAdvisory()
{
    bmqtst::TestHelper::printTestName("NEEDS ADVISORY");

    ClusterAvailability obj(2, bmqtst::TestHelperUtil::allocator());

    // Not the CSL leader.
    obj.onPartitionPrimary(0, 3, false);
    obj.onPartitionPrimary(1, 4, false);
    BMQTST_ASSERT(!obj.needsAdvisory());

    // A partition with no primary.
    setCsl(&obj, true, false);
    obj.onPartitionNoPrimary(1);
    BMQTST_ASSERT(!obj.needsAdvisory());

    // Every partition has a primary; CSL not caught up.  Once per set of
    // leaseIds.
    obj.onPartitionPrimary(1, 4, false);
    BMQTST_ASSERT(obj.needsAdvisory());
    BMQTST_ASSERT(!obj.needsAdvisory());

    // Caught up; nothing confirmed; the advisory is not committed.
    setCsl(&obj, true, true);
    BMQTST_ASSERT(!obj.needsAdvisory());

    // A new leaseId while the advisory is not committed.
    obj.onPartitionPrimary(1, 5, false);
    BMQTST_ASSERT(obj.needsAdvisory());
    BMQTST_ASSERT(!obj.needsAdvisory());

    // The advisory commits.
    confirm(&obj, 0, 3);
    confirm(&obj, 1, 5);
    BMQTST_ASSERT(!obj.needsAdvisory());

    // Losing and regaining CSL leadership proposes the advisory again, if a
    // leaseId is not confirmed.
    obj.onPartitionPrimary(1, 6, false);
    BMQTST_ASSERT(obj.needsAdvisory());
    setCsl(&obj, false, false);
    BMQTST_ASSERT(!obj.needsAdvisory());
    setCsl(&obj, true, false);
    BMQTST_ASSERT(obj.needsAdvisory());

    // Losing and regaining CSL leadership with every leaseId confirmed: the
    // new leader's 'LeaderAdvisory' is not committed yet.
    confirm(&obj, 1, 6);
    setCsl(&obj, false, false);
    setCsl(&obj, true, false);
    BMQTST_ASSERT(obj.needsAdvisory());
    setCsl(&obj, true, true);
    BMQTST_ASSERT(!obj.needsAdvisory());
}

static void test9_confirmedNeverDecreases()
{
    bmqtst::TestHelper::printTestName("CONFIRMED NEVER DECREASES");

    // A CSL leader behind on partition 0 sees the old primary (node 1,
    // leaseId 5) while the CSL has confirmed the new primary (node 3,
    // leaseId 6).
    ClusterAvailability obj(1, bmqtst::TestHelperUtil::allocator());

    ClusterAvailability::PartitionPrimaryInfos infos(
        bmqtst::TestHelperUtil::allocator());
    infos.resize(1);
    infos[0].partitionId()    = 0;
    infos[0].primaryNodeId()  = 3;
    infos[0].primaryLeaseId() = 6;
    obj.onLeaseIdsConfirmed(infos, g_cslTerm, g_cslTerm);
    BMQTST_ASSERT_EQ(6U, obj.confirmedLeaseId(0));

    // A lower leaseId does not move the confirmed leaseId back.
    infos[0].primaryNodeId()  = 1;
    infos[0].primaryLeaseId() = 5;
    obj.onLeaseIdsConfirmed(infos, g_cslTerm, g_cslTerm);
    BMQTST_ASSERT_EQ(6U, obj.confirmedLeaseId(0));

    // The snapshot publishes the confirmed primary node and leaseId.
    obj.raiseToConfirmed(&infos);
    BMQTST_ASSERT_EQ(3, infos[0].primaryNodeId());
    BMQTST_ASSERT_EQ(6U, infos[0].primaryLeaseId());

    // A higher leaseId is kept.
    infos[0].primaryNodeId()  = 2;
    infos[0].primaryLeaseId() = 7;
    obj.raiseToConfirmed(&infos);
    BMQTST_ASSERT_EQ(2, infos[0].primaryNodeId());
    BMQTST_ASSERT_EQ(7U, infos[0].primaryLeaseId());

    // No advisory while self sees a leaseId lower than the confirmed one.
    setCsl(&obj, true, true);
    obj.onPartitionPrimary(0, 5, false);
    BMQTST_ASSERT(!obj.needsAdvisory());

    // Self catches up.
    obj.onPartitionPrimary(0, 6, false);
    BMQTST_ASSERT(!obj.needsAdvisory());
    obj.onPartitionPrimary(0, 7, false);
    BMQTST_ASSERT(obj.needsAdvisory());
}

static void test10_cslCaughtUp()
{
    bmqtst::TestHelper::printTestName("CSL CAUGHT UP");

    ClusterAvailability obj(1, bmqtst::TestHelperUtil::allocator());

    ClusterAvailability::PartitionPrimaryInfos infos(
        bmqtst::TestHelperUtil::allocator());
    infos.resize(1);
    infos[0].partitionId()    = 0;
    infos[0].primaryNodeId()  = 1;
    infos[0].primaryLeaseId() = 5;

    // A leader in term 3; no advisory of term 3 applied.
    obj.onCslLeader(true, false, 3);
    BMQTST_ASSERT(!obj.isCslCaughtUp());

    // An advisory of an earlier term is ignored.
    obj.onLeaseIdsConfirmed(infos, 2, 3);
    BMQTST_ASSERT(!obj.isCslCaughtUp());
    BMQTST_ASSERT_EQ(0U, obj.confirmedLeaseId(0));

    // An advisory of the current term.
    obj.onLeaseIdsConfirmed(infos, 3, 3);
    BMQTST_ASSERT(obj.isCslCaughtUp());
    BMQTST_ASSERT_EQ(5U, obj.confirmedLeaseId(0));

    // No leader, then the leader comes back in the same term.
    obj.onCslLeader(false, false, 3);
    BMQTST_ASSERT(!obj.isCslCaughtUp());
    obj.onCslLeader(true, false, 3);
    BMQTST_ASSERT(obj.isCslCaughtUp());

    // The leader stops.
    obj.onCslLeaderStopping();
    BMQTST_ASSERT(!obj.isCslCaughtUp());

    // A new term.
    obj.onCslLeader(true, true, 4);
    BMQTST_ASSERT(!obj.isCslCaughtUp());
}

static void test8_actionToAscii()
{
    bmqtst::TestHelper::printTestName("ACTION TO ASCII");

    BMQTST_ASSERT_EQ(bsl::string("NONE"),
                     bsl::string(Action::toAscii(Action::e_NONE)));
    BMQTST_ASSERT_EQ(
        bsl::string("ACTIVATE_AND_BECOME_AVAILABLE"),
        bsl::string(Action::toAscii(Action::e_ACTIVATE_AND_BECOME_AVAILABLE)));
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    switch (_testCase) {
    case 0:
    case 10: test10_cslCaughtUp(); break;
    case 9: test9_confirmedNeverDecreases(); break;
    case 8: test8_actionToAscii(); break;
    case 7: test7_needsAdvisory(); break;
    case 6: test6_cslEventSeveralPartitions(); break;
    case 5: test5_otherRoles(); break;
    case 4: test4_syncPointAppliedFirst(); break;
    case 3: test3_selfPrimary(); break;
    case 2: test2_waitReasons(); break;
    case 1: test1_breathingTest(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
}
