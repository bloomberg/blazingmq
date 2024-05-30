"""
BlazingMQ Broker schemas.
"""

from enum import IntEnum, IntFlag
from typing import Any, Dict

# =============================================================================
#                                  CONSTANTS
# =============================================================================


SchemaDescription = Dict[str, Any]


class EventType(IntEnum):
    """
    See also: bmqp::EventType
    """

    CONTROL = 0x01
    PUT = 0x02
    CONFIRM = 0x03


class TypeSpecific(IntEnum):
    """
    See also: bmqp::EventHeader, bmpq::EncodingType
    """

    __RESERVED_BITS = 5

    ENCODING_BER = 0x00 << __RESERVED_BITS
    ENCODING_JSON = 0x01 << __RESERVED_BITS
    EMPTY = 0x00  # equal to ENCODING_BER


class QueueFlags(IntFlag):
    """
    See also: bmqt::QueueFlags
    """

    READ = 0x02
    WRITE = 0x04


class PutHeaderFlags(IntFlag):
    """
    See also: bmqp::PutHeaderFlags
    """

    ACK_REQUESTED = 0x01 << 0
    MESSAGE_PROPERTIES = 0x01 << 1


# =============================================================================
#                    BLAZINGMQ BROKER CONTROL MESSAGE SCHEMAS
# =============================================================================


CLIENT_IDENTITY_SCHEMA: SchemaDescription = {
    "clientIdentity": {
        "protocolVersion": 999999,
        "sdkVersion": 999999,
        "clientType": "E_TCPCLIENT",
        "processName": "fuzztest",
        "pid": 0,
        "sessionId": 1,
        "hostName": "localhost",
        "features": "PROTOCOL_ENCODING:JSON",
        "clusterName": "",
        "clusterNodeId": -1,
        "sdkLanguage": "E_CPP",
        "guidInfo": {"clientId": "fuzztest", "nanoSecondsFromEpoch": 0},
    }
}

CONFIGURE_QUEUE_STREAM_SCHEMA: SchemaDescription = {
    "rId": 0,
    "configureQueueStream": {
        "qId": 0,
        "streamParameters": {
            "subIdInfo": {"subId": 0, "appId": "__default"},
            "maxUnconfirmedMessages": 0,
            "maxUnconfirmedBytes": 0,
            "consumerPriority": -2147483648,
            "consumerPriorityCount": 0,
        },
    },
}

OPEN_QUEUE_SCHEMA: SchemaDescription = {
    "rId": 0,
    "openQueue": {
        "handleParameters": {
            "uri": "bmq://bmq.test.mem.priority/fuzz",
            "qId": 0,
            "subIdInfo": {"subId": 0, "appId": "__default"},
            "flags": QueueFlags.READ | QueueFlags.WRITE,
            "readCount": 1,
            "writeCount": 1,
            "adminCount": 0,
        }
    },
}

CLOSE_QUEUE_SCHEMA: SchemaDescription = {
    "rId": 0,
    "closeQueue": {
        "handleParameters": {
            "uri": "bmq://bmq.test.mem.priority/fuzz",
            "qId": 0,
            "subIdInfo": {"subId": 0, "appId": "__default"},
            "flags": QueueFlags.READ | QueueFlags.WRITE,
            "readCount": 1,
            "writeCount": 1,
            "adminCount": 0,
        },
        "isFinal": True,
    },
}

DISCONNECT_SCHEMA: SchemaDescription = {"rId": 0, "disconnect": {}}

ADMIN_COMMAND_SCHEMA: SchemaDescription = {
    "rId": 0,
    "adminCommand": {"command": "help"},
}
