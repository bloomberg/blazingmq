# Copyright 2026 Bloomberg Finance L.P.
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
Fuzz tests for the BlazingMQ broker protocol.

Each test fuzzes a single request type by mutating messages and sending them
to the broker to verify it handles malformed input without crashing.
"""

import pytest

from blazingmq.dev import fuzztest

REQUESTS = [
    "authentication",
    "negotiation",
    "open_queue",
    "configure_stream",
    "configure_queue_stream",
    "put",
    "confirm",
    "close_queue",
    "disconnect",
    "negotiation_bypass_authentication",
]


@pytest.mark.fuzztest
@pytest.mark.parametrize("request_name", REQUESTS)
def test_fuzz_request(broker, request_name):
    fuzztest.fuzz(broker.host, broker.port, request=request_name)
