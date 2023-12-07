.. py:module:: blazingmq
.. _api-reference:

API Reference
#############


Session
=========================

.. autoclass:: Session
    :members:

.. autoclass:: Timeouts
    :members:
    :member-order: bysource

.. autoclass:: SessionOptions
    :members:
    :member-order: bysource

.. autoclass:: QueueOptions
    :members:
    :member-order: bysource


Message Classes
===============

.. autoclass:: Message()
    :members:

.. autoclass:: MessageHandle()
    :members:

.. autoclass:: Ack()
    :members:


Health Monitoring
=================

.. autoclass:: BasicHealthMonitor
   :members:


Enumerations
============

.. autoclass:: blazingmq.AckStatus()

    .. autoattribute:: AckStatus.SUCCESS
        :annotation:

    .. autoattribute:: AckStatus.CANCELED
        :annotation:

    .. autoattribute:: AckStatus.INVALID_ARGUMENT
        :annotation:

    .. autoattribute:: AckStatus.LIMIT_BYTES
        :annotation:

    .. autoattribute:: AckStatus.LIMIT_MESSAGES
        :annotation:

    .. autoattribute:: AckStatus.NOT_CONNECTED
        :annotation:

    .. autoattribute:: AckStatus.NOT_READY
        :annotation:

    .. autoattribute:: AckStatus.NOT_SUPPORTED
        :annotation:

    .. autoattribute:: AckStatus.REFUSED
        :annotation:

    .. autoattribute:: AckStatus.TIMEOUT
        :annotation:

    .. autoattribute:: AckStatus.UNKNOWN
        :annotation:

    .. autoattribute:: AckStatus.UNRECOGNIZED
        :annotation:

.. autoclass:: CompressionAlgorithmType()

   .. autoattribute:: CompressionAlgorithmType.NONE
        :annotation:

   .. autoattribute:: CompressionAlgorithmType.ZLIB
        :annotation:

.. autoclass:: PropertyType()

    .. autoattribute:: PropertyType.BOOL
        :annotation:

    .. autoattribute:: PropertyType.CHAR
        :annotation:

    .. autoattribute:: PropertyType.SHORT
        :annotation:

    .. autoattribute:: PropertyType.INT32
        :annotation:

    .. autoattribute:: PropertyType.INT64
        :annotation:

    .. autoattribute:: PropertyType.STRING
        :annotation:

    .. autoattribute:: PropertyType.BINARY
        :annotation:

Exceptions
==========

.. autoexception:: blazingmq.Error

.. autoexception:: blazingmq.exceptions.BrokerTimeoutError

Session Events
==============

.. autoclass:: blazingmq.session_events.SessionEvent()

.. autoclass:: blazingmq.session_events.QueueEvent()

.. autoclass:: blazingmq.session_events.Connected()

.. autoclass:: blazingmq.session_events.Disconnected()

.. autoclass:: blazingmq.session_events.ConnectionLost()

.. autoclass:: blazingmq.session_events.Reconnected()

.. autoclass:: blazingmq.session_events.StateRestored()

.. autoclass:: blazingmq.session_events.ConnectionTimeout()

.. autoclass:: blazingmq.session_events.HostUnhealthy()

.. autoclass:: blazingmq.session_events.HostHealthRestored()

.. autoclass:: blazingmq.session_events.QueueSuspended()
   :show-inheritance:

.. autoclass:: blazingmq.session_events.QueueSuspendFailed()
   :show-inheritance:

.. autoclass:: blazingmq.session_events.QueueResumed()
   :show-inheritance:

.. autoclass:: blazingmq.session_events.QueueResumeFailed()
   :show-inheritance:

.. autoclass:: blazingmq.session_events.QueueReopened()
   :show-inheritance:

.. autoclass:: blazingmq.session_events.QueueReopenFailed()
   :show-inheritance:

.. autoclass:: blazingmq.session_events.SlowConsumerNormal()

.. autoclass:: blazingmq.session_events.SlowConsumerHighWaterMark()

.. autoclass:: blazingmq.session_events.Error()

.. autoclass:: blazingmq.session_events.InterfaceError()


Helper Functions
================

.. autofunction:: blazingmq.session_events.log_session_event

Testing Utilities
=================

.. automodule:: blazingmq.testing
   :members:

Helper Types
============

.. autoclass:: blazingmq.PropertyTypeDict

.. autoclass:: blazingmq.PropertyValueDict
