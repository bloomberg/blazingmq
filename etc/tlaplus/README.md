# Leader Election and State Machine Replication in BlazingMQ

## Introduction

BlazingMQ's leader election and state machine replication is strongly inspired
by [*Raft*](https://raft.github.io/raft.pdf) consensus algorithm, but differs
from Raft in some important ways:

- It introduces an `ElectionScouting` request, an enhancement suggested by the
  author of Raft in *"Section 9.6 Preventing disruptions when a server rejoins
  the cluster"* in their Raft
  [thesis](https://web.stanford.edu/~ouster/cgi-bin/papers/OngaroPhD.pdf).
  Note that the author refers to the `ElectionScouting` request as a
  *Pre-Vote*.

- It introduces a `LeadershipCession` notification.  In a scenario where a node
  gives up its leadership and transitions to follower (upon receiving a
  heartbeat from another leader node with higher term), it can be helpful if
  the node notifies all of its peers immediately that it has ceded the
  leadership.  We introduced this notification to help other nodes converge
  quickly so that a new round of election, if needed, can finish quickly,
  thereby reducing fail-over time.

- Raft protocol prevents a node from being elected as the leader unless its log
  is at least as up-to-date as any other log in a majority.  In BlazingMQ, any
  node can become a leader irrespective of its log's status.  This adds the
  additional complexity in that a new leader needs to synchronize its state
  with the followers and that a follower node may need to send messages to the
  new leader if the later is not up to date.  However, this deviation from Raft
  and the custom synchronization protocol comes in handy because it allows
  BlazingMQ to avoid flushing (`fsync`) every message to disk.  Readers
  familiar with Apache Kafka
  [internals](https://jack-vanlightly.com/blog/2023/4/24/why-apache-kafka-doesnt-need-fsync-to-be-safe)
  will see similarities between the two systems here.

  An additional benefit of this deviation is that when we specifically want a
  particular node to become leader for testing, troubleshooting or simulating
  DR scenarios.  On some occasions, the target node could be behind in the
  replicated state machine, and will not be elected as the leader as per
  original *Raft* algorithm.

We modelled our protocol including these extensions in
[TLA+](https://lamport.azurewebsites.net/tla/tla.html), a language to model and
verify concurrent and distributed systems.  We verified with TLA+ that our
implementation did not lead to a state where there was more than one leader at
a time.

---

## Testing the specification

You can use TLC to test the specification via the [TLA+
Toolbox](https://lamport.azurewebsites.net/tla/toolbox.html) or the command
line.  You may have to disable *'Deadlock Check'* when using either option.

See [here](https://learntla.com/topics/cli.html) for instructions on running
TLC from the command line, which you can use with the associated configuration
file (`BlazingMQLeaderElection.cfg`).

---

## References

- Original Raft [spec](https://github.com/ongardie/raft.tla/blob/master/raft.tla)

- Reduced Raft
  [spec](https://github.com/Vanlightly/raft-tlaplus/blob/main/specifications/standard-raft/Raft.tla)
  by Jack Vanlightly
