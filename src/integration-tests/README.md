## Integration Tests

This directory contains integration tests based on the `ito` and 'bmqit'
frameworks. It contains the following test suites:


### `00breathing_test.py`

Provides a basic integration test suite.


### `20cluster_node_shutdown_test.py`

Integration test that shuts down a cluster node and confirms that the system
recovers and keeps working as expected.


### `20node_status_change_test.py`

Integration test that suspends a node and confirms that the system recovers
and performs as expected.

### `30leader_node_delay.py`

Integration test that temporarily suspends the leader node, resulting in
followers becoming leaderless. When leader is unpaused, they will re-discover
the leader but in PASSIVE state, and then in ACTIVE state once healing logic
kicks in.

### `50appids_test.py`

Integration test suite exercising AppIDs.


### `50broadcast_test.py`

Integration test suite exercising broadcast functionality.


### `50list_messages_test.py`

Integration test suite exercising list messages functionality.

### `50maxqueues_test.py`

Integration test suite exercising Max Queues functionality.
