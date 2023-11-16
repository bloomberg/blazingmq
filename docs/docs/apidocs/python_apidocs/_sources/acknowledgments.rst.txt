.. py:currentmodule:: blazingmq


Acknowledgments
###############

A producer can ensure that a message has been successfully posted by requesting an
acknowledgment for the message being posted. This can be requested by passing a
callable to the ``on_ack`` parameter on post. This callback will always get invoked
with an `Ack` message.

A useful pattern for the producer is using the ``on_ack`` callback to add the
incoming ack to a queue and then waiting to recieve it. This allows you to
synchronously `Session.post` to a BlazingMQ queue like: ::

    import blazingmq
    from queue import Queue

    queue_uri = "bmq://bmq.tutorial.workqueue/example_queue"
    with blazingmq.Session(blazingmq.session_events.log_session_event) as session:
        session.open_queue(queue_uri, write=True)

        queue = Queue()
        for i in (1,2,3):
            session.post(queue_uri, b"Some message here", on_ack=queue.put)
            ack = queue.get()
            if ack.status != blazingmq.AckStatus.SUCCESS:
                print("Error! Failed to post message")
            else:
                print("Success!")

        session.close_queue(queue_uri)

.. note::
    If you want to post without blocking on acknowledgments, you can leverage
    `functools.partial` to bind parameters to the ``on_ack`` callable. This can be
    useful if you need to do provide a recovery mechanism for messages that failed to
    post.
