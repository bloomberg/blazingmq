// Copyright 2025 Bloomberg Finance L.P.
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

// mqbc_clusterutil.t.cpp                                             -*-C++-*-

// MQB
#include <mqbc_clusterutil.h>
#include <mqbi_cluster.h>
#include <mqbi_queueengine.h>
#include <mqbmock_cluster.h>

// BMQ
#include <bmqp_protocolutil.h>

// BDE
#include <bdlb_print.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsla_annotations.h>

// TEST DRIVER
#include <bmqtst_testhelper.h>

// CONVENIENCE
using namespace BloombergLP;
using namespace bsl;

// CLASSES
// =============
// struct Tester
// =============

struct Tester {
  private:
    // DATA
    bslma::Allocator* d_allocator_p;
    mqbmock::Cluster  d_cluster;

  public:
    // CREATORS
    Tester(bslma::Allocator* allocator = bmqtst::TestHelperUtil::allocator())
    : d_allocator_p(bslma::Default::allocator(allocator))
    , d_cluster(d_allocator_p)
    {
        // NOTHING
    }

    mqbi::Cluster* cluster() { return &d_cluster; }

    bslma::Allocator* allocator() const { return d_allocator_p; }

    mqbc::ClusterState::DomainStateSp createDomainState()
    {
        mqbc::ClusterState::DomainStateSp domainState;
        domainState.createInplace(d_allocator_p, d_allocator_p);
        return domainState;
    }

    mqbc::ClusterState::QueueInfoSp createQueueInfoSp(
        const bsl::string&      uriString,
        const mqbu::StorageKey& key,
        int                     partitionId,
        BSLA_UNUSED const mqbc::ClusterState::AppInfos& appIdInfos)
    {
        bmqp_ctrlmsg::QueueInfo advisory(bmqtst::TestHelperUtil::allocator());

        advisory.uri() = uriString;
        key.loadBinary(&advisory.key());
        advisory.partitionId() = partitionId;
        mqbc::ClusterState::QueueInfoSp queueInfo;
        queueInfo.createInplace(d_allocator_p, advisory, d_allocator_p);
        return queueInfo;
    }
};

/// This class provides the mock cluster and other components necessary to
/// test the cluster state manager in isolation, as well as some helper
/// methods.

// ============================================================================
//                                    TESTS
// ----------------------------------------------------------------------------

static void test1_validateState()
// ------------------------------------------------------------------------
// VALIDATE STATE
//
// Concerns:
//   Ensure proper behavior of 'validateState' method.
//
// Testing:
//   validateState(...)
// ------------------------------------------------------------------------
{
    bmqtst::TestHelper::printTestName("VALIDATE STATE");

    Tester tester;

    // We need to generate two different states and make sure we have the
    // expected outputs
    mqbc::ClusterState original(tester.cluster(), 5, tester.allocator());
    mqbc::ClusterState reference(tester.cluster(), 5, tester.allocator());

    // 0. Generate different and same primary lease Id
    original.setPartitionPrimary(0, 10, 0);
    reference.setPartitionPrimary(0, 9, 0);
    original.setPartitionPrimary(1, 20, 0);
    reference.setPartitionPrimary(1, 20, 0);

    // 1. Generate extraQueues from an extra domain
    bsl::string domainExtraDomain = "domain.extra.domain";
    original.domainStates().emplace(domainExtraDomain,
                                    tester.createDomainState());

    // 2. Generate extraQueues from a corrrect domain
    bsl::string domainExtraQueue = "domain.extra.queue";
    bsl::string queueExtraQueue  = "bmq://" + domainExtraQueue + "/qqq";
    bsl::string keyExtraQueue    = "extra.queue";

    mqbu::StorageKey extraQueueKey(mqbu::StorageKey::BinaryRepresentation(),
                                   keyExtraQueue.data());
    mqbc::ClusterState::QueueInfoSp extraQueueInfoSp =
        tester.createQueueInfoSp(queueExtraQueue,
                                 extraQueueKey,
                                 2,
                                 mqbc::ClusterState::AppInfos());

    mqbc::ClusterState::DomainStateSp extraQueueDomainStateSp =
        tester.createDomainState();
    mqbc::ClusterState::DomainStateSp extraQueueDomainStateRefSp =
        tester.createDomainState();
    extraQueueDomainStateSp->queuesInfo().emplace(queueExtraQueue,
                                                  extraQueueInfoSp);

    original.domainStates().emplace(domainExtraQueue, extraQueueDomainStateSp);
    reference.domainStates().emplace(domainExtraQueue,
                                     extraQueueDomainStateRefSp);

    // 3. Generate incorrect queues
    bsl::string domainIncorrectQueue = "domain.incorrect.queue";
    bsl::string queueIncorrectQueue = "bmq://" + domainIncorrectQueue + "/qqq";
    bsl::string keyIncorrectQueue   = "incorrect.queue";
    bsl::string keyIncorrectQueueRef = "reference.incorrect.queue";

    // origin
    mqbu::StorageKey incorrectQueueKey(
        mqbu::StorageKey::BinaryRepresentation(),
        keyIncorrectQueue.data());
    mqbc::ClusterState::QueueInfoSp incorrectQueueInfoSp =
        tester.createQueueInfoSp(queueIncorrectQueue,
                                 incorrectQueueKey,
                                 3,
                                 mqbc::ClusterState::AppInfos());

    mqbc::ClusterState::DomainStateSp incorrectQueueDomainStateSp =
        tester.createDomainState();
    incorrectQueueDomainStateSp->queuesInfo().emplace(queueIncorrectQueue,
                                                      incorrectQueueInfoSp);

    original.domainStates().emplace(domainIncorrectQueue,
                                    incorrectQueueDomainStateSp);

    // reference
    mqbu::StorageKey incorrectQueueKeyRef(
        mqbu::StorageKey::BinaryRepresentation(),
        keyIncorrectQueueRef.data());
    mqbc::ClusterState::QueueInfoSp incorrectQueueInfoRefSp =
        tester.createQueueInfoSp(queueIncorrectQueue,
                                 incorrectQueueKeyRef,
                                 3,
                                 mqbc::ClusterState::AppInfos());

    mqbc::ClusterState::DomainStateSp incorrectQueueDomainStateRefSp =
        tester.createDomainState();
    incorrectQueueDomainStateRefSp->queuesInfo().emplace(
        queueIncorrectQueue,
        incorrectQueueInfoRefSp);

    reference.domainStates().emplace(domainIncorrectQueue,
                                     incorrectQueueDomainStateRefSp);

    // 4. Generate a missing queue
    bsl::string domainMissingQueue = "domain.missing.queue";
    bsl::string queueMissingQueue  = "bmq://" + domainMissingQueue + "/qqq";
    bsl::string keyMissingQueue    = "missing.queue";

    mqbu::StorageKey missingQueueKey(mqbu::StorageKey::BinaryRepresentation(),
                                     keyMissingQueue.data());
    mqbc::ClusterState::QueueInfoSp missingQueueInfoSp =
        tester.createQueueInfoSp(queueMissingQueue,
                                 missingQueueKey,
                                 4,
                                 mqbc::ClusterState::AppInfos());

    mqbc::ClusterState::DomainStateSp missingQueueDomainStateSp =
        tester.createDomainState();
    mqbc::ClusterState::DomainStateSp missingQueueDomainStateRefSp =
        tester.createDomainState();
    missingQueueDomainStateRefSp->queuesInfo().emplace(queueMissingQueue,
                                                       missingQueueInfoSp);

    original.domainStates().emplace(domainMissingQueue,
                                    missingQueueDomainStateSp);
    reference.domainStates().emplace(domainMissingQueue,
                                     missingQueueDomainStateRefSp);

    // 5. Generate two correct queues.  One of them has null appIds in the
    // original state and default appId in the reference state; they should be
    // treated as equivalent.
    bsl::string domainCorrectQueue = "domain.correct.queue";
    bsl::string queueCorrectQueue  = "bmq://" + domainCorrectQueue + "/qqq";
    bsl::string keyCorrectQueue    = "correct.queue";
    mqbu::StorageKey correctQueueKey(mqbu::StorageKey::BinaryRepresentation(),
                                     keyCorrectQueue.data());
    mqbc::ClusterState::QueueInfoSp correctQueueInfoSp =
        tester.createQueueInfoSp(queueCorrectQueue,
                                 correctQueueKey,
                                 5,
                                 mqbc::ClusterState::AppInfos());

    bsl::string queueCorrectQueue2 = "bmq://" + domainCorrectQueue + "/qqq2";
    bsl::string keyCorrectQueue2   = "correct.queue2";
    mqbu::StorageKey correctQueueKey2(mqbu::StorageKey::BinaryRepresentation(),
                                      keyCorrectQueue2.data());
    mqbc::ClusterState::QueueInfoSp correctQueueInfoSp2NullApp =
        tester.createQueueInfoSp(queueCorrectQueue2,
                                 correctQueueKey2,
                                 6,
                                 mqbc::ClusterState::AppInfos());
    mqbc::ClusterState::AppInfos defaultAppInfos;
    defaultAppInfos.insert(bsl::make_pair(
        bsl::string(bmqp::ProtocolUtil::k_DEFAULT_APP_ID, tester.allocator()),
        mqbi::QueueEngine::k_DEFAULT_APP_KEY));
    mqbc::ClusterState::QueueInfoSp correctQueueInfoSp2DefaultApp =
        tester.createQueueInfoSp(queueCorrectQueue2,
                                 correctQueueKey2,
                                 6,
                                 defaultAppInfos);

    mqbc::ClusterState::DomainStateSp correctQueueDomainStateSp =
        tester.createDomainState();
    mqbc::ClusterState::DomainStateSp correctQueueDomainStateRefSp =
        tester.createDomainState();
    correctQueueDomainStateSp->queuesInfo().emplace(queueCorrectQueue,
                                                    correctQueueInfoSp);
    correctQueueDomainStateSp->queuesInfo().emplace(
        queueCorrectQueue,
        correctQueueInfoSp2NullApp);
    correctQueueDomainStateRefSp->queuesInfo().emplace(queueCorrectQueue,
                                                       correctQueueInfoSp);
    correctQueueDomainStateRefSp->queuesInfo().emplace(
        queueCorrectQueue,
        correctQueueInfoSp2DefaultApp);
    original.domainStates().emplace(domainCorrectQueue,
                                    correctQueueDomainStateSp);
    reference.domainStates().emplace(domainCorrectQueue,
                                     correctQueueDomainStateRefSp);

    // validate state
    bmqu::MemOutStream errorDescription;
    const int          rc = mqbc::ClusterUtil::validateState(errorDescription,
                                                    original,
                                                    reference);
    BMQTST_ASSERT_NE(rc, 0);

    bmqu::MemOutStream out;
    const int          level = 0;

    bdlb::Print::newlineAndIndent(out, level);
    out << "---------------------------";
    bdlb::Print::newlineAndIndent(out, level);
    out << "Incorrect Partition Infos :";
    bdlb::Print::newlineAndIndent(out, level);
    out << "---------------------------";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "Partition [0]:  primaryLeaseId: 10, primaryNodeId: -1";

    bdlb::Print::newlineAndIndent(out, level);
    out << "--------------------------------";
    bdlb::Print::newlineAndIndent(out, level);
    out << "Partition Infos In Cluster State :";
    bdlb::Print::newlineAndIndent(out, level);
    out << "--------------------------------";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "Partition [0]:  primaryLeaseId: 9, primaryNodeId: -1";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "Partition [1]:  primaryLeaseId: 20, primaryNodeId: -1";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "Partition [2]:  primaryLeaseId: 0, primaryNodeId: -1";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "Partition [3]:  primaryLeaseId: 0, primaryNodeId: -1";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "Partition [4]:  primaryLeaseId: 0, primaryNodeId: -1";

    bdlb::Print::newlineAndIndent(out, level);
    out << "-----------------";
    bdlb::Print::newlineAndIndent(out, level);
    out << "Incorrect Queues :";
    bdlb::Print::newlineAndIndent(out, level);
    out << "-----------------";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "[ uri = " << queueIncorrectQueue << " queueKey = ";
    bdlb::Print::singleLineHexDump(out,
                                   keyIncorrectQueue.begin(),
                                   mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    out << " partitionId = 3 appIdInfos = [ ] "
           "stateOfAssignment = NONE ]";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "(correct queue info) [ uri = " << queueIncorrectQueue
        << " queueKey = ";
    bdlb::Print::singleLineHexDump(out,
                                   keyIncorrectQueueRef.begin(),
                                   mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    out << " partitionId = 3 appIdInfos = [ ] "
           "stateOfAssignment = NONE ]";

    bdlb::Print::newlineAndIndent(out, level);
    out << "--------------";
    bdlb::Print::newlineAndIndent(out, level);
    out << "Extra queues :";
    bdlb::Print::newlineAndIndent(out, level);
    out << "--------------";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "[ uri = " << queueExtraQueue << " queueKey = ";
    bdlb::Print::singleLineHexDump(out,
                                   keyExtraQueue.begin(),
                                   mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    out << " partitionId = 2 appIdInfos = [ ] "
           "stateOfAssignment = NONE ]";

    bdlb::Print::newlineAndIndent(out, level);
    out << "----------------";
    bdlb::Print::newlineAndIndent(out, level);
    out << "Missing queues :";
    bdlb::Print::newlineAndIndent(out, level);
    out << "----------------";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "[ uri = " << queueMissingQueue << " queueKey = ";
    bdlb::Print::singleLineHexDump(out,
                                   keyMissingQueue.begin(),
                                   mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    out << " partitionId = 4 appIdInfos = [ ] "
           "stateOfAssignment = NONE ]";

    bdlb::Print::newlineAndIndent(out, level);
    out << "-------------------------";
    bdlb::Print::newlineAndIndent(out, level);
    out << "QUEUES IN CLUSTER STATE :";
    bdlb::Print::newlineAndIndent(out, level);
    out << "-------------------------";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "[ uri = " << queueCorrectQueue << " queueKey = ";
    bdlb::Print::singleLineHexDump(out,
                                   keyCorrectQueue.begin(),
                                   mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    out << " partitionId = 5 appIdInfos = [ ] "
           "stateOfAssignment = NONE ]";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "[ uri = " << queueMissingQueue << " queueKey = ";
    bdlb::Print::singleLineHexDump(out,
                                   keyMissingQueue.begin(),
                                   mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    out << " partitionId = 4 appIdInfos = [ ] "
           "stateOfAssignment = NONE ]";
    bdlb::Print::newlineAndIndent(out, level + 1);
    out << "[ uri = " << queueIncorrectQueue << " queueKey = ";
    bdlb::Print::singleLineHexDump(out,
                                   keyIncorrectQueueRef.begin(),
                                   mqbu::StorageKey::e_KEY_LENGTH_BINARY);
    out << " partitionId = 3 appIdInfos = [ ] "
           "stateOfAssignment = NONE ]";

    BMQTST_ASSERT_EQ(errorDescription.str(), out.str());
}

// ============================================================================
//                                 MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    TEST_PROLOG(bmqtst::TestHelper::e_DEFAULT);

    bmqt::UriParser::initialize(bmqtst::TestHelperUtil::allocator());

    switch (_testCase) {
    case 0:
    case 1: test1_validateState(); break;
    default: {
        cerr << "WARNING: CASE '" << _testCase << "' NOT FOUND." << endl;
        bmqtst::TestHelperUtil::testStatus() = -1;
    } break;
    }

    bmqt::UriParser::shutdown();

    TEST_EPILOG(bmqtst::TestHelper::e_CHECK_GBL_ALLOC);
    // Can't ensure no default memory is allocated because
    // 'bdlmt::EventSchedulerTestTimeSource' inside 'mqbmock::Cluster' uses
    // the default allocator in its constructor.
}
