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
Regression fuzz test for crash: PUT with messagePropertiesAreaWords=0.

Crash path (signal 6, abort):
  ProtocolUtil::calcUnpaddedLength        <- BSLS_ASSERT_SAFE(length), length=0
  MessageProperties::streamInHeader       <- calls calcUnpaddedLength(blob, 0)
  MessageProperties::streamIn
  SchemaLearner::read
  Routers::MessagePropertiesReader::get   <- reading property "x"
  SimpleEvaluator::Comparison<std::greater>::evaluate  <- expression "x > 0"

Root cause: streamInHeader computes msgPropsAreaSize = messagePropertiesAreaWords
* k_WORD_SIZE. When messagePropertiesAreaWords==0, msgPropsAreaSize==0. The
guard 'if (msgPropsAreaSize > blob.length())' does not catch zero. After the
per-field validity checks pass (mphSize > 0, numProperties > 0), calcUnpaddedLength
is called with length=0, which hits BSLS_ASSERT_SAFE(length).

The crash only fires during routing, not at PUT ingestion time: the broker
stores the message first and parses the properties only when evaluating a
subscription expression ('x > 0') to select a consumer.

The broker fixture asserts a zero exit code after the session; if the broker
aborts (signal 6), the test fails.
"""

import pytest

from blazingmq.dev import fuzztest


@pytest.mark.fuzztest
def test_put_malformed_message_properties_crash(broker):
    """
    Fuzz PUT messages with a malformed MessagePropertiesHeader (focusing on
    messagePropertiesAreaWords) against a broker consumer that has a property
    expression subscription, replicating the signal-6 abort.
    """
    fuzztest.fuzz_put_with_properties(broker.host, broker.port)
