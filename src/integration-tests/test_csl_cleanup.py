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
Testing cleanup of CSL file.
"""
import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    cluster,
    order,
    tweak,
)
import glob

pytestmark = order(2)
timeout = 10


@tweak.cluster.partition_config.max_qlist_file_size(2000)
def test_csl_cleanup(cluster: Cluster, domain_urls: tc.DomainUrls):
    leader = cluster.last_known_leader
    proxy = next(cluster.proxy_cycle())
    domain_priority = domain_urls.domain_priority

    producer = proxy.create_client("producer")

    cluster._logger.info("Start to write to clients")

    csl_files_before_rollover = glob.glob(
        str(cluster.work_dir.joinpath(leader.name, "storage")) + "/*csl*"
    )
    assert len(csl_files_before_rollover) == 1

    # opening 10 queues would cause a rollover
    for i in range(0, 10):
        producer.open(
            f"bmq://{domain_priority}/q{i}", flags=["write,ack"], succeed=True
        )
        producer.close(f"bmq://{domain_priority}/q{i}", succeed=True)

    csl_files_after_rollover = glob.glob(
        str(cluster.work_dir.joinpath(leader.name, "storage")) + "/*csl*"
    )

    assert leader.outputs_regex(r"Log closed and cleaned up. Time taken", timeout)

    assert len(csl_files_after_rollover) == 1
    assert csl_files_before_rollover[0] != csl_files_after_rollover[0]
