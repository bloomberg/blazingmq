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
This test verifies that a proxy will be able to open a domain whose config was
deployed after the broker started, even if the domain is backed by a cluster
whose config was also deployed after the broker started.
"""

from blazingmq.dev.configurator.localsite import LocalSite
import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import start_cluster
from blazingmq.dev.reserveport import reserve_port


def test_deploy_new_domain_existing_cluster(multi_node):
    proxies = multi_node.proxy_cycle()
    proxy = next(proxies)

    new_domain_name = "my.new.domain"
    queue = f"bmq://{new_domain_name}/q"

    producer = proxy.create_client("producer")
    producer.open(queue, flags=["write,ack"], succeed=False)

    new_domain = multi_node.config.priority_domain(new_domain_name)
    proxy.config.proxy(new_domain)
    multi_node.deploy_domains()

    producer = proxy.create_client("producer")
    producer.open(queue, flags=["write,ack"], succeed=True)


@start_cluster(False, False)
def test_deploy_new_domain_new_cluster(multi_node):
    configurator = multi_node.configurator
    proxy_name = "westp2"

    with reserve_port() as tcp_address:
        # Add a proxy. It doesn't know about the domains or the cluster yet.
        proxy_config = configurator.broker(
            name=proxy_name,
            tcp_host="localhost",
            tcp_port=tcp_address.port,
            data_center="north",
        )
        configurator.deploy(proxy_config, LocalSite(multi_node.work_dir / proxy_name))

        try:
            multi_node.start()

            proxy = multi_node.proxies(datacenter="north")[0]
            producer = proxy.create_client("producer")

            # This fails because the proxy doesn't know about the domain.
            producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=False)

            # Add the domain to the proxy. This also adds the cluster.
            proxy_config.proxy(
                configurator.clusters[multi_node.name].domains[tc.DOMAIN_PRIORITY]
            )
            configurator.deploy(
                proxy_config, LocalSite(multi_node.work_dir / proxy_name)
            )

            # Now openQueue works.
            producer.open(tc.URI_PRIORITY, flags=["write,ack"], succeed=True)

        finally:
            multi_node.stop()
