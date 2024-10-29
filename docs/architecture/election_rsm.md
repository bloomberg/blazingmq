---
layout: default
title: Leader Election
parent: Architecture
nav_order: 6
---

# Leader Election and State Machine Replication in BlazingMQ
{: .no_toc }

* toc
{:toc}

## Summary

This articles introduces readers to the leader election algorithm in BlazingMQ.
The article explains the motivation for having a **leader** node in a given
BlazingMQ cluster, and then goes on to explain the leader election algorithm
implementation, which is inspired by the algorithm proposed in the
[*Raft*](https://raft.github.io/) consensus protocol.  The article also
discusses an extension to the leader election algorithm (what Raft refers to as
*PreVote*), which helps BlazingMQ's leader election implementation achieve
stability during network disruptions.  Note that *Raft* is a state machine
replication algorithm, and indeed, BlazingMQ's replication model is also
inspired by it, although with some important differences.  This article
primarily focuses only on BlazingMQ's leader election algorithm, and explains
state machine replication only briefly.  Similarly, differences with *Raft* are
discussed briefly.

---

## Introduction

Before we jump into the nitty-gritty of the leader election algorithm, let's
first look at why some distributed systems like BlazingMQ need a node in their
clusters to act as a coordinator or "leader".

A distributed system often needs to carry out some bookkeeping and management
of its workload as well as nodes in the cluster.  These responsibilities can
take various forms for different distributed systems, including but not limited
to:

- Keeping an eye on the health of nodes in the cluster (e.g., demoting an
  unhealthy node, synchronizing a newly joined node, etc.)

- Distributing work across nodes in the cluster (e.g., assigning a primary node
  to a stream of data, distributing ranges of keys across nodes in the cluster,
  etc.)

- Storing and replicating any metadata required to bootstrap the cluster upon
  restart

In order to avoid conflicting decisions, only one node in the cluster (the
"leader") should be authorized to carry out such work, while other nodes (the
"followers") must honor the decisions made by the leader node.

It is worth noting that not all distributed systems require a leader node to
work.  In such systems, each node can act as a coordinator for a request that
it receives from the user.  Such systems are out of the scope of this article.

### Responsibilities of the Leader Node in BlazingMQ

At this point, it is worth discussing the role of a leader node in BlazingMQ
cluster.  A BlazingMQ leader node performs the following responsibilities:

- **Managing Cluster Metadata**: A BlazingMQ cluster hosts its metadata itself,
  instead of relying on an external metadata store like *Apache ZooKeeper*,
  *etcd*, etc.  It is the responsibility of the leader to store and replicate
  metadata among nodes in the cluster.  See [*Cluster
  Metadata*](../clustering#cluster-metadata) section for more details about
  metadata.

- **Synchronizing New Nodes**: The leader node is also in charge of bringing a
  new node (which just restarted or joined the BlazingMQ cluster) up to date
  with cluster's metadata and other information which helps the new node catch
  up with other nodes.

- **Checking the Health of Peer Nodes**: Leader node keeps an eye on the health
  of peer nodes, and may choose to assign another node as the primary upon
  detecting a failing or crashed primary node.

---

## The Leader Election Algorithm in BlazingMQ

As mentioned previously, a BlazingMQ cluster elects a node as the leader which
acts as a coordinator for the cluster's metadata and healthiness of nodes.  The
leader node maintains a replicated state machine to ensure that the cluster's
metadata is persisted at the majority of the nodes.  This section explains the
leader election algorithm at a high level.  It is by no means exhaustive and
deliberately avoids any formal specification or proof.  Readers looking for an
exhaustive explanation should refer to the *Raft*
[paper](https://raft.github.io/raft.pdf), which acts as a strong inspiration
for BlazingMQ's leader election algorithm.

### Naïve Leader Election Algorithm

Instead of jumping into the final version of algorithm, let's try to write one,
starting from something simple.  Here's our first attempt:

1. When a node in the BlazingMQ cluster detects the absence of a leader, it
   proposes election by sending an `ElectionProposal` request to the peer
   nodes.

2. Peers can respond to the `ElectionProposal` request with a *yes* or a *no*,
   depending upon their elector state machine.

3. If the proposing node gets support from the majority of the peer nodes, it
   becomes the leader.  Here, majority would mean `N/2 + 1`, where `N` is the
   total number of nodes in the cluster.  Enforcing quorum ensures that at any
   given time, there is no more than one leader in the BlazingMQ cluster.

4. New leader starts sending periodic heartbeats to the peers ("followers").

5. Follower nodes periodically check for heartbeats from the leader.  If a
   configured number of consecutive heartbeats are missed, then the follower
   may assume that the leader node has disappeared, and may decide to propose
   an election immediately or after some time.


### Problems with the Naïve Algorithm

While the above algorithm looks reasonable enough, it has several problems:

- **Concurrent Election Proposals**: It cannot handle concurrent election
  proposals.  Let's say the two follower nodes notice that the leader has
  disappeared, and decide to propose an election at the same time.  As a
  result, other nodes would receive multiple election proposals, one from each
  of the nodes proposing an election.  How should a node decide which proposal
  to respond to with a *yes* and which one with a *no*?

- **Unneeded Election Proposals**: It cannot handle unneeded election
  proposals.  Let's say a follower node already views a node as the leader, but
  then receives an election proposal from another node.  Should the follower
  node continue to follow the original leader, or should it support the second
  one?  What if the second one has more up-to-date information?  There is no
  way for the follower node to figure that out from the proposal.

- **Stale Leader Detection**: It cannot detect a stale leader.  Let's say a
  follower node already views a node as the leader, but then starts receiving
  leader heartbeats from another node as well.  How does the follower node
  determine which of the two leaders is the stale one?

- **Ordering Leader Updates**: It cannot help nodes to order updates published
  by the leader.  Given two updates, there is no way to figure out which update
  originated from an old leader and which one from a new leader.


### Introducing Election *Term*

All of the above problems can be solved by introducing a monotonically
increasing integer in our election algorithm.  Here are the details:

- Every message exchanged within the algorithm (e.g., `ElectionProposal`,
  `ElectionProposalResponse`, `LeaderHeartbeat`, etc) will contain an integer.

- Every node will maintain its own copy of the integer.

- A node will *always* honor an election request/notification/heartbeat message
  if the message contains a higher value for this integer than its own copy
  (and will also update its own copy with the higher value).

This integer can be thought of as a logical clock, a generation count, a
fencing token, or as *Raft* calls it: an election *term*.


### Updated Algorithm using Term

Here's the updated algorithm which now uses a term:

1. When a node in the BlazingMQ cluster detects the absence of a leader, it
   proposes an election by sending an `ElectionProposal` request to the peer
   nodes containing `proposedTerm == ++selfTerm`.

2. A peer node responds to `ElectionProposal` with a:

   - *Yes* if `proposedTerm > peer's selfTerm`, and also updates `selfTerm =
     proposedTerm`.

   - *No* if `proposedTerm <= peer's selfTerm`

3. If the proposing node gets support from the majority of the peer nodes, it
   transitions to leader (at this point the leader's `selfTerm ==
   proposedTerm`).

4. The new leader starts sending periodic heartbeats to the peers
   ("followers").  Every `Heartbeat` message contains the leader's term.

5. Follower nodes periodically check for heartbeats from the leader and ensure
   that `selfTerm == term in heartbeat message`.  If a configured number of
   consecutive heartbeats are missed, then follower may assume that the leader
   node has disappeared.

Additional notes on elector term:

- One can see that every leader will have a unique term associated with it, and
  a newer leader will have a higher term than the previous leader.  This can
  help followers detect and ignore stale leaders.

- A leader can also use its term to create composite sequence numbers to be
  used in any application-level messages sent by the leader.  These sequence
  numbers can be of the form *(Term, Counter)* where *Counter* is an integer
  starting from zero for every new leader.  In BlazingMQ, these composite
  sequence numbers are referred to as *Leader Sequence Numbers* or *LSNs*.  So
  a leader with a term of 5 can issue messages with LSNs *(5, 1)*, *(5,2)* and
  so on.  LSNs can also be used to order messages from the leader as well as to
  ignore messages from stale leaders.  For example, a message LSN *(5, 2)* is
  newer than a message with LSN *(4, 1000)*.

- Lastly, what *Raft* refers to as a term is referred to as *view*, *ballot
  number*, *proposal number*, *round number*, *epoch*, etc by other similar
  systems like *Paxos*, *Apache ZooKeeper* etc.  Moreover, what we refer to as
  *Leader Sequence Number* in BlazingMQ is referred to as *Log Sequence
  Number*, *ZXID*, etc by other systems.


### Elector State Machine

At a given time, every participating node can be in one of the three states:

- *FOLLOWER*: a follower node is not the leader and has not proposed an
  election.  It may or may not be aware of the leader.

- *CANDIDATE*: a candidate node is not the leader, but has a pending election
  proposal.

- *LEADER*: a leader node is one which currently enjoys support of the majority
  of the nodes in the cluster.

Some general notes on these elector states:

- A node always starts as a *FOLLOWER* with `selfTerm = 0`.  Upon start up, the
  node waits a configured amount of time in order to discover the existing
  leader, if one exists.

- A *FOLLOWER* node will transition to *CANDIDATE* and propose an election if
  it cannot find a leader node in the configured time.

- A *CANDIDATE* waits for a configured amount of time to hear from all or the
  majority of its peers for its election proposal.

- If a *CANDIDATE* receives an election proposal or a heartbeat with higher
  term, the *CANDIDATE* transitions back to *FOLLOWER* and supports the node
  with higher term.

- Similarly, if a *LEADER* receives an election proposal or a heartbeat with
  higher term, it transitions back to *FOLLOWER* and supports the node with
  higher term.

A node will *always* honor an election proposal or heartbeat message with
higher term, irrespective of its role.

### Elector Algorithm: Node Startup Scenario

Let's walk through a common scenario where a node starts up and joins the peers
participating in the election:

1. The new node starts up as a *FOLLOWER* with `selfTerm = 0` and waits for the
   configured amount of time to discover the existing leader, if any.

2. If the node discovers a leader via heartbeat messages, it updates `selfTerm
   = leaderTerm`, and also sends an unsolicited `ElectionProposal` response to
   the leader node, letting it know that it supports the leader.

3. If the node does not find the leader node, it waits for some additional time
   (random interval between 0-3 seconds) before transitioning to a *CANDIDATE*
   and proposing an election with `++selfTerm`.  The reason for waiting a
   random time interval before proposing an election is explained in the third
   bullet of the next section.


### Elector Algorithm: Leader Crash Scenario

Let's walk through another common scenario where the leader node crashes or
stops gracefully:

1. Once a *FOLLOWER* node starts following a *LEADER* node, it schedules a
   periodic event to check for heartbeat messages from the leader.

2. If three consecutive heartbeats are missed, the follower waits for a random
   time interval (between 0-3 seconds) before proposing an election with
   `++selfTerm`.

3. The additional random wait interval prevents multiple *FOLLOWER* nodes from
   proposing elections simultaneously.  This helps to avoid failed election
   proposals, which helps with faster convergence to elect a new leader.


### Differences from *Raft*'s Consensus Algorithm

BlazingMQ's leader election and state machine replication differs from that of
*Raft* in one way: in *Raft*s leader election, only the node having the most
up-to-date log can become the leader.  If a follower receives an election
proposal from a node with stale view of the log, it will not support it.  This
ensures that the elected leader has up-to-date messages in the replicated
stream, and simply needs to sync up any followers which are not up to date.  A
good thing about this choice is that messages always flow from leader to
follower nodes.

BlazingMQ's elector implementation relaxes this requirement.  Any node in the
cluster can become a leader, irrespective of its position in the log.  This
adds additional complexity in that a new leader needs to synchronize its state
with the followers and that a follower node may need to send messages to the
new leader if the latter is not up to date.  However, this deviation from
*Raft* and the custom synchronization protocol comes in handy because it allows
BlazingMQ to avoid flushing (`fsync`) every message to disk.  Readers familiar with
Apache Kafka
[internals](https://jack-vanlightly.com/blog/2023/4/24/why-apache-kafka-doesnt-need-fsync-to-be-safe)
will see similarities between the two systems here.

An additional benefit of this deviation is the case when we specifically want a
particular node to become leader for testing, troubleshooting, or simulating
disaster recovery scenarios.  On some occasions, the target node could be
behind in the replicated state machine, and would not be elected as the leader
in the original *Raft* algorithm.

---

## Extending the Leader Election Algorithm

In the previous sections, we discussed a couple of scenarios (node start and
leader crash) and saw how the election algorithm handles them well.  Let's
discuss a couple of scenarios where the BlazingMQ algorithm could do better.

### Non-sticky Leader

![Non-sticky Leader](../../../assets/images/nonStickyLeader.png "Non-sticky
Leader")

The figure above demonstrates a setup where there are 4 nodes in a cluster.  In
the absence of any disturbance, all 4 nodes connect to each other over TCP,
creating full mesh.  However, let's assume that due to network disruption, node
B gets isolated from the other 3 nodes.  Let's also assume that node A is the
leader with a term of 10.  Note that even after node B gets isolated, node A
continues to enjoy support of the majority of the nodes in the cluster and thus
continues to be the leader as perceived by itself and nodes C and D.

However, since node B is isolated, it cannot see node A and its elector state
machine eventually transitions to *CANDIDATE*, bumps up it's local term to 11
and proposes an election.  Inevitably, this election round fails after a
timeout.  Node B then waits for random time interval and proposes an election
yet again, this time with a term of 12, which fails yet again.  The cycle
continues and node B's term keeps on incrementing with every failed election.

Now, let's assume that network heals and node B is no longer isolated from its
peers.  Node B will eventually receive heartbeats from node A with a term of
10, which node B will ignore since its own term is higher than that.
Additionally, node B will propose an election with a term higher than 10.
Nodes A, C, and D, upon receiving this proposal, will support node B because of
higher term in the proposal, and node A will transition from *LEADER* to
*FOLLOWER*.  Eventually, node B will transition to *LEADER* with the other 3
nodes following it.

While our algorithm works correctly (at a given time, there is only one leader)
one can see that there was an unnecessary leadership switch from node A to node
B.  It would be ideal if node A continued to be the leader, with node B simply
following it upon re-joining the cluster.

### Leader Ping-Pong

![Leader Ping-Pong](../../../assets/images/leaderPingPong.png "Leader Ping
Pong")

The figure above demonstrates another scenario which is more disruptive than
the one discussed in the previous section.

In this scenario, let's assume that node A is the leader with a term of 10 with
the other three nodes following it.  Then, due to a network disturbance, let's
assume that node B gets disconnected from node A, while still being connected
to nodes C and D.

In this scenario, node B will detect the dropped connection (or missed leader
heartbeats) from node A, and will eventually transition to *FOLLOWER* and then
become a *CANDIDATE* and propose an election with term 11.  Node A will not
receive this proposal, but nodes C and D will, and both of them will support
node B because of higher term in the election proposal.  Node B will get
support from the majority of the nodes and will transition to *LEADER*.

Node A will come to know that nodes C and D no longer support it as the leader
when it receives an `InvalidHeartbeat` message from one or both of them with
the latest term (11).  Node A will transition to *FOLLOWER*, and since it
cannot see the current leader (node B), it will propose an election with a term
of 12.  Again, nodes C and D will support this election, and node A will end up
being the leader with a term of 12.

The cycle will repeat with node B becoming a leader with a term of 13, then
node A becoming a leader with a term of 14, and so on.  Leadership will keep
bouncing between nodes A and B.  While at a given time, there will be only one
leader, this scenario can be extremely disruptive for an application.

### Solution: The `ElectionScouting` Request

The problems of *non-sticky leaders* and *leader ping-pong* discussed above can
be solved by introducing an `ElectionScouting` request (*Raft* calls this as
*PreVote* request):

1. A node, instead of sending an `ElectionProposal` request with `proposedTerm
   = ++selfTerm`, sends an `ElectionScouting` request with `proposedTerm =
   selfTerm + 1`.  In other words, the node specifies an incremented term in
   the `ElectionScouting` request but *does not* bump up its own term.  For
   example, a node with term 10 will specify term 11 in `ElectionScouting`
   request.  Logically, an `ElectionScouting` request asks this question to the
   peer nodes: *"If I were to propose an election with the specified term, will
   you support me?"*.

2. A node which receives `ElectionScouting` request responds as per this logic:
   - If it already perceives another node as the leader, it responds to the
     `ElectionScouting` request with a *No*.
   - If it does not perceive any node as the leader, it responds with a *Yes*
     if `selfNode < proposedNode`, otherwise a *No*.

Introducing the `ElectionScouting` request ensures that a node does not
needlessly bump up its own term every time it proposes an election, without
even knowing if peers will support its proposal or not.

Additionally, the logic in second bullet ensures that if a peer is already
aware of a leader, it will not honor an `ElectionScouting` request from another
node, even if the request contains a higher term.  Note that the rule of
honoring a message with higher term still applies to all other
requests/notifications/heartbeats, except for `ElectionScouting` requests.

Readers will notice that the two enhancements above ensure that the election
algorithm is no longer susceptible to the *non-sticky leader* and *leader
ping-pong* scenarios previously mentioned.

---

## Testing

Just like BlazingMQ's other subsystems, its leader election implementation (and
general replicated state machinery) is tested with unit and integration tests.
In addition, we periodically run chaos testing on BlazingMQ using our Jepsen
chaos testing suite, which we will be publishing soon as open source. We have
also tested our implementation with a TLA+
[specification](https://github.com/bloomberg/blazingmq/tree/main/etc/tlaplus)
for BlazingMQ's elector state machine.

---
