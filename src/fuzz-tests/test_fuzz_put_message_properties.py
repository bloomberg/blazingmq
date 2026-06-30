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
Fuzz test for MessagePropertyHeader fields in PUT messages.

Systematically mutates prop_type, value_length, and name_length across
old-style and new-style encodings with varying property counts.  Uses a
property expression subscription ('x > 0') so the broker parses properties
during routing.
"""

import pytest

from blazingmq.dev.fuzztest.put_message_properties import (
    fuzz_properties,
    fuzz_property_data,
)


@pytest.mark.fuzztest
def test_fuzz_put_message_properties(broker):
    fuzz_properties(broker.host, broker.port)


@pytest.mark.fuzztest
def test_fuzz_put_message_property_data(broker):
    fuzz_property_data(broker.host, broker.port)
