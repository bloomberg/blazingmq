"""
blazingmq.dev.it.util


PURPOSE: Provide miscellaneous utilities .
"""

import contextlib
import logging
import random
import string
import time


def random_string(len):
    letters = string.ascii_lowercase
    return "".join(random.choice(letters) for i in range(len))


def wait_until(command, timeout, interval=1, quiet=False):
    """
    Repeatedly execute the specified 'command' every specified 'interval'
    seconds until the first successful execution or after the specified
    'timeout' seconds have passed.  Return true if the command has succeeded,
    or false otherwise.
    """
    give_up_time = time.time() + timeout
    while time.time() < give_up_time:
        if command():
            return True
        time.sleep(interval)
    if not quiet:
        logging.getLogger("test").warning(
            f"TIMEOUT: command did not succeed within {timeout}s"
        )
    return False


def _invalid_attempt_error(reason):
    raise RuntimeError("invalid explicit call to @attempt function")


def attempt(count, interval=1):
    """
    Function decorator.  Execute the decorated function.  If the function
    raises an exception, retry it after the specified 'interval'.  If the
    function fails more than 'count' times, give up and re-raise the exception.
    """

    def wrapper(function):
        if not callable(function):
            raise RuntimeError("@attempt is only for functions")

        for i in range(count):
            try:
                function()
                return _invalid_attempt_error
            except:
                if i == count - 1:
                    raise
                time.sleep(interval)

    return wrapper


@contextlib.contextmanager
def internal_use(obj):
    """
    Used to indicate that we are in an internal context. Methods like
    'capture()' will log at debug level if in internal context, in contrast to
    info when outside any context. When 'capture()' is used directly by tests
    we want to log because it shows that the test makes progress.  Printing on
    every internal call would create too much noise, that wouldn't be useful
    for showing progress of tests.  If 'src' is specified, copy its log level,
    otherwise set level to 'DEBUG'.  This facilitates passing down level from
    Cluster to Process.
    """

    logger = logging.getLogger(__name__)
    logger.debug(
        "internal_use: switching logger from %s to %s",
        obj._logger,
        obj._internal_logger,
    )
    previous = obj._logger
    obj._logger = obj._internal_logger
    try:
        yield
    finally:
        logger.debug("internal_use: restoring logger to %s", previous)
        obj._logger = previous


class Queue:
    """
    This class provides a convenient way of interacting with a queue via a
    client without having to specify the client and the URI over and over
    again.  It can also be used as a context manager, in which case the exit
    action is to close the queue (if it has not yet been explicitly closed).
    """

    def __init__(self, client, uri, flags, **kw):
        """Open the queue specified by 'uri' via the specified 'client, using the
        specified 'flags'.
        """
        self.client = client
        self.uri = uri
        self.flags = flags
        client.open(uri, flags, succeed=True, **kw)

    def __enter__(self):
        return self

    def __exit__(self, *args):
        if self.client:
            self.close()

    def __repr__(self):
        return f"Queue<{self.client.name}: {', '.join([ self.uri] + self.flags)}>"

    def post(self, *args, **kwargs):
        return self.client.post(self.uri, *args, **kwargs)

    def list(self, *args, **kwargs):
        return self.client.list(self.uri, *args, **kwargs)

    def confirm(self, *args, **kwargs):
        return self.client.confirm(self.uri, *args, **kwargs)

    def configure(self, *args, **kwargs):
        return self.client.configure(self.uri, *args, **kwargs)

    def close(self, *args, **kwargs):
        rc = self.client.close(self.uri, *args, **kwargs)
        self.client = None
        return rc


class ListContextManager(list):
    def __enter__(self):
        return self

    def __exit__(self, *args):
        for item in self.__reversed__():
            item.__exit__(*args)
