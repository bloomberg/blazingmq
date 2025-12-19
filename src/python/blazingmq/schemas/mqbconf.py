# Copyright 2024 Bloomberg Finance L.P.
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

from dataclasses import dataclass, field
from decimal import Decimal
from enum import Enum
from typing import Optional

__NAMESPACE__ = "urn:x-bloomberg-com:mqbconfm"


class ExpressionVersion(Enum):
    """
    Enumeration of the various expression versions.
    """

    E_UNDEFINED = "E_UNDEFINED"
    E_VERSION_1 = "E_VERSION_1"


@dataclass
class FileBackedStorage:
    """
    Configuration for storage using a file on disk.
    """


@dataclass
class InMemoryStorage:
    """
    Configuration for storage using an in-memory map.
    """


@dataclass
class Limits:
    """Represent the various limitations to apply to either a 'domain' or an
    individual 'queue'.

    messages...............: maximum number of messages
    messagesWatermarkRatio.: threshold ratio to the maximum number of
    messages for which a high watermark alarm
    will trigger
    bytes..................: maximum cumulated number of bytes
    bytesWatermarkRatio....: threshold ratio to the maximum cumulated
    number of bytes for which a high watermark
    alarm will trigger
    """

    messages: Optional[int] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    messages_watermark_ratio: Decimal = field(
        default=Decimal("0.8"),
        metadata={
            "name": "messagesWatermarkRatio",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    bytes: Optional[int] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    bytes_watermark_ratio: Decimal = field(
        default=Decimal("0.8"),
        metadata={
            "name": "bytesWatermarkRatio",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )


@dataclass
class MsgGroupIdConfig:
    """Configuration for the use of Group Ids for routing.

    The garbage collection arguments could be assigned manually or get
    calculated out of statistics on the streams. They are considered
    internal and our intentions is _not_ to give customers full control
    over those numbers. Their role is to protect BlazingMQ from abuse
    i.e. cases of infinite Group Ids being stored. Another assumption is
    that 'maxGroups &gt;&gt; number of consumers'. rebalance..: groups
    will be dynamically rebalanced in way such that all consumers have
    equal share of Group Ids assigned to them maxGroups..: Maximum
    number of groups. If the number of groups gets larger than this, the
    least recently used one is evicted. This is a "garbage collection"
    parameter ttlSeconds.: minimum time of inactivity (no messages for a
    Group Id), in seconds, before a group becomes available for "garbage
    collection". 0 (the default) means unlimited
    """

    rebalance: bool = field(
        default=False,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    max_groups: int = field(
        default=2147483647,
        metadata={
            "name": "maxGroups",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    ttl_seconds: int = field(
        default=0,
        metadata={
            "name": "ttlSeconds",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )


@dataclass
class QueueConsistencyEventual:
    """
    Configuration for eventual consistency.
    """


@dataclass
class QueueConsistencyStrong:
    """
    Configuration for strong consistency.
    """


@dataclass
class QueueModeBroadcast:
    """
    Configuration for a broadcast queue.
    """


@dataclass
class QueueModeFanout:
    """Configuration for a fanout queue.

    appIDs.............: List of appIDs authorized to consume from the
    queue.
    publishAppIdMetrics: Whether to publish appId metrics.
    """

    app_ids: list[str] = field(
        default_factory=list,
        metadata={
            "name": "appIDs",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "min_occurs": 1,
        },
    )
    publish_app_id_metrics: bool = field(
        default=True,
        metadata={
            "name": "publishAppIdMetrics",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )


@dataclass
class QueueModePriority:
    """
    Configuration for a priority queue.
    """


@dataclass
class Consistency:
    """Consistency choices to configure a queue.

    eventual........: no Replication Receipt is required.
    strong..........: require Replication Receipt before ACK/PUSH
    """

    eventual: Optional[QueueConsistencyEventual] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
        },
    )
    strong: Optional[QueueConsistencyStrong] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
        },
    )


@dataclass
class Expression:
    """This complex type contains expression to evaluate when selecting
    Subscription for delivery.

    version................: expression version (default is no expression)
    text...................: textual representation of the expression
    """

    version: ExpressionVersion = field(
        default=ExpressionVersion.E_UNDEFINED,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    text: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )


@dataclass
class QueueMode:
    """Choice of all the various modes a queue can be configured in.

    fanout.........: multiple consumers are each getting all messages
    priority.......: consumers with highest priority are sharing load in
    round robin way
    broadcast......: send to all available consumers on a best-effort basis
    """

    fanout: Optional[QueueModeFanout] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
        },
    )
    priority: Optional[QueueModePriority] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
        },
    )
    broadcast: Optional[QueueModeBroadcast] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
        },
    )


@dataclass
class Storage:
    """
    Choice of all the various Storage backends inMemory....: store data in memory
    fileBacked..: store data in a file on disk.
    """

    in_memory: Optional[InMemoryStorage] = field(
        default=None,
        metadata={
            "name": "inMemory",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
        },
    )
    file_backed: Optional[FileBackedStorage] = field(
        default=None,
        metadata={
            "name": "fileBacked",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
        },
    )


@dataclass
class StorageDefinition:
    """Type representing the configuration for a Storage.

    config........: configuration for the type of storage to use
    domainLimits..: global limits to apply to the entire domain,
    cumulated for all queues in the domain
    queueLimits...: individual limits (as a subset of the global limits)
    to apply to each queue of the domain
    """

    domain_limits: Optional[Limits] = field(
        default=None,
        metadata={
            "name": "domainLimits",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    queue_limits: Optional[Limits] = field(
        default=None,
        metadata={
            "name": "queueLimits",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    config: Optional[Storage] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )


@dataclass
class Subscription:
    """This complex type contains various parameters required by an upstream node
    to configure subscription for an app.

    appId..................: app identifier
    expression.............: expression denoting a subscription for the app
    """

    app_id: Optional[str] = field(
        default=None,
        metadata={
            "name": "appId",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    expression: Optional[Expression] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )


@dataclass
class Domain:
    """Configuration for a Domain using the custom Bloomberg Domain.

    name................: name of this domain
    mode................: mode of the queues in this domain
    storage.............: storage to use by queues in this domain
    maxConsumers........: will reject if more than this number of consumers
    tries to connect. 0 (the default) means unlimited
    maxProducers........: will reject if more than this number of producers
    tries to connect. 0 (the default) means unlimited
    maxQueues...........: will reject creating more than this number of
    queues. 0 (the default) means unlimited
    msgGroupIdConfig....: optional configuration for Group Id routing
    features
    maxIdleTime.........: (seconds) time queue can be inactive before
    alarming. 0 (the default) means no monitoring and
    alarming
    messageTtl..........: (seconds) minimum time before which a message can
    be discarded (i.e., it's not guaranteed that the
    message will be discarded exactly after
    'ttlSeconds' time, but it is guaranteed that it
    will not be discarded before at least
    'ttlSeconds' time
    maxDeliveryAttempts.: maximum number of times BlazingMQ framework will
    attempt to deliver a message to consumers before
    purging it from the queue.  Zero (the default)
    means unlimited
    deduplicationTimeMs.: timeout, in milliseconds, to keep GUID of PUT
    message for the purpose of detecting duplicate
    PUTs.
    consistency.........: optional consistency mode.
    subscriptions.......: optional application subscriptions
    """

    name: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    mode: Optional[QueueMode] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    storage: Optional[StorageDefinition] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    max_consumers: int = field(
        default=0,
        metadata={
            "name": "maxConsumers",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    max_producers: int = field(
        default=0,
        metadata={
            "name": "maxProducers",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    max_queues: int = field(
        default=0,
        metadata={
            "name": "maxQueues",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    msg_group_id_config: Optional[MsgGroupIdConfig] = field(
        default=None,
        metadata={
            "name": "msgGroupIdConfig",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
        },
    )
    max_idle_time: int = field(
        default=0,
        metadata={
            "name": "maxIdleTime",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    message_ttl: Optional[int] = field(
        default=None,
        metadata={
            "name": "messageTtl",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    max_delivery_attempts: int = field(
        default=0,
        metadata={
            "name": "maxDeliveryAttempts",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    deduplication_time_ms: int = field(
        default=300000,
        metadata={
            "name": "deduplicationTimeMs",
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    consistency: Optional[Consistency] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    subscriptions: list[Subscription] = field(
        default_factory=list,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "min_occurs": 1,
        },
    )


@dataclass
class DomainDefinition:
    """Top level type representing the information retrieved when resolving a
    domain.

    location..: Domain location (i.e., cluster name)  REVIEW: consider: s/location/cluster/
    parameters: Domain parameters
    REVIEW: consider merging Domain into DomainDefinition
    """

    location: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )
    parameters: Optional[Domain] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
            "required": True,
        },
    )


@dataclass
class DomainVariant:
    """Either a Domain or a DomainRedirection.

    definition..: The full definition of a domain redirection.: The name
    of the domain to redirect to
    """

    definition: Optional[DomainDefinition] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
        },
    )
    redirect: Optional[str] = field(
        default=None,
        metadata={
            "type": "Element",
            "namespace": "urn:x-bloomberg-com:mqbconfm",
        },
    )
