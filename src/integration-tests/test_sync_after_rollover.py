# Copyright 2025 Bloomberg Finance L.P.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Testing primary-replica synchronization after missed rollover.
"""

import glob
from os import EX_OK
import time
from pathlib import Path
import subprocess


import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    cluster,
    multi_node,
    single_node,
    tweak,
    start_cluster,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.client import Client
from blazingmq.dev.it.process.proc import Process

from blazingmq.dev import paths


@tweak.cluster.partition_config.max_journal_file_size(1004) #524
def test_synch_after_missed_rollover(cluster: Cluster, domain_urls: tc.DomainUrls,
                                                        ) -> None:
    """
    Test replica journal file syncronization with cluster after missed rollover.
    - start cluster
    - put messages
    - kill replica
    - put messages to initiate rollover
    - restart replica
    - check that replica is synchronized with primary
    """
    uri_priority = domain_urls.uri_priority

    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())

    # Create producer and consumer
    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write,ack"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(uri_priority, flags=["read"], succeed=True)

    replicas = cluster.nodes(exclude=leader)
    replica = replicas[0]
    print(f"MyReplica: {replica.name}. leader: {leader.name}")

    # Post 2 messages (no rollover yet)
    for i in range(1, 3):
        # time.sleep(1)  
        producer.post(uri_priority, [f"msg{i}"], succeed=True, wait_ack=True)

        consumer.wait_push_event()
        consumer.confirm(uri_priority, "*", succeed=True)

    # Kill replica
    # replica.check_exit_code = False
    # replica.kill()
    # replica.wait()

    # Stop replica
    replica.stop()
    cluster.make_sure_node_stopped(replica)
    replica.drain()

    # Post 1 more message to initiate rollover
    producer.post(uri_priority, ["msg3"], succeed=True, wait_ack=True)
    producer.post(uri_priority, ["msg4"], succeed=True, wait_ack=True)
    producer.post(uri_priority, ["msg5"], succeed=True, wait_ack=True)

    # time.sleep(3)  
    # time.sleep(2)
    # leader.outputs_regex(r"ROLLOVER COMPLETE", 20)

    # Wait until rollover completed
    assert leader.outputs_substr("ROLLOVER COMPLETE", 10)

    # Restart the stopped replica which missed rollover
    replica.start()
    replica.wait_until_started()

    # print("MyReplica started")

    # leader.outputs_substr("FAIL_REPLICA_DATA_RSPN_DROP", 10)
    # print("MyReplica aborted")
    # replica.start()
    # print("MyReplica re-started")
    
    # # Test assert
    # time.sleep(30)
    assert replica.outputs_substr("Cluster (itCluster) is available", 10)

    leader = cluster.last_known_leader
    print(f"MyReplica: {replica.name}. NEW leader: {leader.name}")

    leader_journal_files_after_rollover = glob.glob(
            str(cluster.work_dir.joinpath(leader.name, "storage")) + "/*journal*")
    replica_journal_files_after_rollover = glob.glob(
            str(cluster.work_dir.joinpath(replica.name, "storage")) + "/*journal*")

    # assert len(journal_file_after_rollover) == 1
    # print("leader_journal_file_after_rollover: ", sorted(leader_journal_files_after_rollover))
    # print("replica_journal_file_after_rollover: ", sorted(replica_journal_files_after_rollover))
    for leader_file, replica_file in zip(sorted(leader_journal_files_after_rollover), sorted(replica_journal_files_after_rollover)):
        print(f"Leader journal file {Path(leader_file).name} : {Path(leader_file).stat().st_size}")
        print(f"Replica journal file {Path(replica_file).name} : {Path(replica_file).stat().st_size}")

        leader_res = subprocess.run(
            [paths.required_paths.storagetool, "--journal-file", leader_file, "--details", "--print-mode=json-pretty"],
            capture_output=True,
            check=True,
        )
        assert leader_res.returncode == 0
        # print("LEADER: ", leader_res.stdout)

        replica_res = subprocess.run(
            [paths.required_paths.storagetool, "--journal-file", replica_file, "--details", "--print-mode=json-pretty"],
            capture_output=True,
            check=True,
        )
        assert replica_res.returncode == 0
        # print("REPLICA: ", replica_res.stdout)     
        
        # assert leader_res.stdout == replica_res.stdout
        if leader_res.stdout != replica_res.stdout:
            print("LEADER: ", leader_res.stdout)
            print("REPLICA: ", replica_res.stdout)     

        assert leader_res.stdout == replica_res.stdout

              
    assert False, "Test assert"
