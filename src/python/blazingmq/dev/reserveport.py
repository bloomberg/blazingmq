"""
blazingmq.dev.reserveport


PURPOSE: Provide a means to discover and reserve unused TCP ports.

FUNCTIONS:
    reserve_port: reserve a TCP port for 'bind'ing
    tcp_address: utility function that returns 'TcpAddress' objects

TYPES:
    TcpAddress: value semantic type for an IPv4 TCP address

ACKNOWLEDGEMENTS:
    This component was originally written by the BAS team.

This module defines a single function, 'reserve_port', that discovers and
reserves an unused TCP port on a specified network. The port is allocated
with the 'SO_REUSEADDR' options, which allows 'bind' to be called multiple
times. This technique is useful for pre-allocating ports that are utilized
by server components in test drivers, rather than rely on system-wide
reserved ports, or specially chosen port numbers.

This module also provides a value semantic type, 'TcpAddress', and a utility
function, 'tcp_address' for creating IPv4 'TcpAddress' objects. This type
is used as a vocabulary type throughout the framework APIs to provide
type-safe interfaces and a standard set of expectations for accessing and
manipulating the represented data.

Usage
-----

This section illustrates intended use of this module.

Example 1: Allocating ports used by server processes in a test driver
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

The following example illustrates the use of 'reserve_port' to allocate a
port that is shared between client and server processes in a test driver
scenario. When testing client/server interactions, it is necessary to use a
port that is known to both the client and the server. However, using a
well-known port means that the test driver cannot be run concurrently on
the same machine, while choosing a random port is risky because the test
case can fail if the port is in use. The 'reserve_port' function addresses
this issue.

```Python
from blazingmq.dev.reserveport import reserve_port


with reserve_port() as address:
    server = Server(listen_address=port)
    client = Client(connect_address=port)

    client.send("hello world")


Example 2: Create a localhost address
- - - - - - - - - - - - - - - - - - -

This example illustrates the use of 'tcp_address' to create a 'TcpAddress'
object representing port 65456 on the localhost network.

```Python
address = blazingmq.dev.reserveport.tcp_address("127.0.0.1", 65456)

assert(str(address) == "%s:%s" % (address.address, address.port))
```


References
----------
* ipaddress[1]

[1] https://docs.python.org/3/library/ipaddress.html
"""
import contextlib
import socket
import ipaddress
import typing


# pylint: disable=too-few-public-methods,inherit-non-class
class TcpAddress(typing.NamedTuple):
    """This class implements a value semantic type for a TCP address."""

    address: ipaddress.IPv4Address
    port: int

    def __str__(self) -> str:
        """Return a string representation of this object."""
        return "{}:{}".format(self.address, self.port)


def tcp_address(address: str, port: int) -> TcpAddress:
    """Create a 'TcpAddress' object having the specified 'address' and 'port'.

    The specified 'address' must represent a valid IPv4 address in dotted-quad
    format as documented by the standard Python 'ipaddress' module.
    """
    return TcpAddress(ipaddress.IPv4Address(address), port)


@contextlib.contextmanager
def reserve_port() -> typing.Iterator[TcpAddress]:
    """Return a TCP address with a reserved port for 'bind'ing."""

    with socket.socket() as sock:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(("0.0.0.0", 0))
        sockname = sock.getsockname()

        address = tcp_address(sockname[0], sockname[1])

    yield address
