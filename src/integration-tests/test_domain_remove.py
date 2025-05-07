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
This suite of test cases verifies the admin command
"DOMAINS REMOVE <domain> [FINALIZE]" work as expected
"""

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (
    multi_node,
    single_node,
    cluster,
    Cluster,
)
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.client import Client
import time


def write_messages(proxy, uri, n_msgs=5, do_confirm=True):
    """
    producer send a message, client confirm, then both close connection
    """
    producer = proxy.create_client("producer")
    producer.open(uri, flags=["write"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(uri, flags=["read"], succeed=True)

    producer.post(uri, [f"msg{i}" for i in range(n_msgs)], succeed=True, wait_ack=True)

    if do_confirm:
        consumer.confirm(uri, "*", succeed=True)

    producer.close(uri, succeed=True)
    consumer.close(uri, succeed=True)


def test_remove_domain_when_cluster_unhealthy(
    multi_node: Cluster, domain_urls: tc.DomainUrls
):
    """
    send DOMAINS REMOVE command when the cluster is not healthy
    the command fails with a routing error
    resend the command and it should succeed
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    du = domain_urls
    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)

    # find the two nodes which are not the known leader
    leader = multi_node.last_known_leader
    replicas = multi_node.nodes(exclude=leader)
    member = replicas[0]

    write_messages(proxy, du.uri_priority, n_msgs=5, do_confirm=False)

    # set quorum to make it impossible to select a leader
    for node in multi_node.nodes():
        node.set_quorum(99, succeed=True)

    # kill the leader to make the cluster unhealthy
    leader.check_exit_code = False
    leader.kill()
    leader.wait()

    # send remove domain admin command
    # command couldn't go through since state is unhealthy
    admin = AdminClient()
    admin.connect(member.config.host, int(member.config.port))  # member = east2
    res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")
    assert "Error occurred routing command to this node" in res
    assert res.split("\n").count("No queue purged.") == 3

    # restart the previous leader node
    # set quorum to make a member become the leader
    # wait until the cluster become healthy again
    leader.start()
    leader.wait()
    replicas[1].set_quorum(1)  # Quorum set to 1 from 99
    leader.wait_status(wait_leader=True, wait_ready=False)  # new leader = west1

    # send DOMAINS REMOVE admin command again
    multi_node._logger.info("BEFORE SENDING ADMIN COMMAND AGAIN")
    res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")
    assert "Purged 5 message(s) for a total of 20  B from 1 queue(s):" in res
    assert res.split("\n").count("No queue purged.") == 3


def test_remove_different_domain(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
    """
    send DOMAINS REMOVE command to remove a different domain
    the original one should be intact
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    proxies = cluster.proxy_cycle()

    # open queue in PRIORITY domain but remove PRIORITY_SC
    # producer produces messages and then closes connection
    producer = next(proxies).create_client("producer")
    producer.open(tc.URI_PRIORITY, flags=["write"], succeed=True)

    producer.post(
        tc.URI_PRIORITY,
        [f"msg{i}" for i in range(3)],
        succeed=True,
        wait_ack=True,
    )

    # send DOMAINS REMOVE admin command to a different domain
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))

    res = admin.send_admin(f"DOMAINS REMOVE {tc.DOMAIN_PRIORITY_SC}")
    assert "No queue purged." in res

    # post message to the untouched domain
    producer.post(
        tc.URI_PRIORITY,
        [f"msg{i}" for i in range(5)],
        succeed=True,
        wait_ack=True,
    )

    # do the same things for a different pair reversely
    # open queue in FANOUT_SC domain but remove FANOUT
    producer.open(tc.URI_FANOUT_SC, flags=["write"], succeed=True)

    producer.post(
        tc.URI_FANOUT_SC,
        [f"msg{i}" for i in range(3)],
        succeed=True,
        wait_ack=True,
    )

    res = admin.send_admin(f"DOMAINS REMOVE {tc.DOMAIN_FANOUT}")
    assert "No queue purged." in res

    # post message to the unremoved domain
    producer.post(
        tc.URI_FANOUT_SC,
        [f"msg{i}" for i in range(5)],
        succeed=True,
        wait_ack=True,
    )


def test_open_queue_after_remove_domain(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    try to open a queue after the first round of DOMAINS REMOVE command
    and it should fail since we started remove but not fully finished yet
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    du = domain_urls
    proxies = cluster.proxy_cycle()
    next(proxies)  # eastp
    proxy = next(proxies)  # westp

    uri = du.uri_priority
    # producer produces messages and consumer confirms
    # then both close connections
    producer = proxy.create_client("producer")
    producer.open(uri, flags=["write"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(uri, flags=["read"], succeed=True)

    producer.post(
        uri,
        [f"msg{i}" for i in range(3)],
        succeed=True,
        wait_ack=True,
    )
    consumer.confirm(uri, "*", succeed=True)
    producer.close(uri, succeed=True)
    consumer.close(uri, succeed=True)

    # send remove domain admin command
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")

    # open queues on the removed domain should fail
    assert producer.open(uri, flags=["write"], block=True) != Client.e_SUCCESS


def test_remove_domain_with_producer_queue_open(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    issue DOMAINS REMOVE command when consumer closes connection
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    du = domain_urls
    proxies = cluster.proxy_cycle()
    proxy = next(proxies)

    # producer produces messages and consumer confirms
    producer = proxy.create_client("producer")
    producer.open(du.uri_priority, flags=["write"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(du.uri_priority, flags=["read"], succeed=True)

    producer.post(
        du.uri_priority,
        [f"msg{i}" for i in range(3)],
        succeed=True,
        wait_ack=True,
    )
    consumer.confirm(du.uri_priority, "*", succeed=True)

    # consumer closes connection
    consumer.close(du.uri_priority, succeed=True)

    # send admin command
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")
    assert (
        f"Trying to remove the domain '{du.domain_priority}' while there are queues opened or opening"
        in res
    )


def test_remove_domain_with_consumer_queue_open(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    issue DOMAINS REMOVE command when producer closes connection
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    du = domain_urls
    proxies = cluster.proxy_cycle()
    proxy = next(proxies)

    # producer produces messages and consumer confirms
    producer = proxy.create_client("producer")
    producer.open(du.uri_priority, flags=["write"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(du.uri_priority, flags=["read"], succeed=True)

    producer.post(
        du.uri_priority,
        [f"msg{i}" for i in range(3)],
        succeed=True,
        wait_ack=True,
    )
    consumer.confirm(du.uri_priority, "*", succeed=True)

    # producer closes connection
    producer.close(du.uri_priority, succeed=True)

    # send admin command
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")
    assert (
        f"Trying to remove the domain '{du.domain_priority}' while there are queues opened or opening"
        in res
    )


def test_remove_domain_with_both_queue_open_and_closed(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    issue DOMAINS REMOVE command when both producer and consumer have queue open
    and both have queue closed
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    du = domain_urls
    proxies = cluster.proxy_cycle()
    proxy = next(proxies)

    # producer produces messages and consumer confirms
    producer = proxy.create_client("producer")
    producer.open(du.uri_priority, flags=["write"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(du.uri_priority, flags=["read"], succeed=True)

    producer.post(
        du.uri_priority,
        [f"msg{i}" for i in range(3)],
        succeed=True,
        wait_ack=True,
    )
    consumer.confirm(du.uri_priority, "*", succeed=True)

    # send admin command
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")
    assert (
        f"Trying to remove the domain '{du.domain_priority}' while there are queues opened or opening"
        in res
    )

    # close connections and try again
    producer.close(du.uri_priority, succeed=True)
    consumer.close(du.uri_priority, succeed=True)

    res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")
    assert "Purged 0 message(s)" in res


def test_try_open_removed_domain(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    1. producer send messages and consumer confirms
    2. send DOMAINS REMOVE admin command
    3. close both producer and consumer
    4. try open both, and they should all fail
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    du = domain_urls
    proxies = cluster.proxy_cycle()
    proxy = next(proxies)

    # producer produces messages and consumer confirms
    producer = proxy.create_client("producer")
    producer.open(du.uri_priority, flags=["write"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(du.uri_priority, flags=["read"], succeed=True)

    producer.post(
        du.uri_priority,
        [f"msg{i}" for i in range(3)],
        succeed=True,
        wait_ack=True,
    )
    consumer.confirm(du.uri_priority, "*", succeed=True)

    # send admin command
    # when both producer and consumer open
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")
    assert (
        f"Trying to remove the domain '{du.domain_priority}' while there are queues opened or opening"
        in res
    )

    # close producer and send the command again
    producer.close(du.uri_priority, succeed=True)
    consumer.close(du.uri_priority, succeed=True)
    res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")
    assert "Purged 0 message(s)" in res

    # try open producer and consumer again
    assert producer.open(du.uri_priority, flags=["write"], block=True) < 0
    assert consumer.open(du.uri_priority, flags=["read"], block=True) < 0


def test_remove_domain_with_unconfirmed_message(
    cluster: Cluster, domain_urls: tc.DomainUrls
):
    """
    issue DOMAINS REMOVE command with unconfirmed messages
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    du = domain_urls
    proxies = cluster.proxy_cycle()
    proxy = next(proxies)

    # producer open the queue,
    # produce messages and close the queue
    producer = proxy.create_client("producer")
    producer.open(du.uri_priority, flags=["write"], succeed=True)

    producer.post(
        du.uri_priority,
        [f"msg{i}" for i in range(3)],
        succeed=True,
        wait_ack=True,
    )
    producer.close(du.uri_priority, succeed=True)

    # send admin command
    # unconfirmed messages will be purged
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")
    assert "Purged 3 message(s)" in res


def test_remove_domain_not_on_disk(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
    """
    issue DOMAINS REMOVE command when the domain is not on disk
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    domain_name = "domain.foo"
    res = admin.send_admin(f"DOMAINS REMOVE {domain_name}")
    assert f"Domain '{domain_name}' doesn't exist" in res


def test_remove_domain_on_disk_not_in_cache(
    cluster: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
):
    """
    issue DOMAINS REMOVE command when the domain is not on disk
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {tc.DOMAIN_BROADCAST}")
    assert "Trying to remove a nonexistent domain" not in res
    assert "No queue purged." in res


def test_send_to_replicas(multi_node: Cluster, domain_urls: tc.DomainUrls):
    """
    send DOMAINS REMOVE admin command to replica instead of primary
    replica will boardcast to all the nodes including the primary
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    domain_priority = domain_urls.domain_priority
    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)

    queue1 = f"bmq://{domain_priority}/q1"
    queue2 = f"bmq://{domain_priority}/q2"

    # producer and consumer open the queue,
    # post and confirm messages and both close
    producer = proxy.create_client("producer")
    producer.open(queue1, flags=["write"], succeed=True)

    consumer = proxy.create_client("consumer")
    consumer.open(queue1, flags=["read"], succeed=True)

    producer.post(
        queue1,
        [f"msg{i}" for i in range(3)],
        succeed=True,
        wait_ack=True,
    )
    consumer.confirm(queue1, "*", succeed=True)
    producer.close(queue1, succeed=True)
    consumer.close(queue1, succeed=True)

    # producer open another queue, should be on a different partition
    producer.open(queue2, flags=["write"], succeed=True)
    producer.post(
        queue2,
        [f"msg{i}" for i in range(3)],
        succeed=True,
        wait_ack=True,
    )
    producer.close(queue2, succeed=True)

    leader = multi_node.last_known_leader
    member = multi_node.nodes(exclude=leader)[0]

    # send remove domain admin command
    # command couldn't go through since there's a queue open
    admin = AdminClient()
    admin.connect(member.config.host, int(member.config.port))

    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority}")
    assert "Purged" in res


def test_second_round(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    issue DOMAINS REMOVE command, and later finalize the command
    a queue and the removed domain can be opened after finalizing
    and when the domain exists on the disk
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    du = domain_urls
    proxies = cluster.proxy_cycle()
    proxy = next(proxies)

    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    uri = du.uri_priority

    def remove_from_disk_and_add_back(du):
        """
        remove the domain config file from the fisk,
        send the finalize DOMAINS REMOVE command
        check a producer can't open a queue under that domain
        add the domain config file back to the disk
        check now a producer can open a queue under that domain
        """
        # remove domain config file
        domain_config = cluster.config.domains[du.domain_priority]

        for node in cluster.configurator.brokers.values():
            del node.domains[du.domain_priority]
        cluster.deploy_domains()

        # second round
        res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority} FINALIZE")
        assert "SUCCESS" in res

        # producer can't open a queue since the domain config file doesn't exist
        producer = proxy.create_client("producer")
        assert producer.open(du.uri_priority, flags=["write"], block=True) < 0

        # add back the domain config file
        for node in cluster.configurator.brokers.values():
            node.domains[du.domain_priority] = domain_config
        cluster.deploy_domains()

        # now the queue can be opened
        assert producer.open(du.uri_priority, flags=["write"], succeed=True) == 0

        producer.close(uri=du.uri_priority, succeed=True)

    # put -> confirm -> admin command -> remove_from_disk_and_add_back
    write_messages(proxy, uri)
    res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")
    assert "Purged 0 message(s) for a total of 0  B from 1 queue(s)" in res
    remove_from_disk_and_add_back(du)

    # put -> no confirm -> admin command -> remove_from_disk_and_add_back
    producer = proxy.create_client("producer")
    producer.open(uri, flags=["write"], succeed=True)
    producer.post(uri, [f"msg{i}" for i in range(3)], succeed=True, wait_ack=True)
    producer.close(uri=uri, succeed=True)
    res = admin.send_admin(f"DOMAINS REMOVE {du.domain_priority}")
    assert "Purged 3 message(s) for a total of 12  B from 1 queue(s)" in res
    remove_from_disk_and_add_back(du)


def test_purge_then_remove(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    purge queue then remove
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    proxies = cluster.proxy_cycle()
    proxy = next(proxies)
    uri = domain_urls.uri_priority

    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))

    producer = proxy.create_client("producer")
    producer.open(uri, flags=["write"], succeed=True)
    producer.post(uri, [f"msg{i}" for i in range(5)], succeed=True, wait_ack=True)
    producer.close(uri=uri, succeed=True)

    res = admin.send_admin(f"DOMAINS DOMAIN {domain_urls.domain_priority} PURGE")
    assert f"Purged 5 message(s)" in res

    res = admin.send_admin(f"DOMAINS REMOVE {domain_urls.domain_priority}")
    assert "Purged 0 message(s) for a total of 0  B from 1 queue(s)" in res


def test_remove_without_connection(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    issue DOMAINS REMOVE command without any connection to a domain on disk
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    domain_priority = domain_urls.domain_priority
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))

    # first round
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority}")
    assert "No queue purged." in res

    # remove domain config file
    for node in cluster.configurator.brokers.values():
        del node.domains[domain_priority]
    cluster.deploy_domains()

    # second round
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority} FINALIZE")
    assert "SUCCESS" in res


def test_remove_then_restart(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    1. produce 5 messages
    2. completely remove the domain
    3. restart all nodes
    4. producer tries to open a queue on that domain
    5. add the domain back to disk
    6. produce 3 messages
    7. completely remove the domain
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    uri_priority = domain_urls.uri_priority
    domain_priority = domain_urls.domain_priority
    proxies = cluster.proxy_cycle()
    proxy = next(proxies)

    # produce 5 messages
    write_messages(proxy, uri_priority, 5, do_confirm=False)

    # first round
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority}")

    assert "Purged 5 message(s)" in res

    # remove domain config file
    domain_config = cluster.config.domains[domain_priority]
    for node in cluster.configurator.brokers.values():
        del node.domains[domain_priority]
    cluster.deploy_domains()

    # second round
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority} FINALIZE")
    assert "SUCCESS" in res

    # restart all nodes
    cluster.restart_nodes(wait_leader=True, wait_ready=True)

    # producer fails to open a queue
    producer = proxy.create_client("producer")
    producer.open(uri_priority, flags=["write"], block=True) != Client.e_SUCCESS

    # add back the domain config file
    for node in cluster.configurator.brokers.values():
        node.domains[domain_priority] = domain_config
    cluster.deploy_domains()

    # produce messages
    write_messages(proxy, uri_priority, 3, do_confirm=False)

    # first round
    admin.stop()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority}")
    assert "Purged 3 message(s)" in res

    # remove domain config file
    for node in cluster.configurator.brokers.values():
        del node.domains[domain_priority]
    cluster.deploy_domains()

    # second round
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority} FINALIZE")
    assert "SUCCESS" in res


def test_remove_with_reconfig(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    1. produce 5 messages
    2. completely remove the domain
    3. restart broker
    4. add domain back to disk
    5. call reconfigure to load the domain
    6. produce 3 messages
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    uri_priority = domain_urls.uri_priority
    domain_priority = domain_urls.domain_priority
    proxies = cluster.proxy_cycle()
    proxy = next(proxies)

    # produce 5 messages
    write_messages(proxy, uri_priority, 5, do_confirm=False)

    # first round
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority}")

    assert "Purged 5 message(s)" in res

    # remove domain config file
    domain_config = cluster.config.domains[domain_priority]
    for node in cluster.configurator.brokers.values():
        del node.domains[domain_priority]
    cluster.deploy_domains()

    # second round
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority} FINALIZE")
    assert "SUCCESS" in res

    # restart all nodes
    cluster.restart_nodes(wait_leader=True, wait_ready=True)

    # add back the domain config file
    for node in cluster.configurator.brokers.values():
        node.domains[domain_priority] = domain_config
    cluster.deploy_domains()

    # call reconfigure
    cluster.reconfigure_domain(domain_priority, succeed=True)

    # produce messages
    write_messages(proxy, uri_priority, 3, do_confirm=False)

    # first round
    admin.stop()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority}")
    assert "Purged 3 message(s)" in res


def test_remove_cache_remains(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    1. produce messages
    2. first round of DOMAINS REMOVE
    3. bring in the cache of the domain
    4. remove domain config file
    5. second round of DOMAINS REMOVE
    6. try to open a queue and nothing in the cache
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    proxies = cluster.proxy_cycle()
    proxy = next(proxies)
    uri = domain_urls.uri_priority
    domain_priority = domain_urls.domain_priority

    # produce 5 messages
    write_messages(proxy, uri, 5, do_confirm=False)

    # first round
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority}")

    assert "Purged 5 message(s)" in res

    # bring domain config in to the cache
    producer = proxy.create_client("producer")
    assert (
        producer.open(uri, flags=["write"], block=True, no_except=True)
        == Client.e_REFUSED
    )

    # remove domain config file
    for node in cluster.configurator.brokers.values():
        del node.domains[domain_priority]
    cluster.deploy_domains()

    # second round
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority} FINALIZE")
    assert "SUCCESS" in res

    # try to open a queue -> nothing in the cache
    assert producer.open(uri, flags=["write"], block=True) == Client.e_UNKNOWN


def test_remove_cache_cleaned(cluster: Cluster, domain_urls: tc.DomainUrls):
    """
    1. produce messages
    2. first round of DOMAINS REMOVE
    3. remove domain config file
    4. manually remove the cache
    5. second round of DOMAINS REMOVE
    6. try to open a queue and nothing in the cache
    """
    # TODO Skip this test until admin command routing is re-enabled
    return

    proxies = cluster.proxy_cycle()
    proxy = next(proxies)
    uri = domain_urls.uri_priority
    domain_priority = domain_urls.domain_priority

    # produce 5 messages
    write_messages(proxy, uri, 5, do_confirm=False)

    # first round
    admin = AdminClient()
    leader = cluster.last_known_leader
    admin.connect(leader.config.host, int(leader.config.port))
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority}")

    assert "Purged 5 message(s)" in res

    # remove domain config file
    for node in cluster.configurator.brokers.values():
        del node.domains[domain_priority]
    cluster.deploy_domains()

    # manually remove the cache
    res = admin.send_admin(f"DOMAINS RESOLVER CACHE_CLEAR {domain_priority}")
    res = admin.send_admin(f"CONFIGPROVIDER CACHE_CLEAR {domain_priority}")

    # second round
    res = admin.send_admin(f"DOMAINS REMOVE {domain_priority} FINALIZE")
    assert "SUCCESS" in res

    # try to open a queue -> nothing in the cache
    producer = proxy.create_client("producer")
    assert producer.open(uri, flags=["write"], block=True) == Client.e_UNKNOWN
