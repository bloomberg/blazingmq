.. py:currentmodule:: blazingmq

User Guide
##########

The guide will walk you through building a simple producer and consumer using the
`blazingmq` package. This guide however does not cover some of the more complex
concepts including message properties, queue options, queue configuration and the
`Message` object. For reference documentation see the :ref:`api-reference`.


Simple Producer
===============

This is the basic example for a BlazingMQ producer posting a single message: ::

    import blazingmq

    queue_uri = "bmq://bmq.tutorial.workqueue/example_queue"
    with blazingmq.Session(blazingmq.session_events.log_session_event) as session:
        session.open_queue(queue_uri, write=True)
        session.post(queue_uri, b"Some message here")
        session.close_queue(queue_uri)


First, you need create an instance of the `Session` object.
The only required positional argument is a callback that takes a single argument of
type `.SessionEvent`. For example, the ``on_session_event`` callback *could* look like: ::

    def on_session_event(event):
        print(event)

In the following example however, we use the library function
`session_events.log_session_event` which provides some default configured logging for
incoming session events: ::

    import blazingmq

    with blazingmq.Session(blazingmq.session_events.log_session_event) as session:
        # session can be used inside this block

.. note::
    There should be **only one** `Session` object per process since it is very
    heavyweight, holds a lot of state, and consumes both broker and operating system
    resources.

The session is the object responsible for network connections, thread pools, internal
memory and storage. The context manager will make sure that its resources are
correctly managed even if an exception is raised in the block. On enter, the context
manager will ensure that the `Session` is started and valid and on exit, it will
ensure that the `Session` is stopped and cleaned up.

Using the `Session`, you can open a queue in write mode. Using ``write=True`` enables
our queue for posting messages: ::

        session.open_queue(queue_uri, write=True)



Once opened, you can use the queue URI to post a message on the session like: ::

    session.post(queue_uri, b"Some message here")

The `Session.post` optionally also takes an ``on_ack`` callback if the user wants to
receive an acknowledgment for the message being posted. This ``on_ack`` callback will
be invoked with the result of the post. ::

    from blazingmq

    def on_ack_callback(ack):
        if ack.status != blazingmq.AckStatus.SUCCESS:
            print("Post failed")
        print("Post success")

    session.post(queue_uri, b"Some message here", on_ack=on_ack_callback)

.. note::
    This is the only way for a producer to ensure that a message was received by the
    BlazingMQ framework via the BlazingMQ broker.

.. warning::
    Invoking any queue related method on the `Session` object that invoked
    the ``on_ack`` or ``on_session_event`` callback will lead to a deadlock. That
    means invoking `Session.open_queue`, `Session.configure_queue`, or
    `Session.close_queue` in the callback will deadlock. Additionally, invoking
    `Session.stop` will also deadlock. This implies that the only acceptable `Session`
    method to be called from the callback is is `Session.confirm`, and consequently
    `MessageHandle.confirm`.


You can use this callback to implement a few useful patterns that are documented in
the `Acknowledgments`_ section.

.. _`Acknowledgments` : acknowledgments.html

Additionally, you can associate message properties with messages being posted. This
is documented in `Message Properties`_

.. _`Message Properties` : message_properties.html

Finally, you need to close the queue when you have finished using it. ::

        session.close_queue(queue_uri)

.. note::
    Any acknowledgments that are still outstanding will be negatively acknowledged
    before `Session.close_queue` returns.

.. _simple-consumer-label:

Simple Consumer
===============

This is a basic example for a BlazingMQ consumer, printing and confirming all
incoming messages: ::

    import blazingmq

    import signal
    import threading

    def on_message_callback(message, message_handle):
       print(message.data)
       message_handle.confirm()

    event = threading.Event()

    def handler(signum, frame):
        print("Goodbye!")
        event.set()

    signal.signal(signal.SIGINT, handler)

    queue_uri = "bmq://bmq.tutorial.workqueue/example_queue"
    with blazingmq.Session(blazingmq.session_events.log_session_event,
                           on_message=on_message_callback) as session:
        session.open_queue(queue_uri, read=True)
        event.wait()
        session.configure_queue(queue_uri, blazingmq.QueueOptions(0, 0, 0))
        session.close_queue(queue_uri)

The first thing you need to do for any BlazingMQ application is to create the
`Session`. Since we intend to consume messages from a queue opened in read
mode, we also want to specify the optional ``on_message`` callback in addition
to the required ``on_session_event`` callback: ::

    with blazingmq.Session(blazingmq.session_events.log_session_event,
                           on_message=on_message_callback) as session:
        # session can be used inside this block

.. note::
    There should be **only one** `Session` object per process since it is very
    heavyweight, holds a lot of state, and consumes both broker and operating system
    resources.

.. note::
    The ``on_message`` callback will receive messages for **all** queues in read mode. If 
    the program is reading from multiple queues, `Message.queue_uri` will indicate which
    queue this message is associated with.

.. warning::
    Invoking any queue related method on the `Session` object that invoked
    the ``on_message`` or ``on_session_event`` callback will lead to a deadlock. That
    means invoking `Session.open_queue`, `Session.configure_queue`, or
    `Session.close_queue` in the callback will deadlock. Additionally, invoking
    `Session.stop` will also deadlock. This implies that the only acceptable `Session`
    method to be called from the callback is `Session.confirm`, and consequently
    `MessageHandle.confirm`.


You can then use the `Session` to open a queue. When you are opening a queue in read
mode, you *must* specify an ``on_message`` callback to process incoming messages as
documented above: ::

    session.open_queue(queue_uri, read=True)

.. note::
    You can create a queue that is both a producer and a consumer,
    by passing in both ``read=True`` and ``write=True``.

To open the queue you need to provide the URI that uniquely identifies the
queue within the BlazingMQ framework
(``bmq://bmq.tutorial.workqueue/example_queue``). To open it in read mode,
``read=True`` is used.

.. note::
    The `QueueOptions` parameter has been elided, and
    the default is being used.

When `Session.open_queue` method returns, messages directed towards the specified
queue will start being received in the ``on_message_callback``.

Once you get a `Message` object in the callback, you can inspect the data inside
the message: ::

        print(message.data)

The data contained inside will be of type `bytes`. To correctly decode the data
inside the `Message` object you need to know the encoding that the producer
used when it placed the message in the queue. This could be JSON, XML, BER or
any other type of encoding. From the perspective of BlazingMQ, the encoding
does not matter since only bytes are transmitted.

Assuming at this point the processing of the message was successful and you do
not want to receive it again, you can call `Session.confirm` with this message
passed as an argument. This will notify the BlazingMQ broker that the message
should not be re-delivered to another consumer. ::

    session.confirm(message)

Alternatively, an instance of `.blazingmq.MessageHandle` is received along with every message.
It can be used to confirm the message with which it was received. Notice that you don't
need to pass the message as an argument. ::

    message_handle.confirm()

At the end, when the queue has served its purpose, you want to first pause incoming
messages and ensure in-flight callbacks to finish processing by calling
`Session.configure_queue` with zero-ed queue options: ::

    session.configure_queue(queue_uri, blazingmq.QueueOptions(0, 0, 0))

For more about `QueueOptions` and `Session.configure_queue`, please refer to
the `Queue Options`_ section.

.. _`Queue Options` : queue_options_and_configuration.html

Finally, once this returns successfully, you can safely close it by calling
`Session.close_queue` with the appropriate queue URI: ::

    session.close_queue(queue_uri)


Once this method returns, you will no longer receive messages for the queue and the
queue URI can no longer be used for any operations, other than `Session.open_queue`.


Host Health Monitoring
======================

You can pass ``host_health_monitor=None`` to the `Session` constructor if you
don't want any host health monitoring, in which case you won't be able to use
the *suspends_on_bad_host_health* queue option, and you will never get any host
health related session events.

For testing purposes, you can pass an instance of `BasicHealthMonitor` as the
*host_health_monitor* argument for the `Session` constructor, and your tests
can control whether the `Session` believes the host is healthy or not by
calling the `.set_healthy` and `.set_unhealthy` methods of that instance.

.. versionadded:: 0.7.0
   Host health monitoring and queue suspension
