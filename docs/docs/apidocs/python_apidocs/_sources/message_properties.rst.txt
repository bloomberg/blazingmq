.. py:currentmodule:: blazingmq


Message Properties
##################

A message can have some arbitrary metadata associated with it. This metadata is
represented by a dictionary of key-value pairs. Here, we cover how to associate
properties with messages being posted, and how to retrieve properties when consuming
messages.

Posting
=======

Message With Properties
-----------------------

When invoking `Session.post` with a message, you can provide a `dict` to the optional
``properties`` parameter to associate properties with the message being posted.

BlazingMQ has particular types of property values that can be part of this
dictionary -- as enumerated in `PropertyType`.

If you do not particularly care about which exact `PropertyType` is being targeted,
the types inferred by default are as follows:

+-------------+-----------------------+
| Python Type | BlazingMQ Type        |
+=============+=======================+
| int         | `PropertyType.INT64`  |
+-------------+-----------------------+
| bool        | `PropertyType.BOOL`   |
+-------------+-----------------------+
| str         | `PropertyType.STRING` |
+-------------+-----------------------+
| bytes       | `PropertyType.BINARY` |
+-------------+-----------------------+

Properties With Overrides
-------------------------

However, if you want to target a particular `PropertyType`, you can leverage the
``property_type_overrides`` in `Session.post`. Be aware that any key included in
``property_type_overrides`` *must* be present in ``properties``. For any particular
property, if an override is missing, the default is inferred as above.

The following table describes the properties that can be set and the accepted types:

+-----------------------+-------------+
| BlazingMQ Type        | Python Type |
+=======================+=============+
| `PropertyType.INT64`  | int         |
+-----------------------+-------------+
| `PropertyType.INT32`  | int         |
+-----------------------+-------------+
| `PropertyType.SHORT`  | int         |
+-----------------------+-------------+
| `PropertyType.CHAR`   | bytes       |
+-----------------------+-------------+
| `PropertyType.BOOL`   | bool        |
+-----------------------+-------------+
| `PropertyType.STRING` | str         |
+-----------------------+-------------+
| `PropertyType.BINARY` | bytes       |
+-----------------------+-------------+

.. note::
    - All integers being passed as values need to be in the appropriate range to not
      trigger an exception.
    - The `PropertyType.CHAR` type accepts a `bytes` object with a single byte, i.e.,
      bytes with a length of 1.


Consuming
=========

If the `Message` received in your ``on_message`` callback installed on `Session`
contains any properties, it will always contain both a `Message.properties` and a
fully populated `Message.property_types`. This means that, for every property denoted
by a key in `Message.properties`, there will also be a corresponding key in
`Message.property_types` to denote the BlazingMQ type of the property received.

The Python types that you can expect will be a mirror of the second table in the
section above.
