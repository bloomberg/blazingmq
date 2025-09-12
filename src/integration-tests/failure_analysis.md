# Analysis of logs for good and bad IT test runs
## Conditions
 - IT test: test_sync_after_rollover.py, run as:
 ```
python3 -m pytest -m "fsm_mode and multi and eventual_consistency" test_sync_after_rollover 
py::test_synch_after_missed_rollover -vvv
 ```
  - IT test logic
 ```
    Test replica journal file syncronization with cluster after missed rollover.
    - start cluster
    - put 2 messages
    - stop replica
    - put one message to initiate rollover
    - restart replica
    - check that replica is synchronized with primary (primary and replica journal files content is equal)
```

  - good run log file: `good_run.log`;
  - bad run log file: `bad_run.log`;

# Analysis of good run (`good_run.log`)
Form the point when replica is dropped its partition 0 and requests PrimaryState  

6493: Replica: `Sending request to '[east1, 1]' [request: [ rId = 6 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ primaryStateRequest ...`

6071: Primary: Got request: `Partition FSM for Partition [0] on Event 'PRIMARY_STATE_RQST'`

6077: Primary: Sent PrimaryState response: `sent response [ rId = 6 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ primaryStateResponse...`

6497: Replica: Got PrimaryState response: `Partition FSM for Partition [0] on Event 'PRIMARY_STATE_RSPN'`

6498: Replica: `In Partition [0]'s FSM, storing the sequence number of [east1, 1] as [ primaryLeaseId = 1 sequenceNumber = 17 ], firstSyncPointSeqNum: [ primaryLeaseId = 1 sequenceNumber = 13 ]`

6511: Replica: `received PrimaryStateResponse [ rId = 6 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ primaryStateResponse = [ partitionId = 0`

6079: Primary: send PUSH request `Sending request to '[east2, 2]' [request: [ rId = 32 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ replicaDataRequest = [ replicaDataType = E_PUSH`

6507: Replica: Got request `Partition FSM for Partition [0] on Event 'REPLICA_DATA_RQST_PUSH'`

Replica correctly gets firstSyncpointSeqNum from context (stored by log line 6498) 

Raw log from Replica
```
east2            12:23:01.595 (139881355396672) INFO     *rkr.bmqp.requestmanager bmqp_requestmanager.h:1359 Sending request to '[east1, 1]' [request: [ rId = 6 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ primaryStateRequest = [ partitionId = 0 latestSequenceNumber = [ primaryLeaseId = 0 sequenceNumber = 0 ] firstSyncPointSequenceNumber = [ primaryLeaseId = 0 sequenceNumber = 0 ] ] ] ] ] ] ] ], timeout: (10, 0)]
east2            12:23:01.595 (139881355396672) INFO     *qbrkr.mqbc.partitionfsm mqbc_partitionfsm.cpp:76  Partition FSM for Partition [0] on Event 'PRIMARY_STATE_RSPN', transition: State 'REPLICA_HEALING' =>  State 'REPLICA_HEALING'
east2            12:23:01.596 (139881355396672) INFO     *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:1433 Cluster (itCluster): In Partition [0]'s FSM, storing the sequence number of [east1, 1] as [ primaryLeaseId = 1 sequenceNumber = 17 ], firstSyncPointSeqNum: [ primaryLeaseId = 1 sequenceNumber = 13 ]
east2            12:23:01.597 (139881095353920) INFO     *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:563 Cluster (itCluster) Partition [0]: Received ReplicaDataRequestPush: [ rId = 32 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ replicaDataRequest = [ replicaDataType = E_PUSH partitionId = 0 beginSequenceNumber = [ primaryLeaseId = 0 sequenceNumber = 0 ] endSequenceNumber = [ primaryLeaseId = 1 sequenceNumber = 17 ] ] ] ] ] ] ] ] from [east1, 1].
east2            12:23:01.599 (139881355396672) INFO     *qbrkr.mqbc.partitionfsm mqbc_partitionfsm.cpp:76  Partition FSM for Partition [0] on Event 'REPLICA_DATA_RQST_PUSH', transition: State 'REPLICA_HEALING' =>  State 'REPLICA_HEALING'
east2            12:23:01.600 (139881355396672) INFO     *kr.mqbc.recoverymanager mqbc_recoverymanager.cpp:359 Cluster (itCluster) Partition [0]: Got notification to expect data chunks of range [ primaryLeaseId = 0 sequenceNumber = 0 ] to [ primaryLeaseId = 1 sequenceNumber = 17 ] from [east1, 1]. Recovery requestId is 32.
east2            12:23:01.600 (139881095353920) INFO     *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:747 Cluster (itCluster): received PrimaryStateResponse [ rId = 6 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ primaryStateResponse = [ partitionId = 0 latestSequenceNumber = [ primaryLeaseId = 1 sequenceNumber = 17 ] firstSyncPointSequenceNumber = [ primaryLeaseId = 1 sequenceNumber = 13 ] ] ] ] ] ] ] ] from [east1, 1]
east2            12:23:01.601 (139881355396672) WARNING  *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:3177 FOUND partitionId: 0 [ primaryLeaseId = 1 sequenceNumber = 13 ]
east2            12:23:01.601 (139881355396672) WARNING  *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:3191 do_updateStorage partitionId: 0 source: [east1, 1] firstSyncPointAfterRolloverSeqNum: [ primaryLeaseId = 1 sequenceNumber = 13 ]

```


# Analysis of bad run `bad_run.log`
Form the point when replica is dropped its partition 0 and requests PrimaryState  

6605: Replica: `Sending request to '[east1, 1]' [request: [ rId = 6 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ primaryStateRequest ...`

6177: Primary: Got request: `Partition FSM for Partition [0] on Event 'PRIMARY_STATE_RQST'`

6183: Primary: Sent PrimaryState response: `sent response [ rId = 6 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ primaryStateResponse...`  

**NO LOG FOR** `Partition FSM for Partition [0] on Event 'PRIMARY_STATE_RSPN'` and `storing the sequence number of [east1, 1]`

6511: Replica: `received PrimaryStateResponse [ rId = 6 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ primaryStateResponse = [ partitionId = 0`

6186: Primary: send PUSH request `Sending request to '[east2, 2]' [request: [ rId = 32 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ replicaDataRequest = [ replicaDataType = E_PUSH`

6621: Replica: Got request `Partition FSM for Partition [0] on Event 'REPLICA_DATA_RQST_PUSH'`

Replica cannot get firstSyncpointSeqNum from context (missed log `storing the sequence number of [east1, 1]`) 

Raw log from Replica
```
east2            13:19:39.824 (138912194360896) INFO     *rkr.bmqp.requestmanager bmqp_requestmanager.h:1359 Sending request to '[east1, 1]' [request: [ rId = 6 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ primaryStateRequest = [ partitionId = 0 latestSequenceNumber = [ primaryLeaseId = 0 sequenceNumber = 0 ] firstSyncPointSequenceNumber = [ primaryLeaseId = 0 sequenceNumber = 0 ] ] ] ] ] ] ] ], timeout: (10, 0)]
east2            13:19:39.824 (138912185968192) INFO     *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:2597 Cluster (itCluster) Partition [1]: Opened successfully, applying 0 buffered storage events to the partition.
east2            13:19:39.825 (138912185968192) INFO     *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:2667 Cluster (itCluster) Partition [1]: Processing 1 buffered primary status advisory.
east2            13:19:39.826 (138912185968192) INFO     *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:268 (format: [QueueUri] [QueueKey] [Num Msgs] [Num Virtual Storages] [Virtual Storages Details])
east2            13:19:39.827 (138912152397376) INFO     *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:563 Cluster (itCluster) Partition [0]: Received ReplicaDataRequestPush: [ rId = 32 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ replicaDataRequest = [ replicaDataType = E_PUSH partitionId = 0 beginSequenceNumber = [ primaryLeaseId = 0 sequenceNumber = 0 ] endSequenceNumber = [ primaryLeaseId = 1 sequenceNumber = 17 ] ] ] ] ] ] ] ] from [east1, 1].
east2            13:19:39.827 (138912194360896) INFO     *qbrkr.mqbc.partitionfsm mqbc_partitionfsm.cpp:76  Partition FSM for Partition [0] on Event 'REPLICA_DATA_RQST_PUSH', transition: State 'REPLICA_HEALING' =>  State 'REPLICA_HEALING'
east2            13:19:39.827 (138912194360896) INFO     *kr.mqbc.recoverymanager mqbc_recoverymanager.cpp:359 Cluster (itCluster) Partition [0]: Got notification to expect data chunks of range [ primaryLeaseId = 0 sequenceNumber = 0 ] to [ primaryLeaseId = 1 sequenceNumber = 17 ] from [east1, 1]. Recovery requestId is 32.
east2            13:19:39.828 (138912152397376) INFO     *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:747 Cluster (itCluster): received PrimaryStateResponse [ rId = 6 choice = [ clusterMessage = [ choice = [ partitionMessage = [ choice = [ primaryStateResponse = [ partitionId = 0 latestSequenceNumber = [ primaryLeaseId = 1 sequenceNumber = 17 ] firstSyncPointSequenceNumber = [ primaryLeaseId = 1 sequenceNumber = 13 ] ] ] ] ] ] ] ] from [east1, 1]
east2            13:19:39.828 (138912194360896) WARNING  *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:3179 Cluster (itCluster)do_updateStorage partitionId: [0]: No seqNum context found for source node: [east1, 1]. Using default firstSyncPointAfterRolloverSeqNum.
east2            13:19:39.828 (138912194360896) WARNING  *rkr.mqbc.storagemanager mqbc_storagemanager.cpp:3191 do_updateStorage partitionId: 0 source: [east1, 1] firstSyncPointAfterRolloverSeqNum: [ primaryLeaseId = 0 sequenceNumber = 0 ]

```

## Analysis
It seems the problem is that in bad run event `e_PRIMARY_STATE_RSPN` (logged by `mqbc_storagemanager.cpp:747 received PrimaryStateResponse [ rId = 6`) is not dispatched by some reason - there is no log ``Partition FSM for Partition [0] on Event 'PRIMARY_STATE_RSPN'``.  As a consequence, primary sequence numbers (last and firstSyncPoint) are not saved in context.

Also noticed that in both logs there is strange log `Partition FSM for Partition [0] on Event 'PRIMARY_STATE_RSPN', transition: State 'REPLICA_HEALED' =>  State 'REPLICA_HEALED'` which is skipped in `REPLICA_HEALED` state.
 - `good_run.log` - line 6600
 - `bad_run.log` - line 6710
 Who did send the request? There is no rId to check
 

## Issue to clarify
 - why does log `on Event 'PRIMARY_STATE_RSPN` appear before `received PrimaryStateResponse` (which is logged from mqbc_storagemanager.cpp:747 before calling  `dispatchEventToPartition`)
