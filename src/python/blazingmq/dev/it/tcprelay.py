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
Forward TCP connections from a local port to a remote endpoint.

Prints "Listening on <host>:<port>" once ready.  Killing the process drops
all forwarded connections.
"""

import argparse
import asyncio

_LISTEN_HOST = "127.0.0.1"


async def _pipe(reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
    try:
        while True:
            data = await reader.read(65536)
            if not data:
                break
            writer.write(data)
            await writer.drain()
    except OSError:
        pass
    finally:
        writer.close()


async def _serve(listen_port: int, remote_host: str, remote_port: int) -> None:
    async def handle(
        client_reader: asyncio.StreamReader, client_writer: asyncio.StreamWriter
    ) -> None:
        try:
            remote_reader, remote_writer = await asyncio.open_connection(
                remote_host, remote_port
            )
        except OSError:
            client_writer.close()
            return
        await asyncio.gather(
            _pipe(client_reader, remote_writer), _pipe(remote_reader, client_writer)
        )

    server = await asyncio.start_server(handle, _LISTEN_HOST, listen_port)
    port = server.sockets[0].getsockname()[1]
    print(f"Listening on {_LISTEN_HOST}:{port}", flush=True)
    async with server:
        await server.serve_forever()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-r", "--remote", required=True, help="host:port")
    parser.add_argument("-p", "--port", type=int, default=0, help="listen port")
    args = parser.parse_args()

    remote_host, remote_port = args.remote.rsplit(":", 1)
    asyncio.run(_serve(args.port, remote_host, int(remote_port)))


if __name__ == "__main__":
    main()
