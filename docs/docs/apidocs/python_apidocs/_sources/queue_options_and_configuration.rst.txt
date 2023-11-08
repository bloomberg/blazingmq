.. py:currentmodule:: blazingmq


Queue Options and Configuration
###############################

There are several available options for configuring a queue's behavior. The
documentation for `QueueOptions` shows all of the options available for you to
configure.

Setting Queue Options
=====================

Most applications will use default values for their queue settings, but if you
need to adjust an option, you can provide `QueueOptions` when you open the
queue. For example, if you want to configure the priority of your consumer
relative to other consumers of the same queue, you could set the
*consumer_priority* option when you open the queue::

  session.open(queue_uri,
               options=blazingmq.QueueOptions(consumer_priority=100)
  )

You can also adjust the options of an already opened queue to meet the
application's needs. If the application enters a phase where it's receiving
larger messages, you might want to tell the broker that it's okay for you to
have twice as many bytes as normal for messages that have been received but
have not yet been confirmed. You could do that with::

  more_bytes_options = blazingmq.QueueOptions(
      max_unconfirmed_bytes=blazingmq.QueueOptions.DEFAULT_MAX_UNCONFIRMED_BYTES
          * 2
  )
  session.configure_queue(queue_uri, options=more_bytes_options)

Common Scenarios
================

There are several common patterns for using non-default queue options.

Pausing consumption on demand
-----------------------------

An application can ask the broker to stop delivering new messages. This is most
common as part of a graceful shutdown (as seen in the User Guide's
:ref:`simple-consumer-label`), but you could choose to do this at any time. You
accomplish this by setting *max_unconfirmed_messages* to zero::

    session.configure_queue(queue_uri, QueueOptions(max_unconfirmed_messages=0))

If your application wants to resume receiving messages later, it should save
the queue's original options with a call to `.get_queue_options` before setting
``max_unconfirmed_messages=0``, and then supply those original options to
`.configure_queue` when it's ready for new messages again.

Delivering all messages to one consumer
---------------------------------------

When multiple consumers are attached to a queue with the default
*consumer_priority*, the broker delivers messages in a round-robin
fashion. However, it may be preferable to have a single consumer receiving the
messages, with one or more backup consumers that can take over if the primary
consumer goes offline. This can be achieved by supplying a unique
*consumer_priority* for each consumer, in which case every message for the
queue will be delivered to the highest priority consumer that is connected and
not suspended.
