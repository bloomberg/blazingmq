# Copyright 2025 Bloomberg Finance L.P.
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
Authentication test suite using ONLY built-in authenticators.

This test suite validates authentication logic without any external plugins.
All tests use the built-in authenticators:
  - BasicAuthenticator: BASIC mechanism, validates credentials from config
                        Config format: {"key": "username", "value": {"stringVal": "password"}}
  - AnonAuthenticator: ANONYMOUS mechanism, passes if "shouldPass" setting is true (default),
                       fails otherwise
  - TestAuthenticator: TEST mechanism, sleeps for a configured duration (default 0)
                       Config format: {"key": "sleepTimeMs", "value": {"intVal": "duration"}}

This approach tests all authentication scenarios without needing external plugins.
"""

import threading
import pytest

import blazingmq.dev.it.testconstants as tc
from blazingmq.dev.it.process.rawclient import RawClient
from blazingmq.dev.it.process.admin import AdminClient
from blazingmq.dev.it.process.proc import ProcessExitError

from blazingmq.dev.it.fixtures import (  # pylint: disable=unused-import
    Cluster,
    order,
    single_node,
    tweak,
    start_cluster,
)

pytestmark = order(99)


# ==============================================================================
# Basic Authentication Tests
# ==============================================================================


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            }
        ]
    }
)
def test_authenticate_basic_success(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test successful authentication with built-in BasicAuthenticator."""
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    auth_resp = client.authenticate("Basic", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0

    nego_resp = client.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            }
        ]
    }
)
def test_authenticate_basic_failure(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test failed authentication with invalid credentials."""
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Invalid credentials should fail
    auth_resp = client.authenticate("Basic", "invalid:wrong")
    assert auth_resp["authenticationResponse"]["status"]["code"] != 0

    client.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user0", "value": {"stringVal": "password0"}},
                    {"key": "user1", "value": {"stringVal": "password1"}},
                    {"key": "user2", "value": {"stringVal": "password2"}},
                    {"key": "user3", "value": {"stringVal": "password3"}},
                    {"key": "user4", "value": {"stringVal": "password4"}},
                    {"key": "user5", "value": {"stringVal": "password5"}},
                    {"key": "user6", "value": {"stringVal": "password6"}},
                    {"key": "user7", "value": {"stringVal": "password7"}},
                ],
            }
        ]
    }
)
def test_authenticate_concurrent(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test concurrent authentication with BasicAuthenticator."""
    num_threads = 8
    results = [None] * num_threads
    threads = []

    def auth_worker(idx):
        client = RawClient()
        client.open_channel(*single_node.admin_endpoint)
        auth_resp = client.authenticate(
            "Basic", f"user{idx}:password{idx}"
        )
        results[idx] = auth_resp["authenticationResponse"]["status"]["code"]
        client.negotiate()
        client.stop()

    for i in range(num_threads):
        t = threading.Thread(target=auth_worker, args=(i,))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()

    # Assert all authentications succeeded
    assert all(code == 0 for code in results), f"Some authentications failed: {results}"


# ==============================================================================
# Reauthentication Tests
# ==============================================================================


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [{"key": "user1", "value": {"stringVal": "password1"}}],
            }
        ]
    }
)
def test_reauthenticate_success(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test successful reauthentication with same credentials."""
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Initial authentication
    auth_resp = client.authenticate("Basic", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0
    assert auth_resp["authenticationResponse"]["lifetimeMs"] == 600000

    nego_resp = client.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    # Reauthentication with same credentials
    auth_resp = client.authenticate("Basic", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0

    client.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            }
        ]
    }
)
def test_reauthenticate_failure(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test reauthentication failure with invalid credentials."""
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Initial authentication succeeds
    auth_resp = client.authenticate("Basic", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0
    assert auth_resp["authenticationResponse"]["lifetimeMs"] == 600000

    nego_resp = client.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    # Reauthentication with wrong credentials fails
    auth_resp = client.authenticate("Basic", "user1:wrongpass")
    assert auth_resp["authenticationResponse"]["status"]["code"] != 0

    # Connection should be closed after failed reauthentication
    with pytest.raises(ConnectionError):
        client.negotiate()

    client.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            },
            {
                "name": "TestAuthenticator",
                "settings": [],
            },
        ]
    }
)
def test_reauthenticate_mechanism_mismatch(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test that reauthentication fails when mechanism changes.

    The client must use the same authentication mechanism for all authentication
    requests during the lifetime of a session. Changing the mechanism should
    result in an error and disconnect.
    """
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Initial authentication with Basic mechanism
    auth_resp = client.authenticate("Basic", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0

    nego_resp = client.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    # Reauthentication with different mechanism (TEST) should fail
    # The broker should reject this because the mechanism changed
    with pytest.raises(ConnectionError):
        client.authenticate("TEST", "")

    client.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                    {"key": "user2", "value": {"stringVal": "password2"}},
                ],
            }
        ]
    }
)
def test_reauthenticate_principal_change(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test that reauthentication fails when principal changes.

    The client's principal cannot change as a result of reauthentication.
    If reauthentication results in a new principal (different user),
    the broker will return an authentication error and disconnect the client.
    """
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Initial authentication as user1
    auth_resp = client.authenticate("Basic", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0

    nego_resp = client.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    # Reauthentication as user2 (generates a different principal) should fail
    auth_resp = client.authenticate("Basic", "user2:password2")
    assert auth_resp["authenticationResponse"]["status"]["code"] != 0

    # Connection should be closed after principal change attempt
    with pytest.raises(ConnectionError):
        client.negotiate()

    client.stop()


# ==============================================================================
# Default Anonymous Credential Tests
# ==============================================================================


def test_default_anonymous_single_node(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test default anonymous authentication on single node."""
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Should succeed with default AnonAuthenticator
    nego_resp = client.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client.stop()


def test_default_anonymous_multi_node(
    multi_node: Cluster,
    sc_domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test default anonymous authentication on multi-node cluster."""
    client = RawClient()
    client.open_channel(*multi_node.admin_endpoint)

    nego_resp = client.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client.stop()


def test_empty_authenticators_reject_basic(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Empty authenticators should reject non-ANONYMOUS mechanisms."""
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Should reject Basic when only default AnonPass is available
    auth_resp = client.authenticate("Basic", "user:pass")
    assert auth_resp["authenticationResponse"]["status"]["code"] != 0

    client.stop()


# ==============================================================================
# Anonymous Credential Tests
# ==============================================================================


@tweak.broker.app_config.authentication({"anonymousCredential": {"disallow": {}}})
def test_anonymous_disallowed(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test that anonymous authentication can be disallowed."""
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Should fail when anonymous is disallowed
    with pytest.raises(ConnectionError):
        client.negotiate()

    client.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            }
        ],
        "anonymousCredential": {
            "credential": {"mechanism": "Basic", "identity": "user1:wrongpass"}
        },
    }
)
def test_anonymous_credential_invalid(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test negotiation with invalid anonymous credential."""
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Should fail with invalid anonymousCredential
    with pytest.raises(ConnectionError):
        client.negotiate()

    client.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            },
        ],
        "anonymousCredential": {
            "credential": {"mechanism": "Basic", "identity": "user1:password1"}
        },
    }
)
def test_anonymous_credential_mechanism_match(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test anonymous credential with matching authenticator."""
    # Test explicit authentication
    client1 = RawClient()
    client1.open_channel(*single_node.admin_endpoint)

    auth_resp = client1.authenticate("Basic", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0

    nego_resp = client1.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client1.stop()

    # Test default authentication via negotiation
    client2 = RawClient()
    client2.open_channel(*single_node.admin_endpoint)

    nego_resp = client2.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client2.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "AnonAuthenticator",
                "settings": [
                    {"key": "shouldPass", "value": {"boolVal": "false"}},
                ],
            },
        ],
        "anonymousCredential": {
            "credential": {"mechanism": "ANONYMOUS", "identity": ""}
        },
    }
)
def test_anon_fail_authenticator(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test AnonAuthenticator that always fails when "shouldPass" is false.
    This tests the scenario where ANONYMOUS mechanism exists but fails authentication.
    """
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Should fail when "shouldPass" is set to fail
    with pytest.raises(ConnectionError):
        client.negotiate()

    client.stop()


# ==============================================================================
# BasicAuthenticator Configuration Tests
# ==============================================================================


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            }
        ]
    }
)
def test_basic_auth_allows_anonymous(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test that BasicAuthenticator coexists with default anonymous (AnonPass)."""
    # Should allow anonymous negotiation (default AnonPass still active)
    client1 = RawClient()
    client1.open_channel(*single_node.admin_endpoint)

    nego_resp = client1.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client1.stop()

    # Should also allow Basic authentication
    client2 = RawClient()
    client2.open_channel(*single_node.admin_endpoint)

    auth_resp = client2.authenticate("Basic", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0

    nego_resp = client2.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client2.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            }
        ]
    }
)
def test_basic_auth_rejects_other_mechanisms(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """BasicAuthenticator should reject unsupported mechanisms."""
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Should reject unsupported mechanism
    auth_resp = client.authenticate("OAuth", "token")
    assert auth_resp["authenticationResponse"]["status"]["code"] != 0

    client.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            },
        ],
        "anonymousCredential": {"disallow": {}},
    }
)
def test_basic_with_anonymous_disallowed(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test BasicAuthenticator with anonymous disallowed."""
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Should fail negotiation without authentication
    with pytest.raises(ConnectionError):
        client.negotiate()

    client.stop()


# ==============================================================================
# Case Sensitivity Tests
# ==============================================================================


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            },
        ],
        "anonymousCredential": {
            "credential": {"mechanism": "BASIC", "identity": "user1:password1"}
        },
    }
)
def test_mechanism_case_insensitive(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test that mechanism names are case-insensitive."""
    # Test lowercase
    client1 = RawClient()
    client1.open_channel(*single_node.admin_endpoint)
    auth_resp = client1.authenticate("basic", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0
    nego_resp = client1.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0
    client1.stop()

    # Test uppercase
    client2 = RawClient()
    client2.open_channel(*single_node.admin_endpoint)
    auth_resp = client2.authenticate("BASIC", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0
    nego_resp = client2.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0
    client2.stop()

    # Test mixed case
    client3 = RawClient()
    client3.open_channel(*single_node.admin_endpoint)
    auth_resp = client3.authenticate("BaSiC", "user1:password1")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0
    nego_resp = client3.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0
    client3.stop()


# ==============================================================================
# Admin Client Tests
# ==============================================================================


def test_admin_with_default_anonymous(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test admin commands with default anonymous authentication."""
    admin = AdminClient()
    admin.connect(*single_node.admin_endpoint)

    assert (
        "This process responds to the following CMD subcommands:"
        in admin.send_admin("help")
    )

    admin.stop()


# ==============================================================================
# Broker Startup Failure Tests
# ==============================================================================


def check_fail_to_start(node: Cluster):
    # Try to start the cluster - should fail due to mismatched mechanism
    try:
        with pytest.raises(ProcessExitError) as exc_info:
            node.start(wait_leader=False)
    finally:
        for proc in node.all_processes:
            proc.check_exit_code = False

    # Verify it's the expected process that failed
    assert "single" in str(exc_info.value)

    # Verify broker is not running
    node = node.nodes()[0]
    assert not node.is_alive(), (
        "Broker should not be running after trying to start with a wrong configuration"
    )


@start_cluster(False)
@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "AnonAuthenticator",
                "settings": [],
            },
            {
                "name": "AnonAuthenticator",  # Duplicate mechanism ANONYMOUS
                "settings": [
                    {"key": "shouldPass", "value": {"boolVal": "false"}},
                ],
            },
        ]
    }
)
def test_duplicate_mechanism_fails_startup(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test that broker fails at startup when two authenticators have the same mechanism.

    This validates that AuthenticationController::initializeAuthenticators() properly
    detects duplicate mechanisms and prevents broker startup.
    """

    check_fail_to_start(single_node)


@start_cluster(False)
@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "BasicAuthenticator",
                "settings": [
                    {"key": "user1", "value": {"stringVal": "password1"}},
                ],
            }
        ],
        "anonymousCredential": {
            "credential": {
                "mechanism": "OAuth",  # Mechanism not matching any authenticator
                "identity": "token123",
            }
        },
    }
)
def test_mismatched_anonymous_credential_fails_startup(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test that broker fails at startup when anonymousCredential uses a mechanism
    that doesn't match any configured authenticator.

    This validates the bidirectional validation logic in validateAnonymousCredential().
    """
    check_fail_to_start(single_node)


@start_cluster(False)
@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {"name": "AnonAuthenticator", "settings": []},
        ],
        # No anonymousCredential specified - custom ANONYMOUS must have credential
    }
)
def test_custom_anonymous_without_credential_fails_startup(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test that broker fails at startup when a custom ANONYMOUS authenticator
    is configured without an anonymousCredential.

    This validates that validateAnonymousCredential() ensures custom ANONYMOUS
    authenticators (non-default) must have an explicit credential configured.
    """
    check_fail_to_start(single_node)


# ==============================================================================
# TestAuthenticator Tests
# ==============================================================================


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "TestAuthenticator",
                "settings": [
                    {"key": "sleepTimeMs", "value": {"intVal": 100}},
                ],
            }
        ]
    }
)
def test_authenticate_with_delay(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """Test TestAuthenticator with a reasonable delay."""
    client = RawClient()
    client.open_channel(*single_node.admin_endpoint)

    # Should succeed even with a delay
    auth_resp = client.authenticate("TEST", "")
    assert auth_resp["authenticationResponse"]["status"]["code"] == 0

    nego_resp = client.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    client.stop()


@tweak.broker.app_config.authentication(
    {
        "authenticators": [
            {
                "name": "TestAuthenticator",
                "settings": [
                    # 10 seconds - long enough for client to disconnect before completion
                    {"key": "sleepTimeMs", "value": {"intVal": 10000}},
                ],
            }
        ]
    }
)
def test_authenticate_client_disconnect_during_auth(
    single_node: Cluster,
    domain_urls: tc.DomainUrls,  # pylint: disable=unused-argument
) -> None:
    """
    Test that the broker handles gracefully when a client disconnects
    while authentication is still in progress.

    This simulates a client that cuts the TCP connection before the
    authentication completes (e.g., due to network issues, client crash,
    or client timeout). The broker should handle this gracefully without
    crashing and should remain functional for new connections.
    """
    # Use a very short timeout so the client disconnects before auth completes
    client = RawClient(socket_timeout=1.0)
    client.open_channel(*single_node.admin_endpoint)

    # Send authentication request - this will timeout on the client side
    # because authentication takes 10 seconds but client timeout is 1 second
    with pytest.raises((ConnectionError, TimeoutError)):
        client.authenticate("TEST", "")

    client.stop()

    # Verify the broker is still functional after the abrupt disconnect
    # by connecting a new client successfully
    new_client = RawClient()
    new_client.open_channel(*single_node.admin_endpoint)

    # Should succeed with default anonymous authentication
    nego_resp = new_client.negotiate()
    assert nego_resp["brokerResponse"]["result"]["code"] == 0

    new_client.stop()
